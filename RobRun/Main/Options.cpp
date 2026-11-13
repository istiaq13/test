#include "Options.hpp"
#include <Utils/Dependencies/ImGui/framework/headers/config.h>
#include <Utils/Dependencies/ImGui/framework/headers/widgets.h>
#include <chrono>
#include <thread>
#include <iostream>

namespace Cheat
{
    void SetConfiguration()
    {
        // ── ESP ──────────────────────────────────────────────────────────
        Config.ESP.Options.Enable = cfg->get<checkbox_t>("Enable ESP").callback;

        if (Config.ESP.Options.Enable)
        {
            Config.ESP.Options.ESPDistance     = cfg->get<slider_int_t>("ESP Distance").callback;
            Config.ESP.Options.IgnoreDead      = cfg->get<checkbox_t>("ESP Ignore Dead").callback;
            Config.ESP.Options.ShowLocalPlayer = cfg->get<checkbox_t>("ESP Show Local Player").callback;

            // Friend color & ignore friends
            auto friendOpt = cfg->get<color_checkbox_t>("ESP Ignore Friends");
            Config.ESP.Options.IgnoreFriends  = friendOpt.callback;
            Config.ESP.Options.FriendColor    = ImVec4(friendOpt.color[0], friendOpt.color[1], friendOpt.color[2], 1.f);
            Config.ESP.Options.FriendColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.Options.FriendColor);

            // Box ESP
            Config.ESP.Box.Enable = cfg->get<color_checkbox_t>("Enable Box ESP").callback;
            if (Config.ESP.Box.Enable)
            {
                auto boxCFG = cfg->get<color_checkbox_t>("Enable Box ESP");
                Config.ESP.Box.BoxColor    = ImVec4(boxCFG.color[0], boxCFG.color[1], boxCFG.color[2], 1.f);
                Config.ESP.Box.BoxColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.Box.BoxColor);
            }

            // Name ESP
            Config.ESP.NameESP.Enable = cfg->get<color_checkbox_t>("Enable Name ESP").callback;
            if (Config.ESP.NameESP.Enable)
            {
                auto nameCFG = cfg->get<color_checkbox_t>("Enable Name ESP");
                Config.ESP.NameESP.NameColor    = ImVec4(nameCFG.color[0], nameCFG.color[1], nameCFG.color[2], 1.f);
                Config.ESP.NameESP.NameColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.NameESP.NameColor);
            }

            // Distance ESP
            Config.ESP.DistanceESP.Enable = cfg->get<color_checkbox_t>("Enable Distance ESP").callback;
            if (Config.ESP.DistanceESP.Enable)
            {
                auto distCFG = cfg->get<color_checkbox_t>("Enable Distance ESP");
                Config.ESP.DistanceESP.DistanceColor    = ImVec4(distCFG.color[0], distCFG.color[1], distCFG.color[2], 1.f);
                Config.ESP.DistanceESP.DistanceColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.DistanceESP.DistanceColor);
            }

            // Health Bar ESP
            Config.ESP.HealthBarESP.Enable = cfg->get<color_checkbox_t>("Enable Health Bar ESP").callback;
            if (Config.ESP.HealthBarESP.Enable)
            {
                auto hpCFG = cfg->get<color_checkbox_t>("Enable Health Bar ESP");
                Config.ESP.HealthBarESP.HealthBarColor    = ImVec4(hpCFG.color[0], hpCFG.color[1], hpCFG.color[2], 1.f);
                Config.ESP.HealthBarESP.HealthBarColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.HealthBarESP.HealthBarColor);
            }

            // Head Circle
            Config.ESP.HeadESP.Enable = cfg->get<color_checkbox_t>("Enable Head ESP").callback;
            if (Config.ESP.HeadESP.Enable)
            {
                auto hcCFG = cfg->get<color_checkbox_t>("Enable Head ESP");
                Config.ESP.HeadESP.HeadColor    = ImVec4(hcCFG.color[0], hcCFG.color[1], hcCFG.color[2], 1.f);
                Config.ESP.HeadESP.HeadColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.HeadESP.HeadColor);
            }

            // Snapline
            Config.ESP.LineESP.Enable = cfg->get<color_checkbox_t>("Enable Line ESP").callback;
            if (Config.ESP.LineESP.Enable)
            {
                auto lnCFG = cfg->get<color_checkbox_t>("Enable Line ESP");
                Config.ESP.LineESP.LineColor    = ImVec4(lnCFG.color[0], lnCFG.color[1], lnCFG.color[2], 1.f);
                Config.ESP.LineESP.LineColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.LineESP.LineColor);
            }

            // Offscreen ESP
            Config.ESP.OffscreenESP.Enable = cfg->get<color_checkbox_t>("Enable Offscreen ESP").callback;
            if (Config.ESP.OffscreenESP.Enable)
            {
                auto ofCFG = cfg->get<color_checkbox_t>("Enable Offscreen ESP");
                Config.ESP.OffscreenESP.OffscreenColor    = ImVec4(ofCFG.color[0], ofCFG.color[1], ofCFG.color[2], 1.f);
                Config.ESP.OffscreenESP.OffscreenColorU32 = ImGui::ColorConvertFloat4ToU32(Config.ESP.OffscreenESP.OffscreenColor);
            }
        }
    }
}

