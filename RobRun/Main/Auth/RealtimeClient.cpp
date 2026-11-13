#include "RealtimeClient.hpp"
#include "AuthClient.hpp"
#include "AuthWindow.hpp"
#include "HWID.hpp"
#include <Windows.h>
#include <winhttp.h>
#include <time.h>
#include <chrono>
#include <Utils/Dependencies/NlohmannJson.hpp>
#include <Utils/FrameWork.hpp>
#include <Utils/Render/kaanlar.h>

#pragma comment(lib, "winhttp.lib")

namespace Cheat
{
    namespace Auth
    {
        std::atomic<bool> RealtimeClient::s_Running{ false };
        std::atomic<bool> RealtimeClient::s_Connected{ false };
        std::thread RealtimeClient::s_WorkerThread;
        std::mutex RealtimeClient::s_SocketMutex;
        HINTERNET RealtimeClient::s_hWebSocket = NULL;
        std::string RealtimeClient::s_ActiveChannel = "";

        bool RealtimeClient::BroadcastVoiceChunk(const std::string& b64Data, float duration)
        {
            std::lock_guard<std::mutex> lock(s_SocketMutex);
            if (!s_hWebSocket || !s_Connected.load() || s_ActiveChannel.empty() || b64Data.empty())
            {
                return false;
            }

            std::string key = AuthWindow::GetCurrentKey();
            std::string username = AuthWindow::GetCurrentUsername();
            static std::atomic<uint64_t> s_broadcastRef{ 1000 };

            nlohmann::json broadcastMsg;
            broadcastMsg["topic"] = s_ActiveChannel;
            broadcastMsg["event"] = "broadcast";
            broadcastMsg["payload"]["type"] = "broadcast";
            broadcastMsg["payload"]["event"] = "live_voice";
            broadcastMsg["payload"]["payload"]["user_key"] = key;
            broadcastMsg["payload"]["payload"]["username"] = username;
            broadcastMsg["payload"]["payload"]["chunk"] = b64Data;
            broadcastMsg["payload"]["payload"]["duration"] = duration;
            broadcastMsg["ref"] = "vc_" + std::to_string(s_broadcastRef++);

            std::string msgStr = broadcastMsg.dump();
            DWORD sendErr = WinHttpWebSocketSend(
                s_hWebSocket,
                WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
                (PVOID)msgStr.data(),
                (DWORD)msgStr.size()
            );

            return (sendErr == ERROR_SUCCESS);
        }

        void RealtimeClient::Start()
        {
            if (s_Running.exchange(true))
            {
                return; // Already started
            }

            s_WorkerThread = std::thread(&RealtimeClient::Worker);
        }

        void RealtimeClient::Stop()
        {
            if (!s_Running.exchange(false))
            {
                return;
            }

            if (s_WorkerThread.joinable())
            {
                s_WorkerThread.join();
            }
        }

        bool RealtimeClient::IsConnected()
        {
            return s_Connected.load();
        }

        void RealtimeClient::Worker()
        {
            while (s_Running)
            {
                std::string key = AuthWindow::GetCurrentKey();
                std::string username = AuthWindow::GetCurrentUsername();
                std::string hwid = AuthWindow::GetHWID();

                if (key.empty())
                {
                    for (int i = 0; i < 20 && s_Running; ++i)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    continue;
                }

                RealtimeConfigResult cfg = AuthClient::GetRealtimeConfig();
                if (!cfg.success || cfg.host.empty() || cfg.path.empty())
                {
                    for (int i = 0; i < 50 && s_Running; ++i)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    continue;
                }

                std::wstring wHost(cfg.host.begin(), cfg.host.end());
                std::wstring wPath(cfg.path.begin(), cfg.path.end());

                HINTERNET hSession = WinHttpOpen(
                    xorstr_(L"CarnageRealtime/1.0"),
                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS,
                    0
                );

                if (!hSession)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), (INTERNET_PORT)cfg.port, 0);
                if (!hConnect)
                {
                    WinHttpCloseHandle(hSession);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                HINTERNET hRequest = WinHttpOpenRequest(
                    hConnect,
                    L"GET",
                    wPath.c_str(),
                    NULL,
                    WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    WINHTTP_FLAG_SECURE
                );

                if (!hRequest)
                {
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                                 SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                                 SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                                 SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
                WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);

                if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                    !WinHttpReceiveResponse(hRequest, NULL))
                {
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                HINTERNET hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);
                WinHttpCloseHandle(hRequest);

                if (!hWebSocket)
                {
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                {
                    std::lock_guard<std::mutex> lock(s_SocketMutex);
                    s_hWebSocket = hWebSocket;
                    s_ActiveChannel = cfg.channel;
                }
                s_Connected = true;

                // 1. Join Realtime presence channel
                nlohmann::json joinMsg;
                joinMsg["topic"] = cfg.channel;
                joinMsg["event"] = "phx_join";
                joinMsg["payload"]["config"]["presence"]["key"] = key;
                joinMsg["ref"] = "1";
                std::string joinStr = joinMsg.dump();
                WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)joinStr.data(), (DWORD)joinStr.size());

                // 2. Track presence payload
                time_t rawtime;
                time(&rawtime);
                struct tm timeinfo;
                gmtime_s(&timeinfo, &rawtime);
                char timeBuf[64];
                strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

                nlohmann::json trackMsg;
                trackMsg["topic"] = cfg.channel;
                trackMsg["event"] = "presence";
                trackMsg["payload"]["type"] = "presence";
                trackMsg["payload"]["event"] = "track";
                trackMsg["payload"]["payload"]["user_key"] = key;
                trackMsg["payload"]["payload"]["username"] = username;
                trackMsg["payload"]["payload"]["hwid"] = hwid;
                trackMsg["payload"]["payload"]["online_at"] = timeBuf;
                trackMsg["ref"] = "2";
                std::string trackStr = trackMsg.dump();
                WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)trackStr.data(), (DWORD)trackStr.size());

                // 3. Dedicated reader thread to consume incoming frames and detect disconnects
                std::atomic<bool> socketAlive{ true };
                std::thread readerThread([hWebSocket, &socketAlive]() {
                    char recvBuffer[2048];
                    while (socketAlive)
                    {
                        DWORD bytesRead = 0;
                        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
                        DWORD err = WinHttpWebSocketReceive(hWebSocket, recvBuffer, sizeof(recvBuffer) - 1, &bytesRead, &bufferType);
                        if (err != ERROR_SUCCESS || bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
                        {
                            socketAlive = false;
                            break;
                        }
                    }
                });

                // 4. Heartbeat keepalive loop
                int hbCounter = 1;
                int intervalTicks = cfg.heartbeatIntervalMs > 0 ? (cfg.heartbeatIntervalMs / 100) : 250;

                while (s_Running && socketAlive)
                {
                    for (int i = 0; i < intervalTicks && s_Running && socketAlive; ++i)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    if (!s_Running || !socketAlive) break;

                    nlohmann::json hbMsg;
                    hbMsg["topic"] = "phoenix";
                    hbMsg["event"] = "heartbeat";
                    hbMsg["payload"] = nlohmann::json::object();
                    hbMsg["ref"] = "hb_" + std::to_string(hbCounter++);
                    std::string hbStr = hbMsg.dump();

                    DWORD sendErr = WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)hbStr.data(), (DWORD)hbStr.size());
                    if (sendErr != ERROR_SUCCESS)
                    {
                        socketAlive = false;
                        break;
                    }
                }

                // 5. Graceful leave if stopping
                if (!s_Running)
                {
                    nlohmann::json leaveMsg;
                    leaveMsg["topic"] = cfg.channel;
                    leaveMsg["event"] = "phx_leave";
                    leaveMsg["payload"] = nlohmann::json::object();
                    leaveMsg["ref"] = "leave";
                    std::string leaveStr = leaveMsg.dump();
                    WinHttpWebSocketSend(hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (PVOID)leaveStr.data(), (DWORD)leaveStr.size());
                }

                {
                    std::lock_guard<std::mutex> lock(s_SocketMutex);
                    s_hWebSocket = NULL;
                    s_ActiveChannel.clear();
                }
                socketAlive = false;
                WinHttpWebSocketClose(hWebSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
                WinHttpCloseHandle(hWebSocket);

                if (readerThread.joinable())
                {
                    readerThread.join();
                }

                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                s_Connected = false;

                if (s_Running)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
        }
    }
}
