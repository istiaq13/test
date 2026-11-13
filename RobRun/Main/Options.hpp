#pragma once
#include <Windows.h>
#include <string>
#include <Utils/Dependencies/ImGui/imgui.h>
#include <Utils/Dependencies/ImGui/framework/headers/config.h>
#include <chrono>
#include <thread>
#include <iostream>

namespace Cheat
{
    inline bool g_IsBindingKey = false;

    class Options
    {
    public:
        struct General
        {
            bool  ShutDown  = false;
            int   MenuKey   = VK_RSHIFT;
            float ThreadDelay = 0;
            bool  StreamProof = false;
            bool  BlockInput  = false;
        } General;
    };
}

inline Cheat::Options g_Options;

// ================================================================
//  Configuration — ESP only (visual features for ROB RUN v1)
// ================================================================
namespace Cheat
{
    class Configuration
    {
    public:
        struct ESP
        {
            struct Options
            {
                bool Enable         = false;
                int  ESPDistance    = 300;
                bool IgnoreDead     = true;
                bool IgnoreFriends  = false;
                bool ShowLocalPlayer = false;
                ImVec4 FriendColor  = ImVec4(0.18f, 0.80f, 0.60f, 1.0f); // sea green
                ImU32  FriendColorU32 = IM_COL32(46, 204, 142, 255);
            } Options;

            struct Box
            {
                bool Enable             = false;
                int  BoxType            = 0; // 0=Full, 1=Corner
                float BoxLineThickness  = 1.5f;
                ImVec4 BoxColor         = ImVec4(1.f, 1.f, 1.f, 1.f);
                ImU32  BoxColorU32      = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            } Box;

            struct Skeleton
            {
                bool Enable                 = false;
                float SkeletonLineThickness = 1.f;
                ImVec4 SkeletonColor        = ImVec4(1.f, 1.f, 1.f, 1.f);
                ImU32  SkeletonColorU32     = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            } Skeleton;

            struct HeadESP
            {
                bool Enable             = false;
                float HeadSize          = 10.f;
                float HeadLineThickness = 1.f;
                ImVec4 HeadColor        = ImVec4(1.f, 1.f, 1.f, 1.f);
                ImU32  HeadColorU32     = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            } HeadESP;

            struct NameESP
            {
                bool Enable         = false;
                int  NamePlacement  = 0;
                ImVec4 NameColor    = ImVec4(1.f, 1.f, 1.f, 1.f);
                ImU32  NameColorU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            } NameESP;

            struct DistanceESP
            {
                bool Enable             = false;
                int  DistancePlacement  = 0;
                ImVec4 DistanceColor    = ImVec4(0.8f, 0.8f, 0.8f, 1.f);
                ImU32  DistanceColorU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(0.8f, 0.8f, 0.8f, 1.f));
            } DistanceESP;

            struct HealthBarESP
            {
                bool Enable                 = false;
                int  HealthBarPlacement     = 0;
                ImVec4 HealthBarColor       = ImVec4(0.18f, 0.80f, 0.60f, 1.f); // sea green
                ImU32  HealthBarColorU32    = IM_COL32(46, 204, 142, 255);

                bool HealthBarTextEnable    = false;
                ImVec4 HealthBarTextColor   = ImVec4(1.f, 1.f, 1.f, 1.f);
                ImU32  HealthBarTextColorU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            } HealthBarESP;

            struct LineESP
            {
                bool Enable         = false;
                int  LinePlacement  = 0;
                float LineThickness = 1.f;
                ImVec4 LineColor    = ImVec4(1.f, 1.f, 1.f, 1.f);
                ImU32  LineColorU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            } LineESP;

            struct OffscreenESP
            {
                bool Enable                     = false;
                float OffscreenSize             = 1.f;
                float OffscreenRange            = 100.f;
                ImVec4 OffscreenColor           = ImVec4(0.18f, 0.80f, 0.60f, 1.f);
                ImU32  OffscreenColorU32        = IM_COL32(46, 204, 142, 255);
            } OffscreenESP;

        } ESP;

        // Radar (optional visual)
        struct Radar
        {
            bool  enable        = false;
            int   range         = 100;
            int   size          = 200;
            int   position      = 0;
            float customPosX    = 20.0f;
            float customPosY    = 20.0f;
            bool  useCustomPos  = false;
            bool  isDragging    = false;
            bool  showPlayers   = true;
            bool  showNames     = false;
            bool  showDistance  = false;
            bool  showDirection = true;
            bool  circleStyle   = false;
            bool  showWaves     = true;
            bool  animateWaves  = true;
            float backgroundColor[4] = { 0.08f, 0.15f, 0.12f, 0.80f };
            float borderColor[4]     = { 0.18f, 0.80f, 0.60f, 0.60f };
            float playerColor[4]     = { 0.18f, 0.80f, 0.60f, 1.f };
            float enemyColor[4]      = { 1.f, 0.3f, 0.3f, 1.f };
            float friendColor[4]     = { 0.18f, 0.80f, 0.60f, 1.f };
        } Radar;
    };
}

inline Cheat::Configuration Config;

namespace Cheat
{
    // Forward declarations
    void SetConfiguration();

    inline void StartConfiguration()
    {
        if (cfg) cfg->init_config();
        while (!cfg)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        while (!g_Options.General.ShutDown)
        {
            SetConfiguration();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
}

