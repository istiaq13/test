#include "esp.hpp"
#include <cmath>
#include <algorithm>
#include <string>
#include <Main/GameSDK/RobRun.hpp>
#include <Main/Options.hpp>
#include <Utils/Dependencies/ImGui/framework/headers/config.h>

namespace Cheat
{
    // ================================================================
    //  Draw helpers
    // ================================================================
    static void DrawBox(ImDrawList* dl, ImVec2 bMin, ImVec2 bMax,
                        ImU32 col, float thickness, int type)
    {
        if (type == 0) // Full box
        {
            dl->AddRect(bMin, bMax, IM_COL32(0, 0, 0, 180), 0.f, 0, thickness + 1.5f);
            dl->AddRect(bMin, bMax, col, 0.f, 0, thickness);
        }
        else // Corner box
        {
            float w  = bMax.x - bMin.x;
            float h  = bMax.y - bMin.y;
            float cw = w * 0.25f;
            float ch = h * 0.25f;

            auto DrawCorner = [&](ImVec2 origin, float dx, float dy)
            {
                dl->AddLine(origin, origin + ImVec2(dx, 0.f), IM_COL32(0,0,0,180), thickness + 1.5f);
                dl->AddLine(origin, origin + ImVec2(0.f, dy), IM_COL32(0,0,0,180), thickness + 1.5f);
                dl->AddLine(origin, origin + ImVec2(dx, 0.f), col, thickness);
                dl->AddLine(origin, origin + ImVec2(0.f, dy), col, thickness);
            };

            DrawCorner(bMin,                    cw,  ch);
            DrawCorner(ImVec2(bMax.x, bMin.y), -cw,  ch);
            DrawCorner(ImVec2(bMin.x, bMax.y),  cw, -ch);
            DrawCorner(bMax,                   -cw, -ch);
        }
    }

    static void DrawHealthBar(ImDrawList* dl, ImVec2 bMin, ImVec2 bMax,
                               int hp, int maxHp, ImU32 colFill, int placement)
    {
        float frac   = (maxHp > 0) ? (float)hp / (float)maxHp : 0.f;
        float barW   = 4.f;
        float barOff = 3.f;

        float xLeft  = bMin.x - barW - barOff;
        float xRight = bMax.x + barOff;
        float x      = (placement == 0) ? xLeft : xRight;
        float yTop   = bMin.y;
        float yBot   = bMax.y;
        float h      = yBot - yTop;

        dl->AddRectFilled(ImVec2(x, yTop), ImVec2(x + barW, yBot),
                          IM_COL32(30, 30, 30, 200), 2.f);
        float fillTop = yTop + h * (1.f - frac);
        dl->AddRectFilled(ImVec2(x, fillTop), ImVec2(x + barW, yBot), colFill, 2.f);
    }

    // ================================================================
    //  Debug status overlay — always visible when ESP is enabled
    //  Shows: syscall active, module base, entities found, matrix state
    // ================================================================
    static void DrawDebugOverlay(ImDrawList* dl, const ImVec2& screenSize,
                                 int entityCount, bool matrixValid,
                                 uint64_t moduleBase, uint64_t wpPtr)
    {
        float x = 10.f;
        float y = 10.f;
        float lineH = 15.f;
        ImU32 green  = IM_COL32(46, 204, 142, 255);
        ImU32 red    = IM_COL32(220, 60, 60, 255);
        ImU32 yellow = IM_COL32(240, 200, 0, 255);
        ImU32 shadow = IM_COL32(0, 0, 0, 200);

        auto DrawLine = [&](const char* text, ImU32 col)
        {
            dl->AddText(ImVec2(x + 1, y + 1), shadow, text);
            dl->AddText(ImVec2(x, y), col, text);
            y += lineH;
        };

        char buf[128];

        // Syscall status
        extern namespace SysCall { extern bool g_bInitialized; }
        // We read it via extern inline — just show module base instead
        snprintf(buf, sizeof(buf), "[RR] GameAssembly: %s (0x%llX)",
            moduleBase ? "FOUND" : "MISSING", (unsigned long long)moduleBase);
        DrawLine(buf, moduleBase ? green : red);

        snprintf(buf, sizeof(buf), "[RR] WorldEntities: %s (0x%llX)",
            wpPtr ? "FOUND" : "scanning...", (unsigned long long)wpPtr);
        DrawLine(buf, wpPtr ? green : yellow);

        snprintf(buf, sizeof(buf), "[RR] Entities: %d", entityCount);
        DrawLine(buf, entityCount > 0 ? green : yellow);

        snprintf(buf, sizeof(buf), "[RR] ViewMatrix: %s", matrixValid ? "OK" : "no matrix");
        DrawLine(buf, matrixValid ? green : red);
    }

    // ================================================================
    //  ESP::run — called on render thread every frame
    // ================================================================
    void ESP::run()
    {
        if (!Config.ESP.Options.Enable) return;
        if (!C_RobRun.IsInitialized()) return;

        // Update view matrix every frame (tries to find camera)
        C_RobRun.UpdateViewMatrix();

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        const ImVec2 screenSize = ImGui::GetIO().DisplaySize;

        // Snapshot entity list
        static std::vector<RREntity> s_Snap;
        {
            std::lock_guard<std::mutex> lk(C_RobRun.GetLock());
            s_Snap = C_RobRun.GetEntities();
        }

        // --- Always draw debug overlay ---
        DrawDebugOverlay(dl, screenSize,
            (int)s_Snap.size(),
            C_RobRun.IsViewMatrixValid(),
            C_RobRun.m_ModuleBase,
            C_RobRun.GetWorldEntitiesPtr());

        // If no entities at all, nothing to draw
        if (s_Snap.empty()) return;

        bool hasMatrix = C_RobRun.IsViewMatrixValid();

        for (auto& ent : s_Snap)
        {
            if (Config.ESP.Options.IgnoreDead   && ent.Dead)    continue;
            if (!Config.ESP.Options.ShowLocalPlayer && ent.IsLocal) continue;
            if (ent.Distance > Config.ESP.Options.ESPDistance &&
                Config.ESP.Options.ESPDistance > 0) continue;

            // Choose colour
            ImU32 espColor = (ent.IsFriend && !Config.ESP.Options.IgnoreFriends)
                ? Config.ESP.Options.FriendColorU32
                : IM_COL32(255, 255, 255, 255);

            // --- PATH A: WorldToScreen available ---
            if (hasMatrix)
            {
                Vector3D headVec = ent.WorldPos;
                headVec.y += 1.8f;

                ImVec2 screenFeet = C_RobRun.WorldToScreen(ent.WorldPos);
                ImVec2 screenHead = C_RobRun.WorldToScreen(headVec);

                bool feetOnScreen = C_RobRun.IsOnScreen(screenFeet);
                bool headOnScreen = C_RobRun.IsOnScreen(screenHead);

                if (!feetOnScreen && !headOnScreen)
                {
                    // Offscreen arrow
                    if (Config.ESP.OffscreenESP.Enable)
                    {
                        float dist = (std::max)((float)ent.Distance, 1.f);
                        if (dist <= Config.ESP.OffscreenESP.OffscreenRange || Config.ESP.OffscreenESP.OffscreenRange <= 0.f)
                        {
                            ImVec2 center(screenSize.x * 0.5f, screenSize.y * 0.5f);
                            ImVec2 dir = ImVec2(screenFeet.x - center.x, screenFeet.y - center.y);
                            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                            if (len > 0.f) { dir.x /= len; dir.y /= len; }
                            float r = (std::min)(screenSize.x, screenSize.y) * 0.44f;
                            ImVec2 arrowTip(center.x + dir.x * r, center.y + dir.y * r);
                            float sz = 10.f * Config.ESP.OffscreenESP.OffscreenSize;
                            ImVec2 p1 = arrowTip;
                            ImVec2 p2(arrowTip.x - dir.x * sz + dir.y * sz * 0.5f,
                                       arrowTip.y - dir.y * sz - dir.x * sz * 0.5f);
                            ImVec2 p3(arrowTip.x - dir.x * sz - dir.y * sz * 0.5f,
                                       arrowTip.y - dir.y * sz + dir.x * sz * 0.5f);
                            dl->AddTriangleFilled(p1, p2, p3, Config.ESP.OffscreenESP.OffscreenColorU32);
                        }
                    }
                    continue;
                }

                float boxH = std::abs(screenFeet.y - screenHead.y);
                if (boxH < 5.f) boxH = 5.f;
                float boxW = boxH * 0.45f;

                ImVec2 bMin(screenFeet.x - boxW * 0.5f, screenHead.y);
                ImVec2 bMax(screenFeet.x + boxW * 0.5f, screenFeet.y);

                if (Config.ESP.Box.Enable)
                    DrawBox(dl, bMin, bMax, espColor, Config.ESP.Box.BoxLineThickness, Config.ESP.Box.BoxType);

                if (Config.ESP.HeadESP.Enable)
                {
                    float headR = Config.ESP.HeadESP.HeadSize;
                    dl->AddCircle(screenHead, headR, IM_COL32(0,0,0,180), 20, Config.ESP.HeadESP.HeadLineThickness + 1.5f);
                    dl->AddCircle(screenHead, headR, Config.ESP.HeadESP.HeadColorU32, 20, Config.ESP.HeadESP.HeadLineThickness);
                }

                if (Config.ESP.NameESP.Enable)
                {
                    ImVec2 ts = ImGui::CalcTextSize(ent.Name.c_str());
                    ImVec2 namePos(bMin.x + (boxW - ts.x) * 0.5f, bMin.y - ts.y - 2.f);
                    dl->AddText(namePos + ImVec2(1.f, 1.f), IM_COL32(0,0,0,200), ent.Name.c_str());
                    dl->AddText(namePos, Config.ESP.NameESP.NameColorU32, ent.Name.c_str());
                }

                if (Config.ESP.DistanceESP.Enable)
                {
                    std::string distStr = std::to_string(ent.Distance) + "m";
                    ImVec2 ts = ImGui::CalcTextSize(distStr.c_str());
                    ImVec2 dPos(bMin.x + (boxW - ts.x) * 0.5f, bMax.y + 2.f);
                    dl->AddText(dPos + ImVec2(1.f, 1.f), IM_COL32(0,0,0,200), distStr.c_str());
                    dl->AddText(dPos, Config.ESP.DistanceESP.DistanceColorU32, distStr.c_str());
                }

                if (Config.ESP.HealthBarESP.Enable)
                    DrawHealthBar(dl, bMin, bMax, ent.Health, ent.MaxHealth,
                                  Config.ESP.HealthBarESP.HealthBarColorU32,
                                  Config.ESP.HealthBarESP.HealthBarPlacement);

                if (Config.ESP.LineESP.Enable)
                {
                    ImVec2 origin;
                    switch (Config.ESP.LineESP.LinePlacement)
                    {
                    case 0:  origin = ImVec2(screenSize.x * 0.5f, screenSize.y);       break;
                    case 1:  origin = ImVec2(screenSize.x * 0.5f, 0.f);               break;
                    default: origin = ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f); break;
                    }
                    dl->AddLine(origin, screenFeet, IM_COL32(0,0,0,120), Config.ESP.LineESP.LineThickness + 1.5f);
                    dl->AddLine(origin, screenFeet, Config.ESP.LineESP.LineColorU32, Config.ESP.LineESP.LineThickness);
                }
            }
            else
            {
                // --- PATH B: No view matrix — text-only radar ESP ---
                // Show distance + name stacked on the right side of the screen
                // This always works regardless of camera state
                static float yOffset = 80.f;
                if (&ent == &s_Snap[0]) yOffset = 80.f; // reset each frame

                char buf[128];
                snprintf(buf, sizeof(buf), "[%s]  %dm  HP:%d",
                    ent.Name.c_str(), ent.Distance, ent.Health);

                float textX = screenSize.x - 220.f;
                float textY = yOffset;

                // Background
                ImVec2 tSize = ImGui::CalcTextSize(buf);
                dl->AddRectFilled(
                    ImVec2(textX - 4, textY - 2),
                    ImVec2(textX + tSize.x + 4, textY + tSize.y + 2),
                    IM_COL32(0, 0, 0, 160), 3.f);

                // Text (sea green for friends, white for enemies)
                ImU32 textCol = ent.IsFriend
                    ? IM_COL32(46, 204, 142, 255)
                    : IM_COL32(255, 255, 255, 220);

                dl->AddText(ImVec2(textX, textY), textCol, buf);
                yOffset += tSize.y + 4.f;
            }
        }
    }

} // namespace Cheat
