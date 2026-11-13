#include "../headers/config.h"
#include <utility>
#include "../../imgui.h"
#include <iostream>
#include <map>
#include <Main/Options.hpp>

void c_config::init_config()
{
    static bool init = false;

    if (!std::exchange(init, true))
    {
        // Silent Section
        add_option<checkbox_t>("Enable Silent", false);
        add_option<slider_int_t>("Silent Distance", 200, 0, 1000, "%d");
        add_option<color_checkbox_t>("Show Silent FOV", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<color_checkbox_t>("Visible Check", false, col_t{ 0.f, 1.f, 0.f, 1.f });
        add_option<slider_int_t>("Silent FOV", 70, 0, 300, "%d°");
        add_option<multi_dropdown_t>(
            "Silent Bone selection",
            bool_t{ true, true, true, true, true, true, true },
            string_t{ "Head", "Neck", "Chest", "LeftFoot", "RightFoot", "RightHand", "LeftHand" }
        );
        
        add_option<checkbox_t>("Enable Magic", false, false);
        add_option<slider_int_t>("Silent Thread Delay", 1, 0, 100, "%dms");

        add_option<checkbox_t>("Undetectable Silent", false);
        add_option<checkbox_t>("Perfect Silent", false);
        add_option<slider_float_t>("Miss Chance", 0.f, 0.f, 100.f, "%.1f%%");
        add_option<checkbox_t>("Silent Ignore Dead", false);
        add_option<checkbox_t>("Silent Ignore Invisible", false);
        add_option<checkbox_t>("Silent Ignore Friends", false);
        add_option<checkbox_t>("Silent Dynamic FOV", false);
        
        add_option<color_checkbox_t>("Show Silent Bone Line", false, col_t{ 0.f, 1.f, 0.f, 1.f });
        add_option<slider_float_t>("Silent Bone Line Thickness", 2.f, 0.5f, 10.f, "%.1f");

        add_option<checkbox_t>("Enable Legit Silent", false);
        add_option<slider_int_t>("Legit Silent X", 50, 0, 100, "%d%%");
        add_option<slider_int_t>("Legit Silent Y", 50, 0, 100, "%d%%");
        add_option<slider_int_t>("Legit Silent Variance", 10, 0, 100, "%d%%");
        add_option<checkbox_t>("Legit Silent Standalone", false);
        add_option<checkbox_t>("Legit Silent Override", false);


        // Aimbot Section
        add_option<checkbox_t>("Enable Aimbot", false, false);
        add_option<slider_int_t>("Aimbot Distance", 200, 0, 1000, "%d");
        add_option<color_checkbox_t>("Show Aimbot FOV", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<color_checkbox_t>("Aimbot Visible Check", false, col_t{ 0.f, 1.f, 0.f, 1.f });
        add_option<slider_int_t>("Aimbot FOV", 5, 0, 180, "%d°");
        add_option<multi_dropdown_t>(
            "Aimbot Bone selection",
            bool_t{ true, true, true, true, true, true, true },
            string_t{ "Head", "Neck", "Chest", "LeftFoot", "RightFoot", "RightHand", "LeftHand" }
        );
        
        add_option<slider_int_t>("Aimbot Thread Delay", 1, 0, 10, "%dms");

        add_option<slider_int_t>("Aimbot Smoothness", 10, 0, 100, "%d");
        add_option<checkbox_t>("Aimbot Ignore Dead", false);
        add_option<checkbox_t>("Aimbot Ignore Invisible", false);
        add_option<checkbox_t>("Aimbot Ignore Friends", false);
        
        add_option<color_checkbox_t>("Show Aimbot Bone Line", false, col_t{ 1.f, 0.f, 0.f, 1.f });
        add_option<slider_float_t>("Aimbot Bone Line Thickness", 2.f, 0.f, 10.f, "%.1f");

        // Triggerbot Section
        add_option<checkbox_t>("Enable Triggerbot", false, false);
        add_option<checkbox_t>("Triggerbot Key", false, false);
        add_option<slider_int_t>("Triggerbot Distance", 200, 0, 1000, "%d");
        add_option<color_checkbox_t>("Show Triggerbot FOV", false, col_t{ 1.f, 0.f, 0.f, 1.f });
        add_option<slider_int_t>("Triggerbot FOV", 70, 0, 300, "%d°");
        add_option<multi_dropdown_t>(
            "Triggerbot Bone selection",
            bool_t{ true, true, true, true, true, true, true },
            string_t{ "Head", "Neck", "Chest", "LeftFoot", "RightFoot", "RightHand", "LeftHand" }
        );
        
        add_option<slider_int_t>("Triggerbot Thread Delay", 5, 0, 100, "%dms");
        add_option<slider_int_t>("Triggerbot Delay", 50, 0, 500, "%dms");
        add_option<slider_float_t>("Crosshair Tolerance", 15.0f, 0.0f, 50.0f, "%.1f");
        
        add_option<checkbox_t>("Triggerbot Ignore Dead", true);
        add_option<checkbox_t>("Triggerbot Ignore Invisible", true);
        add_option<checkbox_t>("Triggerbot Ignore Friends", true);
        add_option<checkbox_t>("Triggerbot Ignore NPC", true);
        add_option<checkbox_t>("Triggerbot Only Visible", true);
        
        add_option<color_checkbox_t>("Show Triggerbot Bone Line", false, col_t{ 1.f, 0.f, 0.f, 1.f });
        add_option<slider_float_t>("Triggerbot Bone Line Thickness", 2.f, 0.0f, 10.f, "%.1f");

        // World Teleport Section
        
        // Peek Assist Location Section
        add_option<slider_float_t>("Peek Assist Location X", 0.0f, -10000.0f, 10000.0f, "%.2f");
        add_option<slider_float_t>("Peek Assist Location Y", 0.0f, -10000.0f, 10000.0f, "%.2f");
        add_option<slider_float_t>("Peek Assist Location Z", 0.0f, -1000.0f, 1000.0f, "%.2f");
        
        // Peek Assist Keybinds
        add_option<checkbox_t>("Save Peek Position Key", false, true);
        add_option<checkbox_t>("Teleport to Peek Position Key", false, true);
        add_option<checkbox_t>("Teleport to Waypoint Key", false, true);

        // Humanized Aimbot
        add_option<checkbox_t>("Enable Humanized Aimbot", false);
        add_option<slider_int_t>("Humanized Smoothness", 50, 0, 100, "%d%%");
        add_option<slider_int_t>("Humanized Velocity", 50, 0, 100, "%d%%");
        add_option<slider_int_t>("Humanized Variance", 10, 0, 100, "%d%%");
        add_option<checkbox_t>("Humanized Randomness", false);


        // ESP Section
        add_option<checkbox_t>("Enable ESP", false);

        add_option<color_checkbox_t>("Enable Box ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<slider_int_t>("Box Size", 10, 7, 15, "%d");
        add_option<dropdown_t>("Box Line Type", 0, string_t{ "Dashed", "Solid" });
        add_option<slider_float_t>("Box Line Thickness", 1.f, 0.1f, 5.f, "%.1f");
        add_option<dropdown_t>("Box Type", 0, string_t{ "Full Box", "Corner Box" });

        add_option<color_checkbox_t>("Enable Skeleton ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Skeleton Type", 0, string_t{ "Basic", "Complex" });
        add_option<dropdown_t>("Skeleton Line Type", 0, string_t{ "Dashed", "Solid" });
        add_option<slider_float_t>("Skeleton Line Thickness", 1.f, 0.1f, 5.f, "%.1f");

        add_option<color_checkbox_t>("Enable Head ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<slider_float_t>("Head Size", 15.f, 5.f, 25.f, "%.1f");
        add_option<slider_float_t>("Head Line Thickness", 1.f, 0.1f, 5.f, "%.1f");

        add_option<color_checkbox_t>("Enable Name ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Name Placement", 0, string_t{ "Bottom", "Top", "Left upper", "Right upper", "Left lower", "Right lower" });

        add_option<color_checkbox_t>("Enable ID ESP", false, col_t{ 1.f, 1.f, 0.f, 1.f });
        add_option<dropdown_t>("ID Placement", 0, string_t{ "Bottom", "Top", "Left upper", "Right upper", "Left lower", "Right lower" });

        add_option<color_checkbox_t>("Enable Distance ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Distance Placement", 0, string_t{ "Bottom", "Top", "Left upper", "Right upper", "Left lower", "Right lower" });

        add_option<color_checkbox_t>("Enable Weapon ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Weapon Placement", 0, string_t{ "Bottom", "Top", "Left upper", "Right upper", "Left lower", "Right lower" });

        add_option<color_checkbox_t>("Enable Line ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<slider_float_t>("Line Thickness", 1.f, 0.1f, 5.f, "%.1f");
        add_option<dropdown_t>("Line Placement", 0, string_t{ "Bottom", "Center", "Top" });

        add_option<color_checkbox_t>("Enable Offscreen ESP", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Offscreen Movement Type", 0, string_t{ "Static", "Dynamic" });
        add_option<slider_float_t>("Offscreen Size", 1.f, 0.1f, 5.f, "%.1f");
        add_option<slider_float_t>("Offscreen Range", 100.f, 0.f, 1000.f, "%.1f");
        add_option<slider_float_t>("Offscreen Center Threashold", 50.f, 0.f, 500.f, "%.1f");

        add_option<color_checkbox_t>("Enable Health Bar ESP", false, col_t{ 0.f, 1.f, 0.f, 1.f });
        add_option<dropdown_t>("Health Bar Placement", 0, string_t{ "Left", "Right", "Top", "Bottom" });
        add_option<color_checkbox_t>("Enable Health Text", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Health Text Placement", 0, string_t{ "Center", "Above", "Below", "Left", "Right" });

        add_option<color_checkbox_t>("Enable Armor Bar ESP", false, col_t{ 0.f, 0.450f, 1.f, 1.f });
        add_option<dropdown_t>("Armor Bar Placement", 0, string_t{ "Left", "Right", "Top", "Bottom" });
        add_option<color_checkbox_t>("Enable Armor Text", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Armor Text Placement", 0, string_t{ "Center", "Above", "Below", "Left", "Right" });
        


        // Tracer ESP Section
        add_option<color_checkbox_t>("Enable Tracer ESP", false, col_t{ 1.f, 1.f, 0.f, 1.f });
        add_option<slider_float_t>("Tracer Duration", 5.0f, 1.0f, 30.0f, "%.1fs");
        add_option<slider_float_t>("Tracer Thickness", 2.0f, 0.5f, 10.0f, "%.1f");

        add_option<slider_int_t>("ESP Distance", 250, 0, 1000, "%dm");
        add_option<checkbox_t>("ESP Show Local Player", false);
        add_option<checkbox_t>("ESP Ignore Invisible", false);
        add_option<checkbox_t>("ESP Ignore Dead", false);
        add_option<color_checkbox_t>("ESP Ignore Friends", false, col_t{ 0.0f, 0.8f, 1.0f, 1.0f });
        add_option<checkbox_t>("ESP Ignore NPC", true);

        add_option<checkbox_t>("Enable Vehicle ESP", false);
        add_option<slider_int_t>("Vehicle ESP Render Distance", 200, 0, 3000, "%d");
        add_option<color_checkbox_t>("Vehicle ESP Marker", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<color_checkbox_t>("Vehicle ESP Health", false, col_t{ 0.f, 1.f, 0.f, 1.f });
        add_option<color_checkbox_t>("Vehicle ESP Snapline", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<dropdown_t>("Vehicle ESP Snapline Placement", 0, string_t{ "Bottom", "Center", "Top" });
        add_option<color_checkbox_t>("Vehicle ESP Distance", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<color_checkbox_t>("Vehicle ESP Name", false, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<checkbox_t>("Vehicle ESP Lock", false);

        // Vehicle List Section
        add_option<checkbox_t>("Enable Vehicle List", true);
        add_option<slider_int_t>("Vehicle List Distance", 1000, 0, 1000, "%d");
        add_option<text_field_t>("Vehicle List Search", "");

        // Player List Section
        add_option<slider_int_t>("Player List Distance", 1000, 0, 1000, "%d");
        add_option<text_field_t>("Player List Search", "");


        // Misc Section
        add_option<checkbox_t>("Enable Semi Godmode", false, false);
        add_option<checkbox_t>("Enable God Mode", false, false);
        add_option<checkbox_t>("Set Health", false, false);
        add_option<slider_float_t>("Health Amount", 100.f, 0.f, 100.f, "%.1f");
        add_option<checkbox_t>("Set Armor", false, false);
        add_option<slider_float_t>("Armor Amount", 100.f, 0.f, 100.f, "%.1f");
        add_option<checkbox_t>("Enable Cam Fov", false, false);
        add_option<slider_float_t>("Cam Fov Amount", 60.f, 1.f, 100.f, "%.1f");
        add_option<checkbox_t>("Enable Absorption", false, false);
        add_option<slider_float_t>("Absorption Percent", 50.f, 0.f, 100.f, "%.1f");
        add_option<checkbox_t>("Damage Reducer on self", false, false);
        add_option<slider_float_t>("Damage Taken per Hit", 15.f, 5.f, 100.f, "%.0f HP");
        add_option<checkbox_t>("Fire miss on self", false, false);
        add_option<dropdown_t>("Fire Miss Type", 0, string_t{ "Full", "Partial" });
        add_option<dropdown_t>("Bullet Miss Mode", 0, string_t{ "Off", "Partial", "Full" });
        add_option<slider_int_t>("Hits Allowed per 50", 10, 1, 50, "%d / 50");
        add_option<checkbox_t>("Enable Freecam", false, false);
        add_option<slider_float_t>("Freecam Horizontal Speed", 5.f, 0.1f, 50.f, "%.1f");
        add_option<slider_float_t>("Freecam Vertical Speed", 5.f, 0.1f, 50.f, "%.1f");
        add_option<slider_float_t>("Freecam Speed", 5.f, 0.1f, 50.f, "%.1f");
        add_option<checkbox_t>("Freecam Teleport", false, false);
        add_option<checkbox_t>("Enable Noclip", false, false);
        add_option<slider_float_t>("Noclip Horizontal Speed", 5.f, 0.1f, 50.f, "%.1f");
        add_option<slider_float_t>("Noclip Vertical Speed", 5.f, 0.1f, 50.f, "%.1f");
        add_option<checkbox_t>("Enable No Collision", false, false);


        add_option<checkbox_t>("Enable No Recoil", false, false);
        add_option<slider_float_t>("No Recoil Value", 100.f, 0.f, 100.f, "%.1f");
        add_option<checkbox_t>("Enable No Reload", false, false);
        add_option<checkbox_t>("Enable No Spread", false, false);
        add_option<slider_float_t>("No Spread Value", 100.f, 0.f, 100.f, "%.1f");
        add_option<checkbox_t>("Enable Range Modify", false, false);
        add_option<slider_float_t>("Range Modify Value", 100.f, 0.f, 1000.f, "%.1f");
        add_option<checkbox_t>("Enable Damage Boost", false, false);
        add_option<slider_float_t>("Damage Boost Value", 100.f, 0.f, 100.f, "%.1f");
        add_option<checkbox_t>("Enable Infinite Ammo", false, false);
        add_option<checkbox_t>("Enable Safe Damage Boost", false, false);
        add_option<slider_float_t>("Safe Damage Boost Value", 100.f, 0.f, 100.f, "%.1f");


        add_option<checkbox_t>("Enable Vehicle Options", false, true);

        add_option<checkbox_t>("Enable Vehicle Speed", false);
        add_option<slider_float_t>("Vehicle Speed Amount", 2.f, 1.f, 50.f, "%.1fx");
        add_option<checkbox_t>("Enable Vehicle Brake", false);
        add_option<slider_float_t>("Vehicle Brake Amount", 2.f, 1.f, 30.f, "%.1fx");
        add_option<checkbox_t>("Enable Vehicle Fix", false, true);
        add_option<checkbox_t>("Repair Vehicle Key", false, true);
        add_option<checkbox_t>("Enable Vehicle Godmode", false, true);
        add_option<checkbox_t>("Enable Engine Fix", false, true);
        add_option<checkbox_t>("Enable Petrol Tank Fix", false, true);
        add_option<checkbox_t>("Enable Oil Tank Fix", false, true);
        add_option<checkbox_t>("Enable Water Tank Fix", false, true);
        add_option<checkbox_t>("Enable Never Explode", false, true);
        add_option<checkbox_t>("Unlock All Vehicles", false, false);
        add_option<slider_float_t>("Unlock Distance", 250.f, 20.f, 1000.f, "%.0fm");

        add_option<checkbox_t>("Enable Vehicle Color", false, true);
        add_option<checkbox_t>("Enable Primary Color", false, true);
        add_option<slider_int_t>("Primary Color R", 255, 0, 255, "%d");
        add_option<slider_int_t>("Primary Color G", 0, 0, 255, "%d");
        add_option<slider_int_t>("Primary Color B", 0, 0, 255, "%d");
        add_option<checkbox_t>("Enable Secondary Color", false, true);
        add_option<slider_int_t>("Secondary Color R", 0, 0, 255, "%d");
        add_option<slider_int_t>("Secondary Color G", 255, 0, 255, "%d");
        add_option<slider_int_t>("Secondary Color B", 0, 0, 255, "%d");


        // Settings Section
        add_option<color_checkbox_t>("Accent gui color", true, col_t{ 0.f, 0.95f, 1.f, 1.f });
        add_option<dropdown_t>("Interface scale", 1, string_t{ "75%", "100%", "125%", "150%", "200%" });
        add_option<checkbox_t>("Using animations", true);
        add_option<slider_float_t>("Animation speed", 1.f, 0.1f, 2.f, "%.1f");
        add_option<checkbox_t>("Block Input", false);
        add_option<checkbox_t>("Stream Proof", false);

        add_option<checkbox_t>("Spectator list", true);
        add_option<checkbox_t>("Watermark", false);
        add_option<checkbox_t>("Notifications", true);

        // Advance Section
        add_option<slider_float_t>("Radius", 50.0f, 10.0f, 250.0f, "%.0f m");

        // Radar Section
        add_option<checkbox_t>("Enable Radar", false);
        add_option<slider_int_t>("Radar Range", 100, 50, 500, "%d");
        add_option<slider_int_t>("Radar Size", 200, 100, 400, "%d");
        add_option<dropdown_t>("Radar Position", 0, string_t{ "Top Left", "Top Right", "Bottom Left", "Bottom Right" });
        add_option<checkbox_t>("Radar Show Players", true);
        add_option<checkbox_t>("Radar Show Vehicles", true);
        add_option<checkbox_t>("Radar Show Names", false);
        add_option<checkbox_t>("Radar Show Distance", false);
        add_option<checkbox_t>("Radar Show Direction", true);
        add_option<checkbox_t>("Radar Circle Style", false);
        add_option<checkbox_t>("Radar Show Waves", true);
        add_option<checkbox_t>("Radar Animate Waves", true);
        add_option<color_checkbox_t>("Radar Background Color", true, col_t{ 0.2f, 0.2f, 0.2f, 0.6f });
        add_option<color_checkbox_t>("Radar Border Color", true, col_t{ 0.5f, 0.5f, 0.5f, 0.0f });
        add_option<color_checkbox_t>("Radar Player Color", true, col_t{ 1.f, 1.f, 1.f, 1.f });
        add_option<color_checkbox_t>("Radar Enemy Color", true, col_t{ 1.f, 0.f, 0.f, 1.f });
        add_option<color_checkbox_t>("Radar Vehicle Color", true, col_t{ 0.5f, 0.8f, 1.f, 1.f });
        add_option<color_checkbox_t>("Radar Friend Color", true, col_t{ 0.f, 1.f, 0.f, 1.f });
    }
}


void process_single_value(checkbox_t& widget)
{
    auto& callback = widget.callback;
    auto& state = widget.state;
    static std::map<int, bool> key_states;

    if (state.mode == 0)
    {
        bool key_pressed = (GetAsyncKeyState(state.key) & 0x8000) != 0;
        bool was_pressed = key_states[state.key];
        
        if (key_pressed && !was_pressed)
        {
            callback = !callback;
        }
        
        key_states[state.key] = key_pressed;
    }
    else
    {
        callback = GetAsyncKeyState(state.key) & 0x8000;
    }
}

void c_config::process_keybinds()
{
    while (!g_Options.General.ShutDown) {
        for (auto& [name, variant] : options)
        {
            std::visit([this](auto&& widget)
                {
                    using T = std::decay_t<decltype(widget)>;

                    if constexpr (has_keybind<T>::value)
                    {
                        if (widget.keybind)
                        {
                            process_single_value(widget);
                        }
                    }
                }, variant);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
}
