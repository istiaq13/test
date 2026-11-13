#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <windows.h>
#include <winhttp.h>

namespace Cheat
{
    namespace Auth
    {
        class RealtimeClient
        {
        public:
            static void Start();
            static void Stop();
            static bool IsConnected();
            static bool BroadcastVoiceChunk(const std::string& b64Data, float duration);

        private:
            static void Worker();

            static std::atomic<bool> s_Running;
            static std::atomic<bool> s_Connected;
            static std::thread s_WorkerThread;

            static std::mutex s_SocketMutex;
            static HINTERNET s_hWebSocket;
            static std::string s_ActiveChannel;
        };
    }
}

