#include "AuthClient.hpp"
#include "AuthWindow.hpp"
#include <Windows.h>
#include <winhttp.h>
#include <fstream>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "HWID.hpp"
#include <Utils/Dependencies/NlohmannJson.hpp>
#include <Utils/FrameWork.hpp>
#include <Utils/Render/kaanlar.h>

#pragma comment(lib, "winhttp.lib")

namespace Cheat
{
    namespace Auth
    {
        const std::string AuthClient::APP_VERSION = xorstr_("1.3.1");
        std::wstring AuthClient::s_Host = xorstr_(L"lagsim.vercel.app");
        int AuthClient::s_Port = 443;
        bool AuthClient::s_Https = true;
        bool AuthClient::s_ConfigLoaded = false;
        bool AuthClient::s_DsyncAllowed = false;

        void AuthClient::InitializeConfig()
        {
            if (s_ConfigLoaded) return;
            s_ConfigLoaded = true;
            // Secure production defaults (encrypted compile-time literals)
            s_Host = xorstr_(L"lagsim.vercel.app");
            s_Port = 443;
            s_Https = true;

#ifdef _DEBUG
            // Development-only override: only active in Debug builds
            std::ifstream file("auth_config.json");
            if (file.is_open())
            {
                try
                {
                    nlohmann::json j;
                    file >> j;

                    if (j.contains("host") && j["host"].is_string())
                    {
                        std::string h = j["host"];
                        s_Host = std::wstring(h.begin(), h.end());
                    }
                    if (j.contains("port") && j["port"].is_number())
                    {
                        s_Port = j["port"].get<int>();
                    }
                    if (j.contains("https") && j["https"].is_boolean())
                    {
                        s_Https = j["https"].get<bool>();
                    }
                }
                catch (...) {}
            }
#endif
        }

        std::wstring AuthClient::GetServerHost() { InitializeConfig(); return s_Host; }
        int AuthClient::GetServerPort() { InitializeConfig(); return s_Port; }
        bool AuthClient::IsHttps() { InitializeConfig(); return s_Https; }

        std::string AuthClient::SendGetRequest(const std::wstring& path, int& httpStatusCode)
        {
            InitializeConfig();
            httpStatusCode = 0;
            std::string response = "";

            HINTERNET hSession = WinHttpOpen(xorstr_(L"Mozilla/5.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) return "";

            // Resilient timeouts: 10s resolve/connect, 15s send/receive (prevents false timeouts on serverless cold starts)
            WinHttpSetTimeouts(hSession, 10000, 10000, 15000, 15000);

            HINTERNET hConnect = WinHttpConnect(hSession, s_Host.c_str(), static_cast<INTERNET_PORT>(s_Port), 0);
            if (hConnect)
            {
                DWORD flags = s_Https ? WINHTTP_FLAG_SECURE : 0;
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
                if (hRequest)
                {
                    // C-2 fix: TLS certificate validation is enabled — no SECURITY_FLAG_IGNORE_* flags

                    LPCWSTR headers = L"Accept: application/json\r\n";
                    DWORD headersLen = static_cast<DWORD>(wcslen(headers));

                    if (WinHttpSendRequest(hRequest, headers, headersLen, nullptr, 0, 0, 0))
                    {
                        if (WinHttpReceiveResponse(hRequest, nullptr))
                        {
                            DWORD dwStatusCode = 0;
                            DWORD dwSize = sizeof(dwStatusCode);
                            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
                            httpStatusCode = static_cast<int>(dwStatusCode);

                            DWORD dwDownloaded = 0;
                            do
                            {
                                dwSize = 0;
                                if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0)
                                {
                                    std::vector<char> buffer(dwSize + 1, 0);
                                    if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
                                    {
                                        response.append(buffer.data(), dwDownloaded);
                                    }
                                }
                            } while (dwSize > 0);
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);

            return response;
        }

        std::string AuthClient::SendPostRequest(const std::wstring& path, const std::string& jsonBody, int& httpStatusCode)
        {
            InitializeConfig();
            httpStatusCode = 0;
            std::string response = "";

            HINTERNET hSession = WinHttpOpen(xorstr_(L"Mozilla/5.0"), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) return "";

            // Resilient timeouts: 10s resolve/connect, 15s send/receive (prevents false timeouts on serverless cold starts)
            WinHttpSetTimeouts(hSession, 10000, 10000, 15000, 15000);

            HINTERNET hConnect = WinHttpConnect(hSession, s_Host.c_str(), static_cast<INTERNET_PORT>(s_Port), 0);
            if (hConnect)
            {
                DWORD flags = s_Https ? WINHTTP_FLAG_SECURE : 0;
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
                if (hRequest)
                {
                    // C-2 fix: TLS certificate validation is enabled — no SECURITY_FLAG_IGNORE_* flags

                    LPCWSTR headers = L"Content-Type: application/json\r\nAccept: application/json\r\n";
                    DWORD headersLen = static_cast<DWORD>(wcslen(headers));

                    LPVOID pData = const_cast<char*>(jsonBody.c_str());
                    DWORD dataLen = static_cast<DWORD>(jsonBody.length());

                    if (WinHttpSendRequest(hRequest, headers, headersLen, pData, dataLen, dataLen, 0))
                    {
                        if (WinHttpReceiveResponse(hRequest, nullptr))
                        {
                            DWORD dwStatusCode = 0;
                            DWORD dwSize = sizeof(dwStatusCode);
                            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
                            httpStatusCode = static_cast<int>(dwStatusCode);

                            DWORD dwDownloaded = 0;
                            do
                            {
                                dwSize = 0;
                                if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0)
                                {
                                    std::vector<char> buffer(dwSize + 1, 0);
                                    if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
                                    {
                                        response.append(buffer.data(), dwDownloaded);
                                    }
                                }
                            } while (dwSize > 0);
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);

            return response;
        }

        HealthCheckResult AuthClient::CheckHealth()
        {
            HealthCheckResult result;
            int statusCode = 0;
            std::string rawJson = SendGetRequest(L"/", statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Unable to connect to server. Service is offline.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.status = j.value("status", "");
                result.service = j.value("service", "");
                result.database = j.value("database", "");
                result.latencyMs = j.value("latencyMs", 0);
                result.timestamp = j.value("timestamp", "");

                if (result.status == "ok" && result.database == "ok")
                {
                    result.success = true;
                }
                else
                {
                    result.success = false;
                    if (result.status != "ok")
                    {
                        result.error = "Service status is " + (result.status.empty() ? std::string("offline") : result.status) + ".";
                    }
                    else
                    {
                        result.error = "Database is " + (result.database.empty() ? std::string("offline") : result.database) + ".";
                    }
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Server response error: ") + e.what();
            }

            return result;
        }

        CheckVersionResult AuthClient::CheckVersion(const std::string& currentVersion)
        {
            CheckVersionResult result;
            nlohmann::json payload;
            payload["version"] = currentVersion;

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/auth/check-version", payload.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.valid = false;
                result.error = xorstr_("Connection failed while checking version.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.valid = j.value("valid", false);
                result.latestVersion = j.value("latest_version", "");
                result.downloadLink = j.value("download_link", "");
                result.error = j.value("error", "");
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.valid = false;
                result.error = std::string("Malformed version response: ") + e.what();
            }

            return result;
        }

        CheckHwidResult AuthClient::CheckHWID(const std::string& hwid)
        {
            CheckHwidResult result;
            nlohmann::json payload;
            payload["hwid"] = hwid;
            payload["app_version"] = APP_VERSION;
            payload["version"] = APP_VERSION;

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/auth/check-hwid", payload.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);

                if (result.success)
                {
                    result.registered = j.value("registered", false);
                    result.banned = j.value("banned", false);
                    result.expired = j.value("expired", false);
                    result.message = j.value("message", "");
                    if (result.message.find("expired") != std::string::npos || result.message.find("Expired") != std::string::npos)
                    {
                        result.expired = true;
                    }

                    if (j.contains("user") && j["user"].is_object())
                    {
                        result.userId = j["user"].value("id", "");
                        result.username = j["user"].value("username", "");
                        result.key = j["user"].value("key", "");
                        result.createdAt = j["user"].value("created_at", "");
                        result.dsync = j["user"].value("dsync", false);
                        SetDsyncAllowed(result.dsync);
                        if (j["user"].contains("expires_in_days") && !j["user"]["expires_in_days"].is_null())
                        {
                            result.expiresInDays = j["user"].value("expires_in_days", -1);
                        }
                    }
                    if (result.expiresInDays < 0 && j.contains("expires_in_days") && !j["expires_in_days"].is_null())
                    {
                        result.expiresInDays = j.value("expires_in_days", -1);
                    }
                    else if (result.expiresInDays < 0 && j.contains("duration") && !j["duration"].is_null())
                    {
                        int64_t dur = j.value("duration", (int64_t)-1);
                        if (dur >= 0)
                            result.expiresInDays = (int)std::ceil(dur / 86400.0);
                    }
                }
                else
                {
                    result.error = j.value("error", "Hardware ID verification failed.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        RegisterResult AuthClient::Register(const RegisterPayload& payload)
        {
            RegisterResult result;
            nlohmann::json req;
            req["key"] = payload.key;
            req["username"] = payload.username;
            req["hwid"] = payload.hwid;
            req["app_version"] = APP_VERSION;
            req["version"] = APP_VERSION;

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/auth/register", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.message = j.value("message", "");

                if (result.success)
                {
                    if (j.contains("user") && j["user"].is_object())
                    {
                        result.username = j["user"].value("username", payload.username);
                        result.createdAt = j["user"].value("created_at", "");
                        if (j["user"].contains("expires_in_days") && !j["user"]["expires_in_days"].is_null())
                        {
                            result.expiresInDays = j["user"].value("expires_in_days", -1);
                        }
                    }
                    else
                    {
                        result.username = payload.username;
                    }
                    if (result.expiresInDays < 0 && j.contains("expires_in_days") && !j["expires_in_days"].is_null())
                    {
                        result.expiresInDays = j.value("expires_in_days", -1);
                    }
                    else if (result.expiresInDays < 0 && j.contains("duration") && !j["duration"].is_null())
                    {
                        int64_t dur = j.value("duration", (int64_t)-1);
                        if (dur >= 0)
                            result.expiresInDays = (int)std::ceil(dur / 86400.0);
                    }
                }
                else
                {
                    result.error = j.value("error", "Registration rejected by server.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        CreateConfigResult AuthClient::CreateConfig(const CreateConfigPayload& payload)
        {
            CreateConfigResult result;
            nlohmann::json req;
            req["config_name"] = payload.config_name;
            req["created_by"] = payload.created_by;
            // C-3 fix: Include HWID for server-side auth and username verification
            req["hwid"] = AuthWindow::GetHWID();
            req["health_bind"] = payload.health_bind;
            req["armor_bind"] = payload.armor_bind;
            req["vehicle_repair_bind"] = payload.vehicle_repair_bind;
            req["teleport_waypoint_bind"] = payload.teleport_waypoint_bind;
            req["esp_enabled"] = payload.esp_enabled;
            req["esp_name"] = payload.esp_name;
            req["distance"] = payload.distance;
            req["distance_value"] = payload.distance_value;
            req["health_bar"] = payload.health_bar;
            req["armor_bar"] = payload.armor_bar;
            req["silent_aim_enabled"] = payload.silent_aim_enabled;
            req["fov_size"] = payload.fov_size;
            req["fov_visibility"] = payload.fov_visibility;
            req["fov_distance"] = payload.fov_distance;
            req["silent_ignore_dead"] = payload.silent_ignore_dead;
            req["silent_ignore_invisible"] = payload.silent_ignore_invisible;
            req["silent_ignore_friends"] = payload.silent_ignore_friends;
            req["aimbot_enabled"] = payload.aimbot_enabled;
            req["aimbot_fov"] = payload.aimbot_fov;
            req["aimbot_distance"] = payload.aimbot_distance;
            req["aimbot_show_fov"] = payload.aimbot_show_fov;
            req["aimbot_smoothness"] = payload.aimbot_smoothness;
            req["aimbot_ignore_dead"] = payload.aimbot_ignore_dead;
            req["aimbot_ignore_invisible"] = payload.aimbot_ignore_invisible;
            req["aimbot_ignore_friends"] = payload.aimbot_ignore_friends;
            req["friends_list"] = payload.friends_list;
            req["friend_color"] = payload.friend_color;
            req["spectator_list"] = payload.spectator_list;

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/config/create", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection to config server failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.message = j.value("message", "");
                result.config_key = j.value("config_key", "");

                if (!result.success)
                {
                    result.error = j.value("message", "Failed to create config.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        GetConfigResult AuthClient::GetConfig(const std::string& configKey)
        {
            GetConfigResult result;
            nlohmann::json req;
            req["config_key"] = configKey;
            // H-7 fix: Include HWID so the server can verify the caller is an active user
            req["hwid"] = AuthWindow::GetHWID();

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/config/get", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                if (result.success && j.contains("config") && j["config"].is_object())
                {
                    auto& c = j["config"];
                    result.config_key = c.value("config_key", "");
                    result.config_name = c.value("config_name", "");
                    result.created_by = c.value("created_by", "");
                    result.created_at = c.value("created_at", "");
                    result.health_bind = c.value("health_bind", 0);
                    result.armor_bind = c.value("armor_bind", 0);
                    result.vehicle_repair_bind = c.value("vehicle_repair_bind", 0);
                    result.teleport_waypoint_bind = c.value("teleport_waypoint_bind", 0);
                    result.esp_enabled = c.value("esp_enabled", false);
                    result.esp_name = c.value("esp_name", false);
                    result.distance = c.value("distance", false);
                    result.distance_value = c.value("distance_value", 250);
                    result.health_bar = c.value("health_bar", false);
                    result.armor_bar = c.value("armor_bar", false);
                    result.silent_aim_enabled = c.value("silent_aim_enabled", false);
                    result.fov_size = c.value("fov_size", 100.f);
                    result.fov_visibility = c.value("fov_visibility", false);
                    result.fov_distance = c.value("fov_distance", 250);
                    result.silent_ignore_dead = c.value("silent_ignore_dead", false);
                    result.silent_ignore_invisible = c.value("silent_ignore_invisible", false);
                    result.silent_ignore_friends = c.value("silent_ignore_friends", false);
                    result.aimbot_enabled = c.value("aimbot_enabled", false);
                    result.aimbot_fov = c.value("aimbot_fov", 70);
                    result.aimbot_distance = c.value("aimbot_distance", 200);
                    result.aimbot_show_fov = c.value("aimbot_show_fov", false);
                    result.aimbot_smoothness = c.value("aimbot_smoothness", 10);
                    result.aimbot_ignore_dead = c.value("aimbot_ignore_dead", false);
                    result.aimbot_ignore_invisible = c.value("aimbot_ignore_invisible", false);
                    result.aimbot_ignore_friends = c.value("aimbot_ignore_friends", false);
                    if (c.contains("friends_list") && c["friends_list"].is_array())
                    {
                        for (const auto& fItem : c["friends_list"])
                        {
                            if (fItem.is_string())
                                result.friends_list.push_back(fItem.get<std::string>());
                            else if (fItem.is_number())
                                result.friends_list.push_back(std::to_string(fItem.get<int>()));
                        }
                    }
                    result.friend_color = c.value("friend_color", "");
                    result.spectator_list = c.value("spectator_list", true);
                }
                else
                {
                    result.error = j.value("message", "Config not found.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        AddRelationResult AuthClient::AddUserConfig(const std::string& username, const std::string& configKey)
        {
            AddRelationResult result;
            nlohmann::json req;
            req["username"] = username.empty() ? AuthWindow::GetCurrentUsername() : username;
            req["config_key"] = configKey;
            // H-4 fix: Include HWID so the server can verify ownership
            req["hwid"] = AuthWindow::GetHWID();

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/config/relation/add", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection to config server failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.message = j.value("message", "");
                result.limit_reached = j.value("limit_reached", false);

                if (result.success && j.contains("config") && j["config"].is_object())
                {
                    auto& c = j["config"];
                    result.config.relation_id = c.value("relation_id", "");
                    result.config.config_key = c.value("config_key", configKey);
                    result.config.config_name = c.value("config_name", "");
                    result.config.created_by = c.value("created_by", "");
                    result.config.created_at = c.value("created_at", "");
                    result.config.health_bind = c.value("health_bind", 0);
                    result.config.armor_bind = c.value("armor_bind", 0);
                    result.config.vehicle_repair_bind = c.value("vehicle_repair_bind", 0);
                    result.config.teleport_waypoint_bind = c.value("teleport_waypoint_bind", 0);
                    result.config.esp_enabled = c.value("esp_enabled", false);
                    result.config.esp_name = c.value("esp_name", false);
                    result.config.distance = c.value("distance", false);
                    result.config.distance_value = c.value("distance_value", 250);
                    result.config.health_bar = c.value("health_bar", false);
                    result.config.armor_bar = c.value("armor_bar", false);
                    result.config.silent_aim_enabled = c.value("silent_aim_enabled", false);
                    result.config.fov_size = c.value("fov_size", 100.f);
                    result.config.fov_visibility = c.value("fov_visibility", false);
                    result.config.fov_distance = c.value("fov_distance", 250);
                    result.config.silent_ignore_dead = c.value("silent_ignore_dead", false);
                    result.config.silent_ignore_invisible = c.value("silent_ignore_invisible", false);
                    result.config.silent_ignore_friends = c.value("silent_ignore_friends", false);
                    result.config.aimbot_enabled = c.value("aimbot_enabled", false);
                    result.config.aimbot_fov = c.value("aimbot_fov", 70);
                    result.config.aimbot_distance = c.value("aimbot_distance", 200);
                    result.config.aimbot_show_fov = c.value("aimbot_show_fov", false);
                    result.config.aimbot_smoothness = c.value("aimbot_smoothness", 10);
                    result.config.aimbot_ignore_dead = c.value("aimbot_ignore_dead", false);
                    result.config.aimbot_ignore_invisible = c.value("aimbot_ignore_invisible", false);
                    result.config.aimbot_ignore_friends = c.value("aimbot_ignore_friends", false);
                    if (c.contains("friends_list") && c["friends_list"].is_array())
                    {
                        for (const auto& fItem : c["friends_list"])
                        {
                            if (fItem.is_string())
                                result.config.friends_list.push_back(fItem.get<std::string>());
                            else if (fItem.is_number())
                                result.config.friends_list.push_back(std::to_string(fItem.get<int>()));
                        }
                    }
                    result.config.friend_color = c.value("friend_color", "");
                    result.config.spectator_list = c.value("spectator_list", true);
                }
                else if (!result.success)
                {
                    result.error = j.value("message", "Failed to add config relation.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        ListRelationsResult AuthClient::ListUserConfigs(const std::string& username)
        {
            ListRelationsResult result;
            nlohmann::json req;
            req["username"] = username.empty() ? AuthWindow::GetCurrentUsername() : username;
            // H-6 fix: Include HWID so the server can verify the caller owns this username
            req["hwid"] = AuthWindow::GetHWID();

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/config/relation/list", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection to config server failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.count = j.value("count", 0);

                if (result.success && j.contains("configs") && j["configs"].is_array())
                {
                    for (auto& c : j["configs"])
                    {
                        UserConfigItem item;
                        item.relation_id = c.value("relation_id", "");
                        item.config_key = c.value("config_key", "");
                        item.config_name = c.value("config_name", "");
                        item.created_by = c.value("created_by", "");
                        item.created_at = c.value("created_at", "");
                        item.health_bind = c.value("health_bind", 0);
                        item.armor_bind = c.value("armor_bind", 0);
                        item.vehicle_repair_bind = c.value("vehicle_repair_bind", 0);
                        item.teleport_waypoint_bind = c.value("teleport_waypoint_bind", 0);
                        item.esp_enabled = c.value("esp_enabled", false);
                        item.esp_name = c.value("esp_name", false);
                        item.distance = c.value("distance", false);
                        item.distance_value = c.value("distance_value", 250);
                        item.health_bar = c.value("health_bar", false);
                        item.armor_bar = c.value("armor_bar", false);
                        item.silent_aim_enabled = c.value("silent_aim_enabled", false);
                        item.fov_size = c.value("fov_size", 100.f);
                        item.fov_visibility = c.value("fov_visibility", false);
                        item.fov_distance = c.value("fov_distance", 250);
                        item.silent_ignore_dead = c.value("silent_ignore_dead", false);
                        item.silent_ignore_invisible = c.value("silent_ignore_invisible", false);
                        item.silent_ignore_friends = c.value("silent_ignore_friends", false);
                        item.aimbot_enabled = c.value("aimbot_enabled", false);
                        item.aimbot_fov = c.value("aimbot_fov", 70);
                        item.aimbot_distance = c.value("aimbot_distance", 200);
                        item.aimbot_show_fov = c.value("aimbot_show_fov", false);
                        item.aimbot_smoothness = c.value("aimbot_smoothness", 10);
                        item.aimbot_ignore_dead = c.value("aimbot_ignore_dead", false);
                        item.aimbot_ignore_invisible = c.value("aimbot_ignore_invisible", false);
                        item.aimbot_ignore_friends = c.value("aimbot_ignore_friends", false);
                        if (c.contains("friends_list") && c["friends_list"].is_array())
                        {
                            for (const auto& fItem : c["friends_list"])
                            {
                                if (fItem.is_string())
                                    item.friends_list.push_back(fItem.get<std::string>());
                                else if (fItem.is_number())
                                    item.friends_list.push_back(std::to_string(fItem.get<int>()));
                            }
                        }
                        item.friend_color = c.value("friend_color", "");
                        item.spectator_list = c.value("spectator_list", true);
                        result.configs.push_back(item);
                    }
                }
                else if (!result.success)
                {
                    result.error = j.value("message", "Failed to fetch user configs.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        DeleteRelationResult AuthClient::DeleteUserConfig(const std::string& username, const std::string& configKey, const std::string& relationId)
        {
            DeleteRelationResult result;
            nlohmann::json req;
            req["username"] = username;
            req["config_key"] = configKey;
            if (!relationId.empty())
                req["relation_id"] = relationId;
            // H-5 fix: Include HWID so the server can verify ownership before deleting
            req["hwid"] = AuthWindow::GetHWID();

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/config/relation/delete", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection to config server failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.message = j.value("message", "");
                if (!result.success)
                {
                    result.error = j.value("message", "Failed to delete config.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        static std::atomic<bool> s_TelemetryRunning{ false };
        static std::string s_TelHwid;
        static std::string s_TelKey;
        static std::string s_TelUser;

        TelemetryResult AuthClient::SendTelemetry(const TelemetryPayload& payload)
        {
            TelemetryResult result;
            nlohmann::json req;
            req["hwid"] = payload.hwid;
            req["key"] = payload.key;
            req["username"] = payload.username;
            req["event"] = payload.event;
            req["app_version"] = payload.app_version;
            req["pc_username"] = payload.pc_username;
            req["cpu_name"] = payload.cpu_name;
            req["gpu_name"] = payload.gpu_name;
            req["motherboard"] = payload.motherboard;
            req["ram_capacity"] = payload.ram_capacity;
            req["ip"] = payload.ip;
            req["location"] = payload.location;
            req["isp"] = payload.isp;

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/auth/telemetry", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Connection to telemetry service failed.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.event = j.value("event", payload.event);
                result.launch_count = j.value("launch_count", 0);
                if (!result.success)
                {
                    result.error = j.value("error", "Telemetry update failed.");
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed response: ") + e.what();
            }

            return result;
        }

        void AuthClient::StartTelemetry(const std::string& hwid, const std::string& key, const std::string& username)
        {
            s_TelHwid = hwid;
            s_TelKey = key;
            s_TelUser = username;

            if (s_TelemetryRunning.exchange(true))
            {
                return; // Already active
            }

            std::thread([hwid, key, username]() {
                // Gather full hardware specs and precise location
                HardwareInfo hw = HWID::GetHardwareInfo();
                LocationInfo loc = HWID::GetLocationInfo();

                TelemetryPayload p;
                p.hwid = hwid;
                p.key = key;
                p.username = username;
                p.event = "launch";
                p.app_version = APP_VERSION;
                p.pc_username = hw.pcUsername;
                p.cpu_name = hw.cpu;
                p.gpu_name = hw.gpu;
                p.motherboard = hw.motherboard;
                p.ram_capacity = hw.ramCapacity;
                p.ip = loc.ip;
                p.location = loc.formattedLocation;
                p.isp = loc.isp;

                SendTelemetry(p);
            }).detach();
        }

        void AuthClient::StopTelemetry()
        {
            s_TelemetryRunning = false;
        }

        BroadcastResult AuthClient::GetBroadcast()
        {
            BroadcastResult result;
            int statusCode = 0;

            std::wstring path = L"/api/v1/auth/broadcast";
            std::string hwid = s_TelHwid.empty() ? HWID::GetHWID() : s_TelHwid;
            if (hwid.empty()) hwid = AuthWindow::GetHWID();

            std::string user = s_TelUser;
            if (user.empty()) user = AuthWindow::GetCurrentUsername();

            std::string key = s_TelKey;
            if (key.empty()) key = AuthWindow::GetCurrentKey();

            std::string qs = "";
            if (!hwid.empty()) qs += (qs.empty() ? "?" : "&") + std::string("hwid=") + hwid;
            if (!user.empty()) qs += (qs.empty() ? "?" : "&") + std::string("username=") + user;
            if (!key.empty()) qs += (qs.empty() ? "?" : "&") + std::string("key=") + key;

            if (!qs.empty())
            {
                path += std::wstring(qs.begin(), qs.end());
            }

            std::string rawJson = SendGetRequest(path, statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = "Connection failed";
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                if (j.contains("broadcast") && !j["broadcast"].is_null())
                {
                    auto b = j["broadcast"];
                    result.hasBroadcast = true;
                    result.id = b.value("id", "");
                    result.message = b.value("message", "");
                    result.duration = b.value("duration", 15);
                    result.active = b.value("active", false);
                    result.style = b.value("style", "broken_screen");
                    result.targetType = b.value("target_type", "all");
                    result.targetUser = b.value("target_user", "");
                    result.videoUrl = b.value("video_url", "");
                    result.createdAt = b.value("created_at", "");
                    result.createdBy = b.value("created_by", "");
                }
                else
                {
                    result.hasBroadcast = false;
                }
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = e.what();
            }

            return result;
        }

        VoiceSuggestionResult AuthClient::SendVoiceSuggestion(const VoiceSuggestionPayload& payload)
        {
            VoiceSuggestionResult result;
            nlohmann::json req;
            req["username"] = payload.username.empty() ? AuthWindow::GetCurrentUsername() : payload.username;
            req["hwid"] = payload.hwid.empty() ? AuthWindow::GetHWID() : payload.hwid;
            req["audio_base64"] = payload.audio_base64;
            req["duration_seconds"] = payload.duration_seconds;
            req["note"] = payload.note;

            int statusCode = 0;
            std::string rawJson = SendPostRequest(L"/api/v1/voice-suggestion", req.dump(), statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = xorstr_("Failed to connect to backend server.");
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.message = j.value("message", "");
                result.audio_url = j.value("audio_url", "");
                result.error = j.value("error", "");
                if (!result.success && result.message.empty() && !result.error.empty())
                    result.message = result.error;
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = std::string("Malformed server response: ") + e.what();
            }

            return result;
        }

        UserVoiceStatusResult AuthClient::GetUserVoiceStatus(const std::string& key)
        {
            UserVoiceStatusResult result;
            int statusCode = 0;
            std::string targetKey = key.empty() ? AuthWindow::GetCurrentKey() : key;
            if (targetKey.empty())
            {
                result.success = false;
                result.error = "Empty user key";
                return result;
            }

            std::wstring path = L"/api/v1/auth/uservoice?key=" + std::wstring(targetKey.begin(), targetKey.end());
            std::string rawJson = SendGetRequest(path, statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = "Connection failed to server.";
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.voicestatus = j.value("voicestatus", false);
                result.user_key = j.value("user_key", targetKey);
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = e.what();
            }

            return result;
        }

        RealtimeConfigResult AuthClient::GetRealtimeConfig()
        {
            RealtimeConfigResult result;
            int statusCode = 0;
            std::string rawJson = SendGetRequest(L"/api/v1/auth/realtime-config", statusCode);

            if (statusCode == 0 || rawJson.empty())
            {
                result.success = false;
                result.error = "Connection failed to server.";
                return result;
            }

            try
            {
                auto j = nlohmann::json::parse(rawJson);
                result.success = j.value("success", false);
                result.host = j.value("host", "smpzhjaeqvnhublrajcz.supabase.co");
                result.port = j.value("port", 443);
                result.path = j.value("path", "");
                result.channel = j.value("channel", "realtime:presence-players");
                result.heartbeatIntervalMs = j.value("heartbeat_interval_ms", 25000);
            }
            catch (const std::exception& e)
            {
                result.success = false;
                result.error = e.what();
            }

            return result;
        }

        bool AuthClient::IsDsyncAllowed()
        {
            return s_DsyncAllowed;
        }

        void AuthClient::SetDsyncAllowed(bool allowed)
        {
            s_DsyncAllowed = allowed;
        }

        bool AuthClient::QueryDsyncStatus(const std::string& key)
        {
            if (key.empty()) return false;
            int statusCode = 0;
            std::string rawJson = SendGetRequest(L"/api/v1/auth/dsync?key=" + std::wstring(key.begin(), key.end()), statusCode);
            if (statusCode == 200 && !rawJson.empty())
            {
                try
                {
                    auto j = nlohmann::json::parse(rawJson);
                    bool allowed = j.value("dsync", false);
                    SetDsyncAllowed(allowed);
                    return allowed;
                }
                catch (...) {}
            }
            return false;
        }

        // -----------------------------------------------------------------------
        // ConfigCache Implementation (Thread-safe, pre-fetched at auth, RAM wiped at exit)
        // -----------------------------------------------------------------------
        static std::vector<UserConfigItem> s_CachedConfigs;
        static std::mutex s_ConfigCacheMutex;
        static std::atomic<bool> s_CacheFetching(false);
        static std::atomic<bool> s_CacheFetchDone(false);
        static std::atomic<bool> s_CacheWiped(false);
        static std::atomic<uint32_t> s_CacheVersion(1);
        static std::chrono::steady_clock::time_point s_LastFetchAttempt{};
        static std::mutex s_FetchThrottleMutex;

        static std::string s_ActiveConfigName = "";
        static std::string s_ActiveConfigKey = "";
        static std::mutex s_ActiveConfigMutex;
        static std::atomic<uint32_t> s_ActiveConfigVersion(1);

        uint32_t ConfigCache::GetCacheVersion()
        {
            return s_CacheVersion.load(std::memory_order_relaxed);
        }

        uint32_t ConfigCache::GetActiveConfigVersion()
        {
            return s_ActiveConfigVersion.load(std::memory_order_relaxed);
        }

        void ConfigCache::RefreshAsync(const std::string& username)
        {
            s_CacheWiped = false;

            // Cooldown throttle: minimum 3 seconds between background refresh threads
            {
                std::lock_guard<std::mutex> lock(s_FetchThrottleMutex);
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_LastFetchAttempt).count();
                if (elapsed < 3000)
                    return;
                s_LastFetchAttempt = now;
            }

            if (s_CacheFetching.exchange(true))
                return;

            std::string user = username;
            if (user.empty())
                user = AuthWindow::GetCurrentUsername();

            if (user.empty())
            {
                s_CacheFetching = false;
                s_CacheFetchDone = true;
                return;
            }

            std::thread([user]() {
                auto res = AuthClient::ListUserConfigs(user);
                // Transient retry if failed on startup
                if (!res.success && !s_CacheWiped.load())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    res = AuthClient::ListUserConfigs(user);
                }
                if (s_CacheWiped.load())
                {
                    s_CacheFetching = false;
                    return;
                }
                if (res.success)
                {
                    std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
                    if (!s_CacheWiped.load())
                    {
                        s_CachedConfigs = res.configs;
                        s_CacheVersion.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                s_CacheFetchDone = true;
                s_CacheFetching = false;
            }).detach();
        }

        void ConfigCache::RefreshSync(const std::string& username)
        {
            s_CacheWiped = false;
            std::string user = username;
            if (user.empty())
                user = AuthWindow::GetCurrentUsername();

            if (user.empty())
            {
                s_CacheFetchDone = true;
                s_CacheFetching = false;
                return;
            }

            s_CacheFetching = true;
            auto res = AuthClient::ListUserConfigs(user);
            if (res.success && !s_CacheWiped.load())
            {
                std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
                if (!s_CacheWiped.load())
                {
                    s_CachedConfigs = res.configs;
                    s_CacheVersion.fetch_add(1, std::memory_order_relaxed);
                }
            }
            s_CacheFetchDone = true;
            s_CacheFetching = false;
        }

        std::vector<UserConfigItem> ConfigCache::GetConfigs()
        {
            std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
            return s_CachedConfigs;
        }

        void ConfigCache::SetConfigs(const std::vector<UserConfigItem>& configs)
        {
            std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
            s_CachedConfigs = configs;
            s_CacheFetchDone = true;
            s_CacheVersion.fetch_add(1, std::memory_order_relaxed);
        }

        void ConfigCache::AddOrUpdate(const UserConfigItem& item)
        {
            std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
            bool found = false;
            for (auto& existing : s_CachedConfigs)
            {
                if (existing.config_key == item.config_key)
                {
                    existing = item;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                s_CachedConfigs.insert(s_CachedConfigs.begin(), item);
                if (s_CachedConfigs.size() > 5)
                    s_CachedConfigs.resize(5);
            }
            s_CacheFetchDone = true;
            s_CacheVersion.fetch_add(1, std::memory_order_relaxed);
        }

        void ConfigCache::Remove(const std::string& configKey)
        {
            std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
            for (auto it = s_CachedConfigs.begin(); it != s_CachedConfigs.end(); ++it)
            {
                if (it->config_key == configKey)
                {
                    if (!it->config_name.empty())
                        SecureZeroMemory(const_cast<char*>(it->config_name.data()), it->config_name.size());
                    if (!it->config_key.empty())
                        SecureZeroMemory(const_cast<char*>(it->config_key.data()), it->config_key.size());
                    if (!it->relation_id.empty())
                        SecureZeroMemory(const_cast<char*>(it->relation_id.data()), it->relation_id.size());
                    if (!it->created_by.empty())
                        SecureZeroMemory(const_cast<char*>(it->created_by.data()), it->created_by.size());

                    s_CachedConfigs.erase(it);
                    s_CacheVersion.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
        }

        bool ConfigCache::IsFetching()
        {
            return s_CacheFetching.load();
        }

        bool ConfigCache::IsInitialFetchDone()
        {
            return s_CacheFetchDone.load();
        }

        void ConfigCache::SetActiveConfig(const std::string& name, const std::string& key)
        {
            std::lock_guard<std::mutex> lock(s_ActiveConfigMutex);
            s_ActiveConfigName = name;
            s_ActiveConfigKey = key;
            s_ActiveConfigVersion.fetch_add(1, std::memory_order_relaxed);
        }

        void ConfigCache::GetActiveConfig(std::string& outName, std::string& outKey)
        {
            std::lock_guard<std::mutex> lock(s_ActiveConfigMutex);
            outName = s_ActiveConfigName;
            outKey = s_ActiveConfigKey;
        }

        void ConfigCache::Wipe()
        {
            s_CacheWiped = true;
            s_CacheVersion.fetch_add(1, std::memory_order_relaxed);
            s_ActiveConfigVersion.fetch_add(1, std::memory_order_relaxed);

            // 1. Wipe active config tracking
            {
                std::lock_guard<std::mutex> lock(s_ActiveConfigMutex);
                if (!s_ActiveConfigName.empty())
                {
                    SecureZeroMemory(const_cast<char*>(s_ActiveConfigName.data()), s_ActiveConfigName.size());
                    s_ActiveConfigName.clear();
                }
                if (!s_ActiveConfigKey.empty())
                {
                    SecureZeroMemory(const_cast<char*>(s_ActiveConfigKey.data()), s_ActiveConfigKey.size());
                    s_ActiveConfigKey.clear();
                }
            }

            // 2. Wipe every cached item buffer thoroughly
            {
                std::lock_guard<std::mutex> lock(s_ConfigCacheMutex);
                for (auto& item : s_CachedConfigs)
                {
                    if (!item.relation_id.empty())
                    {
                        SecureZeroMemory(const_cast<char*>(item.relation_id.data()), item.relation_id.size());
                        item.relation_id.clear();
                    }
                    if (!item.config_key.empty())
                    {
                        SecureZeroMemory(const_cast<char*>(item.config_key.data()), item.config_key.size());
                        item.config_key.clear();
                    }
                    if (!item.config_name.empty())
                    {
                        SecureZeroMemory(const_cast<char*>(item.config_name.data()), item.config_name.size());
                        item.config_name.clear();
                    }
                    if (!item.created_by.empty())
                    {
                        SecureZeroMemory(const_cast<char*>(item.created_by.data()), item.created_by.size());
                        item.created_by.clear();
                    }
                    if (!item.created_at.empty())
                    {
                        SecureZeroMemory(const_cast<char*>(item.created_at.data()), item.created_at.size());
                        item.created_at.clear();
                    }

                    item.health_bind = 0;
                    item.armor_bind = 0;
                    item.vehicle_repair_bind = 0;
                    item.teleport_waypoint_bind = 0;
                    item.esp_enabled = false;
                    item.esp_name = false;
                    item.distance = false;
                    item.distance_value = 0;
                    item.health_bar = false;
                    item.armor_bar = false;
                    item.silent_aim_enabled = false;
                    item.fov_size = 0.f;
                    item.fov_visibility = false;
                    item.fov_distance = 0;
                }

                s_CachedConfigs.clear();
                std::vector<UserConfigItem>().swap(s_CachedConfigs); // Frees the heap capacity back to Windows RAM
                s_CacheFetchDone = false;
                s_CacheFetching = false;
            }
        }
    }
}
