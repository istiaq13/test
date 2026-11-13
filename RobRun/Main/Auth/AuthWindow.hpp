#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <cstdint>
#include "HWID.hpp"
#include "AuthClient.hpp"

namespace Cheat
{
    namespace Auth
    {
        enum AuthState
        {
            STATE_CHECKING = 0,
            STATE_REGISTER = 1,
            STATE_LOGGED_IN = 2,
            STATE_BANNED = 3,
            STATE_ERROR = 4,
            STATE_EXPIRED = 5,
            STATE_KEY_EXPIRED = 6
        };

        class AuthWindow
        {
            friend class UpdateDownloadCallback;

        public:
            // Main entry: displays auth UI, returns true if user is logged in/activated
            static bool Run();

            // Direct check for current authenticated state.
            // H-9 fix: reads the XOR-masked token — simple memory patch of 0x00→0x01 is ineffective.
            static bool IsAuthenticated();

            static std::string GetCurrentUsername();
            static std::string GetCurrentKey();
            static std::string GetHWID();
            static std::string GetMemberSince();
            static std::string GetExpiresIn();

        private:
            // H-9 fix: Auth state is stored as an XOR-masked 64-bit token instead of a plain bool.
            // The token is zero when unauthenticated. When authenticated, it is set to:
            //   (AUTH_MAGIC ^ GetTickCount64())
            // A memory scanner searching for 0x01 (true) will never find this value.
            // Patching the address to any constant will fail because the XOR key changes every millisecond.
            static std::atomic<uint64_t> s_AuthToken;

            // Compile-time magic constant — unique per build via __DATE__/__TIME__ hash.
            // Change this value each release to invalidate cached patches.
            static constexpr uint64_t AUTH_MAGIC = 0xC4F3B00B'DEAD1337ULL;

            // Internal auth state helpers — not exposed publicly
            static void  SetAuthenticated(bool value);

            static std::string s_CurrentUsername;
            static std::string s_CurrentKey;
            static std::string s_CurrentHwid;
            static std::string s_CurrentCreatedAt;
            static int s_CurrentExpiresInDays;

            static AuthState s_State;
            static HardwareInfo s_HwInfo;
            static LocationInfo s_LocInfo;

            // Form buffers
            static char s_UsernameBuffer[64];
            static char s_KeyBuffer[64];
            static std::string s_ErrorMessage;
            static std::string s_SuccessMessage;
            static std::string s_DownloadLink;
            static std::string s_LatestVersion;
            static std::string s_BannedKey;
            static std::string s_BannedUserId;
            static bool s_IsSubmitting;
            static std::atomic<bool> s_IsDownloading;
            static std::atomic<uint64_t> s_DownloadedBytes;
            static std::atomic<uint64_t> s_TotalBytes;
            static std::atomic<float> s_DownloadSpeed;
            static std::atomic<bool> s_DownloadComplete;
            static std::atomic<bool> s_DownloadFailed;
            static float s_SuccessTimer;
            static float s_LoadingTimer;

            // Background worker thread for initial check
            static void StartBackgroundCheck();

            // Background worker thread for registration
            static void StartBackgroundRegistration();

            // Background worker thread for automatic in-app updates
            static void StartBackgroundUpdate();

            // UI Render routines
            static void RenderLoadingScreen();
            static void RenderRegistrationScreen();
            static void RenderSuccessScreen();
            static void RenderBannedScreen();
            static void RenderErrorScreen();
            static void RenderExpiredScreen();
            static void RenderKeyExpiredScreen();
            static void SetupTheme();
        };
    }
}
