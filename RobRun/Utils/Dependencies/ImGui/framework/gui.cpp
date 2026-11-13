#include "headers/includes.h"
#include "headers/functions.h"
#include "headers/widgets.h"
#include "headers/config.h"
#include "../../../../Main/Cheat.hpp"
#include "../../../../Main/Auth/AuthWindow.hpp"
#include "../../../../Main/Auth/AuthClient.hpp"
#include "../../../../Main/Auth/VoiceRecorder.hpp"
#include "../../../../Main/Auth/RealtimeClient.hpp"
#include <mutex>
#include <thread>
#include <cmath>
#include <unordered_set>

// Sea-green accent: #2ECC8E = IM_COL32(46, 204, 142, 255)

static char s_NewConfigName[64] = "";
static std::atomic<bool> s_IsAddingConfig(false);
static int s_PopupCloseCooldown = 0;

static ImVec4 HexToColor(const std::string& hex,
                          const ImVec4& defaultCol = ImVec4(0.18f, 0.80f, 0.60f, 1.0f))
{
    if (hex.empty()) return defaultCol;
    std::string s = hex;
    if (s[0] == '#') s = s.substr(1);
    if (s.length() == 6)
    {
        unsigned int r = 0, g = 0, b = 0;
        if (sscanf_s(s.c_str(), "%02x%02x%02x", &r, &g, &b) == 3)
            return ImVec4(r / 255.f, g / 255.f, b / 255.f, 1.0f);
    }
    return defaultCol;
}

static void ApplyConfigValues(const Cheat::Auth::UserConfigItem& res)
{
    cfg->get<checkbox_t>("Enable ESP").callback = res.esp_enabled;
    Config.ESP.Options.Enable = res.esp_enabled;

    cfg->get<color_checkbox_t>("Enable Name ESP").callback = res.esp_name;
    Config.ESP.NameESP.Enable = res.esp_name;

    cfg->get<color_checkbox_t>("Enable Distance ESP").callback = res.distance;
    Config.ESP.DistanceESP.Enable = res.distance;

    cfg->get<slider_int_t>("ESP Distance").callback = res.distance_value;
    Config.ESP.Options.ESPDistance = res.distance_value;

    cfg->get<color_checkbox_t>("Enable Health Bar ESP").callback = res.health_bar;
    Config.ESP.HealthBarESP.Enable = res.health_bar;

    if (!res.friend_color.empty())
    {
        ImVec4 fCol = HexToColor(res.friend_color);
        auto& friendOpt = cfg->get<color_checkbox_t>("ESP Ignore Friends");
        friendOpt.color[0] = fCol.x; friendOpt.color[1] = fCol.y;
        friendOpt.color[2] = fCol.z; friendOpt.color[3] = fCol.w;
        Config.ESP.Options.FriendColor    = fCol;
        Config.ESP.Options.FriendColorU32 = ImGui::ColorConvertFloat4ToU32(fCol);
    }

    // Friend list keyed by game ID
    Cheat::C_RobRun.FriendIdList.clear();
    for (const auto& fi : res.friends_list)
    {
        if (fi.empty()) continue;
        try {
            if (std::all_of(fi.begin(), fi.end(), ::isdigit))
                Cheat::C_RobRun.FriendIdList[std::stoi(fi)] = true;
        } catch (...) {}
    }

    Cheat::Auth::ConfigCache::SetActiveConfig(res.config_name, res.config_key);
}

bool Initialized = false;

bool c_gui::is_popup_open() { return false; }

void c_gui::watermarks() {}

void c_gui::render()
{
    if (!Initialized) { gui->initialize(); Initialized = true; }

    if (s_PopupCloseCooldown > 0)
    {
        s_PopupCloseCooldown--;
        ImGuiIO& io = ImGui::GetIO();
        io.MouseClicked[0] = false;
        io.MouseDown[0] = false;
    }

    gui->set_next_window_size(SCALE(var->window.size));
    gui->begin("menu", nullptr, var->window.flags);
    {
        gui->set_style();
        gui->draw_decorations();

        // ── Sidebar ──────────────────────────────────────────────────
        {
            gui->begin_content("sections",
                SCALE(var->gui.sidebar_width, 0),
                SCALE(elements->section.padding),
                SCALE(elements->section.spacing));
            {
                int index = 0;
                int total = static_cast<int>(var->gui.sections_data.second.size());
                float avail_h = gui->window_height() - SCALE(var->gui.logo_height);
                float spacing = SCALE(8.f);
                float sec_h   = (avail_h - (spacing * (total - 1)) - SCALE(16.f)) / total;
                float start_y = SCALE(var->gui.logo_height + 6.f);

                for (const auto& section : var->gui.sections_data.second)
                {
                    gui->set_pos(start_y + (index * (sec_h + spacing)), pos_y);
                    widgets->section(section, index, (int&)var->gui.sections_data.first);
                    index++;
                }
                gui->easing(var->gui.section_alpha,
                    var->gui.active_section == var->gui.sections_data.first ? 1.f : 0.f,
                    8.f, static_easing);
                if (var->gui.section_alpha == 0.f)
                    var->gui.active_section = var->gui.sections_data.first;
            }
            gui->end_content();
            gui->sameline();
        }

        // ── Main area ────────────────────────────────────────────────
        {
            gui->set_pos(SCALE(var->gui.padding), pos_y);
            gui->begin_group();

            gui->begin_content("topbar",
                ImVec2(gui->content_avail().x - SCALE(var->gui.padding), SCALE(var->gui.topbar_height) + 1),
                SCALE(elements->content.padding), SCALE(elements->content.spacing));
            {
                std::string title = var->gui.section_name.at((int)var->gui.active_section);
                draw->text_clipped(gui->window_drawlist(),
                    font->get(inter_semibold_data, 13),
                    gui->window_pos() + SCALE(elements->content.padding.x, 0),
                    gui->window_pos() + gui->window_size(),
                    draw->get_clr(clr->widgets.text, var->gui.section_alpha),
                    title.c_str(), NULL, NULL, ImVec2(0.f, 0.5f));
                gui->easing(var->gui.sub_section_alpha, 1.f, 8.f, static_easing);
                var->gui.active_sub_section.at((int)var->gui.active_section) = 0;
                var->gui.sub_section_data.at((int)var->gui.active_section) = 0;
            }
            gui->end_content();

            static int s_LastSection = -1;
            int cur_section = (int)var->gui.active_section;
            bool section_changed = (cur_section != s_LastSection);
            if (section_changed) s_LastSection = cur_section;

            std::string cid = "content_" + std::to_string(cur_section);
            gui->push_var(style_var_alpha, var->gui.section_alpha * var->gui.sub_section_alpha);
            gui->begin_content(cid.c_str(),
                gui->content_avail() - SCALE(var->gui.padding, var->gui.padding) - ImVec2(0, 1),
                SCALE(elements->content.padding), SCALE(elements->content.spacing));

            if (section_changed) ImGui::SetScrollY(0.f);

            // ========================================================
            // SECTION 0: VISUALS / ESP
            // ========================================================
            if (var->gui.active_section == section_id::visuals)
            {
                widgets->begin_child_ex("PLAYER ESP", -1.f, true);
                {
                    widgets->checkbox("Enable ESP");
                    widgets->slider_int("ESP Distance");
                    widgets->color_checkbox("Enable Box ESP",        "Box ESP");
                    widgets->color_checkbox("Enable Name ESP",       "Name ESP");
                    widgets->color_checkbox("Enable Distance ESP",   "Distance ESP");
                    widgets->color_checkbox("Enable Health Bar ESP", "Health Bar");
                    widgets->color_checkbox("Enable Head ESP",       "Head Circle");
                    widgets->color_checkbox("Enable Line ESP",       "Snapline");
                    widgets->color_checkbox("Enable Offscreen ESP",  "Offscreen ESP");
                    widgets->color_checkbox("ESP Ignore Friends",    "Ignore Friends");
                    widgets->checkbox("ESP Show Local Player",       "Show Local Player");
                    widgets->checkbox("ESP Ignore Dead",             "Ignore Dead Players");
                }
                widgets->end_child();
            }

            // ========================================================
            // SECTION 1: SETTINGS / CONFIG / ACCOUNT
            // ========================================================
            else if (var->gui.active_section == section_id::settings)
            {
                widgets->begin_child_ex("CONFIG", -1.f, true);
                {
                    std::string active_name, active_key;
                    Cheat::Auth::ConfigCache::GetActiveConfig(active_name, active_key);
                    if (active_name.empty()) active_name = "No config loaded";

                    ImGui::TextDisabled("Active Config:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.18f, 0.80f, 0.60f, 1.f), "%s", active_name.c_str());
                    ImGui::Spacing();

                    if (widgets->button("Load Config")->value_changed && !active_key.empty())
                    {
                        auto res = Cheat::Auth::AuthClient::GetConfig(active_key);
                        if (res.success)
                        {
                            Cheat::Auth::UserConfigItem item;
                            item.config_key = res.config_key;
                            item.config_name = res.config_name;
                            item.esp_enabled = res.esp_enabled;
                            item.esp_name = res.esp_name;
                            item.distance = res.distance;
                            item.distance_value = res.distance_value;
                            item.health_bar = res.health_bar;
                            item.friend_color = res.friend_color;
                            item.friends_list = res.friends_list;
                            ApplyConfigValues(item);
                        }
                    }

                    widgets->config_field("New Config Name", s_NewConfigName, sizeof(s_NewConfigName));
                    if (widgets->button("Create Config")->value_changed)
                    {
                        if (strlen(s_NewConfigName) > 0)
                        {
                            Cheat::Auth::CreateConfigPayload payload;
                            payload.config_name = s_NewConfigName;
                            payload.created_by  = Cheat::Auth::AuthWindow::GetCurrentUsername();
                            payload.esp_enabled = Config.ESP.Options.Enable;
                            payload.esp_name    = Config.ESP.NameESP.Enable;
                            payload.distance    = Config.ESP.DistanceESP.Enable;
                            payload.distance_value = Config.ESP.Options.ESPDistance;
                            payload.health_bar  = Config.ESP.HealthBarESP.Enable;
                            s_IsAddingConfig = true;
                            std::thread([payload]{
                                Cheat::Auth::AuthClient::CreateConfig(payload);
                                s_IsAddingConfig = false;
                            }).detach();
                            memset(s_NewConfigName, 0, sizeof(s_NewConfigName));
                        }
                    }

                    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
                    std::string username = Cheat::Auth::AuthWindow::GetCurrentUsername();
                    ImGui::TextDisabled("Logged in as:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.18f, 0.80f, 0.60f, 1.f), "%s", username.c_str());
                    if (widgets->button("Logout")->value_changed)
                        g_Options.General.ShutDown = true;
                }
                widgets->end_child();
            }

            gui->end_content();
            gui->pop_var();
            gui->end_group();
        }
    }
    gui->end();
}
