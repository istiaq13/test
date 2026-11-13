#include "../headers/functions.h"
#include "../headers/widgets.h"
#include <Windows.h>
#include <Main/Options.hpp>

const char* const keys[] = { "Tab", "Left", "Right", "Up", "Down", "Page Up", "Page Down", "Home", "End", "Insert", "Delete", "Backspace", "Space", "Enter", "Escape", "Ctrl", "Shift", "Alt", "Super", "Ctrl", "Shift", "Alt", "Super", "Menu", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "Apostrophe", "Comma", "Minus", "Period", "Slash", "Semicolon", "Equal", "Left Bracket", "Backslash", "Right Bracket", "Grave Accent", "Caps Lock", "Scroll Lock", "Num Lock", "Print Screen", "Pause", "Keypad 0", "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 4", "Keypad 5", "Keypad 6", "Keypad 7", "Keypad 8", "Keypad 9", "Keypad .", "Keypad /", "Keypad *", "Keypad -", "Keypad +", "Keypad Enter", "Keypad =", "App Back", "App Forward", "None", "Gamepad Back", "Gamepad Face Left", "Gamepad Face Right", "Gamepad Face Up", "Gamepad Face Down", "Gamepad Dpad Left", "Gamepad Dpad Right", "Gamepad Dpad Up", "Gamepad Dpad Down", "Gamepad L1", "Gamepad R1", "Gamepad L2", "Gamepad R2", "Gamepad L3", "Gamepad R3", "Gamepad L Stick Left", "Gamepad L Stick Right", "Gamepad L Stick Up", "Gamepad L Stick Down", "Gamepad R Stick Left", "Gamepad R Stick Right", "Gamepad R Stick Up", "Gamepad R Stick Down", "Mouse 1", "Mouse 2", "Mouse 3", "Mouse 4", "Mouse 5", "Mouse Wheel X", "Mouse Wheel Y", "Ctrl", "Shift", "Alt", "Super" };

const char* get_key_name(ImGuiKey key)
{
    if (key == ImGuiKey_None)
        return "-";

    return keys[key - ImGuiKey_NamedKey_BEGIN];
}

// widget_t c_widgets::key_select(std::string_view name, int* key)
// {
//     struct anim_t
//     {
//         bool active{ false };
//         float alpha{ 0 };
//     };

//     widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, SCALE(elements->keybind.button_height)));
//     anim_t* anim = gui->anim_container<anim_t>(data->id);
//     std::string buf_display = anim->active ? "-" : get_key_name((ImGuiKey)*key);

//     if (data->hovered && gui->mouse_released(mouse_button_left) && gui->is_window_hovered())
//         anim->active = true;

//     if (anim->active)
//     {
//         for (int i = 0; i < ImGuiKey_COUNT; ++i)
//         {

//             if (IsKeyPressed(ImGuiKey_Escape))
//             {
//                 *key = 0;
//                 anim->active = false;
//             }

//             if (IsKeyPressed((ImGuiKey)i))
//             {
//                 *key = i;
//                 anim->active = false;
//             }
//         }
//     }

//     gui->easing(anim->alpha, anim->active ? 1.f : 0.f, 8.f, static_easing);

//     draw->rect_filled(gui->window_drawlist(), data->rect.Min, data->rect.Max, draw->get_clr(clr->popup.sub_background), SCALE(elements->keybind.button_rounding));
//     draw->rect_filled(gui->window_drawlist(), data->rect.Min, data->rect.Max, draw->get_clr(clr->accent, 0.2f * anim->alpha), SCALE(elements->keybind.button_rounding));

//     draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.text), buf_display.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

//     return data;
// }

int keyIndexToVK(int index)
{
    switch(index)
    {
        // Basic keys
        case 0: return VK_TAB;
        case 1: return VK_LEFT;
        case 2: return VK_RIGHT;
        case 3: return VK_UP;
        case 4: return VK_DOWN;
        case 5: return VK_PRIOR; // Page Up
        case 6: return VK_NEXT;  // Page Down
        case 7: return VK_HOME;
        case 8: return VK_END;
        case 9: return VK_INSERT;
        case 10: return VK_DELETE;
        case 11: return VK_BACK;
        case 12: return VK_SPACE;
        case 13: return VK_RETURN;
        case 14: return VK_ESCAPE;
        case 15: return VK_CONTROL;   // Ctrl
        case 16: return VK_SHIFT;
        case 17: return VK_MENU;      // Alt
        case 18: return VK_LWIN;      // Super
        case 19: return VK_CONTROL;
        case 20: return VK_SHIFT;
        case 21: return VK_MENU;
        case 22: return VK_LWIN;
        case 23: return VK_APPS;      // Menu
        // Numbers
        case 24: return '0';
        case 25: return '1';
        case 26: return '2';
        case 27: return '3';
        case 28: return '4';
        case 29: return '5';
        case 30: return '6';
        case 31: return '7';
        case 32: return '8';
        case 33: return '9';
        // Letters
        case 34: return 'A';
        case 35: return 'B';
        case 36: return 'C';
        case 37: return 'D';
        case 38: return 'E';
        case 39: return 'F';
        case 40: return 'G';
        case 41: return 'H';
        case 42: return 'I';
        case 43: return 'J';
        case 44: return 'K';
        case 45: return 'L';
        case 46: return 'M';
        case 47: return 'N';
        case 48: return 'O';
        case 49: return 'P';
        case 50: return 'Q';
        case 51: return 'R';
        case 52: return 'S';
        case 53: return 'T';
        case 54: return 'U';
        case 55: return 'V';
        case 56: return 'W';
        case 57: return 'X';
        case 58: return 'Y';
        case 59: return 'Z';
        // Function keys
        case 60: return VK_F1;
        case 61: return VK_F2;
        case 62: return VK_F3;
        case 63: return VK_F4;
        case 64: return VK_F5;
        case 65: return VK_F6;
        case 66: return VK_F7;
        case 67: return VK_F8;
        case 68: return VK_F9;
        case 69: return VK_F10;
        case 70: return VK_F11;
        case 71: return VK_F12;
        case 72: return VK_F13;
        case 73: return VK_F14;
        case 74: return VK_F15;
        case 75: return VK_F16;
        case 76: return VK_F17;
        case 77: return VK_F18;
        case 78: return VK_F19;
        case 79: return VK_F20;
        case 80: return VK_F21;
        case 81: return VK_F22;
        case 82: return VK_F23;
        case 83: return VK_F24;
        // Symbols & punctuation
        case 84: return VK_OEM_7;        // Apostrophe
        case 85: return VK_OEM_COMMA;    // Comma
        case 86: return VK_OEM_MINUS;    // Minus
        case 87: return VK_OEM_PERIOD;   // Period
        case 88: return VK_OEM_2;        // Slash
        case 89: return VK_OEM_1;        // Semicolon
        case 90: return VK_OEM_PLUS;     // Equal
        case 91: return VK_OEM_4;        // Left Bracket
        case 92: return VK_OEM_5;        // Backslash
        case 93: return VK_OEM_6;        // Right Bracket
        case 94: return VK_OEM_3;        // Grave Accent
        // Locks
        case 95: return VK_CAPITAL;      // Caps Lock
        case 96: return VK_SCROLL;       // Scroll Lock
        case 97: return VK_NUMLOCK;      // Num Lock
        case 98: return VK_SNAPSHOT;     // Print Screen
        case 99: return VK_PAUSE;        // Pause
        // Keypad
        case 100: return VK_NUMPAD0;
        case 101: return VK_NUMPAD1;
        case 102: return VK_NUMPAD2;
        case 103: return VK_NUMPAD3;
        case 104: return VK_NUMPAD4;
        case 105: return VK_NUMPAD5;
        case 106: return VK_NUMPAD6;
        case 107: return VK_NUMPAD7;
        case 108: return VK_NUMPAD8;
        case 109: return VK_NUMPAD9;
        case 110: return VK_DECIMAL;     // Keypad .
        case 111: return VK_DIVIDE;      // Keypad /
        case 112: return VK_MULTIPLY;    // Keypad *
        case 113: return VK_SUBTRACT;    // Keypad -
        case 114: return VK_ADD;         // Keypad +
        case 115: return VK_RETURN;      // Keypad Enter
        case 116: return VK_OEM_NEC_EQUAL;// Keypad =
        // App keys
        case 117: return VK_BROWSER_BACK;
        case 118: return VK_BROWSER_FORWARD;
        // Skip gamepads
        // Mouse
        case 145: return VK_LBUTTON;     // Mouse 1
        case 146: return VK_RBUTTON;     // Mouse 2
        case 147: return VK_MBUTTON;     // Mouse 3
        case 148: return VK_XBUTTON1;    // Mouse 4
        case 149: return VK_XBUTTON2;    // Mouse 5
        // Mouse wheel
        case 150: return 0; // Mouse Wheel X (cannot map directly to VK)
        case 151: return 0; // Mouse Wheel Y (cannot map directly to VK)
        // Extra modifiers
        case 152: return VK_CONTROL;
        case 153: return VK_SHIFT;
        case 154: return VK_MENU;
        case 155: return VK_LWIN;
        default: return 0;
    }
}


static std::string get_vk_display_name(int vkKey)
{
    if (vkKey == 0) return "None";
    if (vkKey == VK_RSHIFT) return "Right Shift";
    if (vkKey == VK_LSHIFT) return "Left Shift";
    if (vkKey == VK_SHIFT) return "Shift";
    if (vkKey == VK_RCONTROL) return "Right Ctrl";
    if (vkKey == VK_LCONTROL) return "Left Ctrl";
    if (vkKey == VK_CONTROL) return "Ctrl";
    if (vkKey == VK_RMENU) return "Right Alt";
    if (vkKey == VK_LMENU) return "Left Alt";
    if (vkKey == VK_MENU) return "Alt";
    if (vkKey == VK_INSERT) return "Insert";
    if (vkKey == VK_DELETE) return "Delete";
    if (vkKey == VK_HOME) return "Home";
    if (vkKey == VK_END) return "End";
    if (vkKey == VK_PRIOR) return "Page Up";
    if (vkKey == VK_NEXT) return "Page Down";
    if (vkKey == VK_TAB) return "Tab";
    if (vkKey == VK_SPACE) return "Space";
    if (vkKey == VK_RETURN) return "Enter";
    if (vkKey == VK_ESCAPE) return "Escape";
    if (vkKey == VK_CAPITAL) return "Caps Lock";
    if (vkKey >= VK_F1 && vkKey <= VK_F24) return "F" + std::to_string(vkKey - VK_F1 + 1);
    if (vkKey >= '0' && vkKey <= '9') return std::string(1, (char)vkKey);
    if (vkKey >= 'A' && vkKey <= 'Z') return std::string(1, (char)vkKey);
    if (vkKey == VK_LBUTTON) return "Mouse 1";
    if (vkKey == VK_RBUTTON) return "Mouse 2";
    if (vkKey == VK_MBUTTON) return "Mouse 3";
    if (vkKey == VK_XBUTTON1) return "Mouse 4";
    if (vkKey == VK_XBUTTON2) return "Mouse 5";
    if (vkKey == VK_SUBTRACT) return "Num -";
    if (vkKey == VK_OEM_MINUS) return "-";
    if (vkKey == VK_ADD) return "Num +";
    if (vkKey == VK_OEM_PLUS) return "+";

    char keyName[64] = { 0 };
    UINT scanCode = MapVirtualKeyA(vkKey, MAPVK_VK_TO_VSC);
    if (scanCode != 0) {
        if (vkKey == VK_RSHIFT || vkKey == VK_RMENU || vkKey == VK_RCONTROL ||
            vkKey == VK_INSERT || vkKey == VK_DELETE || vkKey == VK_HOME ||
            vkKey == VK_END || vkKey == VK_PRIOR || vkKey == VK_NEXT) {
            scanCode |= 0x100;
        }
        if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName)) > 0) {
            return keyName;
        }
    }

    for(int i = 0; i < sizeof(keys)/sizeof(keys[0]); ++i)
    {
        if(vkKey == keyIndexToVK(i))
            return keys[i];
    }

    return "Key " + std::to_string(vkKey);
}

// In key_select widget
widget_t c_widgets::key_select(std::string_view name, int* vkKey, float custom_width)
{
    struct anim_t { bool active{ false }; float alpha{ 0 }; };
    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    widget_t data = gui->register_item(name, ImVec2(w, SCALE(elements->keybind.button_height)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    bool is_compact_bind = (name == "key_select" || name.rfind("key_select", 0) == 0);
    std::string buf_display;
    if (is_compact_bind) {
        if (anim->active) {
            buf_display = "...";
        } else if (*vkKey == 0) {
            buf_display = "Bind";
        } else {
            buf_display = get_vk_display_name(*vkKey);
        }
    } else {
        buf_display = anim->active ? "-" : get_vk_display_name(*vkKey);
    }

    bool is_standalone = !is_compact_bind;
    float avail_w = data->rect.GetWidth();
    float btn_width;
    float gap;
    if (is_compact_bind) {
        btn_width = avail_w;
        gap = 0.f;
    } else {
        if (avail_w < SCALE(130.f)) {
            btn_width = SCALE(52.f);
            gap = SCALE(6.f);
        } else {
            btn_width = SCALE(95.f);
            gap = SCALE(10.f);
        }
    }

    float btn_h = (is_standalone && avail_w < SCALE(130.f)) ? SCALE(21.f) : data->rect.GetHeight();
    float btn_y_offset = (data->rect.GetHeight() - btn_h) * 0.5f;

    ImRect btn_rect = is_standalone 
        ? ImRect(data->rect.Max.x - btn_width, data->rect.Min.y + btn_y_offset, data->rect.Max.x, data->rect.Min.y + btn_y_offset + btn_h)
        : data->rect;

    bool btn_hovered = btn_rect.Contains(gui->mouse_pos());

    if (btn_hovered && gui->mouse_released(mouse_button_left) && gui->is_window_hovered())
        anim->active = !anim->active;

    if (anim->active)
    {
        Cheat::g_IsBindingKey = true;

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            *vkKey = 0;
            anim->active = false;
            Cheat::g_IsBindingKey = false;
        }
        else
        {
            for(int i = 1; i < 256; ++i)
            {
                if (i == VK_LBUTTON) continue; // Don't catch the initial mouse click

                if (GetAsyncKeyState(i) & 0x8000)
                {
                    *vkKey = i;
                    anim->active = false;
                    Cheat::g_IsBindingKey = false;
                    break;
                }
            }
        }
    }

    gui->easing(anim->alpha, anim->active ? 1.f : 0.f, 8.f, static_easing);

    if (is_standalone)
    {
        ImFont* lbl_font = font->get(inter_medium_data, 13.f);
        draw->text_clipped(gui->window_drawlist(), lbl_font, 
            data->rect.Min, ImVec2(btn_rect.Min.x - gap, data->rect.Max.y), 
            draw->get_clr(clr->widgets.text_inactive), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));
    }

    float btn_rounding = (is_standalone && avail_w < SCALE(130.f)) ? SCALE(4.f) : SCALE(5.f);
    draw->rect_filled(gui->window_drawlist(), btn_rect.Min, btn_rect.Max, draw->get_clr(clr->widgets.background), btn_rounding);
    
    if (anim->active)
    {
        draw->rect_filled(gui->window_drawlist(), btn_rect.Min, btn_rect.Max, ImColor(0, 242, 254, 30), btn_rounding);
        draw->rect(gui->window_drawlist(), btn_rect.Min, btn_rect.Max, ImColor(0, 242, 254, 220), btn_rounding);
    }
    else
    {
        if (btn_hovered)
            draw->rect(gui->window_drawlist(), btn_rect.Min, btn_rect.Max, ImColor(0, 242, 254, 180), btn_rounding);
        else
            draw->rect(gui->window_drawlist(), btn_rect.Min, btn_rect.Max, ImColor(55, 68, 90, 180), btn_rounding);
    }

    ImFont* btn_font = font->get(inter_semibold_data, (avail_w < SCALE(130.f) || is_compact_bind) ? 11.f : 12.f);
    ImVec4 key_text_col = anim->active ? ImVec4(0.f, 0.95f, 1.f, 1.f) : (btn_hovered ? ImVec4(1.f, 1.f, 1.f, 1.f) : clr->widgets.text.Value);
    draw->text_clipped(gui->window_drawlist(), btn_font, btn_rect.Min, btn_rect.Max, draw->get_clr(key_text_col), buf_display.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::mode_select(std::string_view name, int* mode)
{
    struct anim_t
    {
        float alpha[2]{ 0, 0 };
        ImVec4 text[2]{ clr->widgets.text, clr->widgets.text };
    };

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, SCALE(elements->keybind.button_height)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    const ImRect toggle{ data->rect.Min, ImVec2(data->rect.GetCenter().x - var->style.item_spacing.x / 2, data->rect.Max.y) };
    const ImRect hold{ ImVec2(data->rect.GetCenter().x + var->style.item_spacing.x / 2, data->rect.Min.y), data->rect.Max };

    if (toggle.Contains(gui->mouse_pos()) && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())
        *mode = 0;
    if (hold.Contains(gui->mouse_pos()) && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())
        *mode = 1;

    gui->easing(anim->alpha[0], *mode == 0 ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->alpha[1], *mode == 1 ? 1.f : 0.f, 8.f, static_easing);

    gui->easing(anim->text[0], *mode == 0 ? ImVec4{0.f, 0.f, 0.f, 1.f} : clr->widgets.text.Value, 16.f, dynamic_easing);
    gui->easing(anim->text[1], *mode == 1 ? ImVec4{ 0.f, 0.f, 0.f, 1.f } : clr->widgets.text.Value, 16.f, dynamic_easing);

    draw->rect_filled(gui->window_drawlist(), toggle.Min, toggle.Max, draw->get_clr(clr->popup.sub_background), SCALE(elements->keybind.button_rounding));
    draw->rect_filled(gui->window_drawlist(), toggle.Min, toggle.Max, draw->get_clr(clr->accent, anim->alpha[0]), SCALE(elements->keybind.button_rounding));
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13.f), toggle.Min, toggle.Max, draw->get_clr(anim->text[0]), "Toggle", NULL, NULL, ImVec2(0.5f, 0.5f));

    draw->rect_filled(gui->window_drawlist(), hold.Min, hold.Max, draw->get_clr(clr->popup.sub_background), SCALE(elements->keybind.button_rounding));
    draw->rect_filled(gui->window_drawlist(), hold.Min, hold.Max, draw->get_clr(clr->accent, anim->alpha[1]), SCALE(elements->keybind.button_rounding));
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13.f), hold.Min, hold.Max, draw->get_clr(anim->text[1]), "Hold", NULL, NULL, ImVec2(0.5f, 0.5f));

    return data;
}