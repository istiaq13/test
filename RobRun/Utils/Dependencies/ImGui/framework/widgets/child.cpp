#include "../headers/functions.h"
#include "../headers/widgets.h"

bool c_widgets::begin_child(std::string_view name, std::string_view button_text)
{
    return begin_child_ex(name, 0.f, false, button_text, "", nullptr);
}

bool c_widgets::begin_child(std::string_view name, std::string_view button1_text, std::string_view button2_text, bool* btn2_clicked)
{
    return begin_child_ex(name, 0.f, false, button1_text, button2_text, btn2_clicked);
}

bool c_widgets::begin_child_ex(std::string_view name, float custom_width, bool glowing_border, std::string_view button1_text, std::string_view button2_text, bool* btn2_clicked)
{
    struct child_state
    {
        float height{ 0 };
        float width{ 0 };
    };

    ImGuiWindow* window = gui->get_window();
    const ImVec2 pos = window->DC.CursorPos + SCALE(0, elements->child.title_height);

    child_state* state = gui->anim_container<child_state>(window->GetID(name.data()));
    if (custom_width > 0.f)
        state->width = custom_width;
    else if (custom_width < 0.f)
        state->width = gui->content_avail().x;
    else
        state->width = (gui->content_max().x - var->style.item_spacing.x - var->style.window_padding.x) / 2.f;

    draw->rect_filled(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), draw->get_clr(clr->window.sub_background), SCALE(elements->child.rounding));

    if (glowing_border)
    {
        draw->rect(window->DrawList, window->DC.CursorPos - ImVec2(1.f, 1.f), pos + ImVec2(state->width, state->height) + ImVec2(1.f, 1.f), ImColor(0, 242, 254, 30), SCALE(elements->child.rounding + 1.f));
        draw->rect(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), ImColor(0, 242, 254, 150), SCALE(elements->child.rounding));
    }
    else
    {
        draw->rect(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), draw->get_clr(clr->window.stroke), SCALE(elements->child.rounding));
    }

    draw->text_clipped(window->DrawList, font->get(inter_semibold_data, 13), window->DC.CursorPos + SCALE(elements->child.padding.x, 0), pos + ImVec2(state->width, 0), draw->get_clr(clr->widgets.text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    bool button_clicked = false;
    if (btn2_clicked) *btn2_clicked = false;

    ImFont* btn_font = font->get(inter_medium_data, 13);
    float btn_h = SCALE(22.f);
    ImVec2 title_start = window->DC.CursorPos;
    float cur_right_x = title_start.x + state->width - SCALE(elements->child.padding.x);

    // Button 2 (Create) rendered on the right
    if (!button2_text.empty())
    {
        ImVec2 text_size2 = gui->text_size(btn_font, button2_text.data());
        float btn2_w = text_size2.x + SCALE(16.f);
        cur_right_x -= btn2_w;

        ImVec2 btn2_pos = ImVec2(cur_right_x, title_start.y + (SCALE(elements->child.title_height) - btn_h) / 2.f);
        ImRect btn2_rect(btn2_pos, btn2_pos + ImVec2(btn2_w, btn_h));

        ImGuiID btn2_id = window->GetID((std::string(name) + "_title_btn2").c_str());
        gui->item_add(btn2_rect, btn2_id);
        bool hovered2 = btn2_rect.Contains(gui->mouse_pos());
        if (hovered2 && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
        {
            if (btn2_clicked) *btn2_clicked = true;
        }

        struct btn_anim2 { float hover{ 0.f }; };
        btn_anim2* banim2 = gui->anim_container<btn_anim2>(btn2_id);
        gui->easing(banim2->hover, hovered2 ? 1.f : 0.f, 10.f, dynamic_easing);

        draw->rect_filled(window->DrawList, btn2_rect.Min, btn2_rect.Max, draw->get_clr(clr->widgets.sub_background), SCALE(4.f));
        if (banim2->hover > 0.01f)
        {
            ImVec4 acc_col = clr->accent.Value;
            acc_col.w = banim2->hover * 0.4f;
            draw->rect_filled(window->DrawList, btn2_rect.Min, btn2_rect.Max, draw->get_clr(acc_col), SCALE(4.f));
        }
        draw->rect(window->DrawList, btn2_rect.Min, btn2_rect.Max, draw->get_clr(clr->window.stroke), SCALE(4.f));
        draw->text_clipped(window->DrawList, btn_font, btn2_rect.Min, btn2_rect.Max, draw->get_clr(clr->widgets.text), button2_text.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

        cur_right_x -= SCALE(6.f);
    }

    // Button 1 (Add) rendered next to Button 2
    if (!button1_text.empty())
    {
        ImVec2 text_size1 = gui->text_size(btn_font, button1_text.data());
        float btn1_w = text_size1.x + SCALE(16.f);
        cur_right_x -= btn1_w;

        ImVec2 btn1_pos = ImVec2(cur_right_x, title_start.y + (SCALE(elements->child.title_height) - btn_h) / 2.f);
        ImRect btn1_rect(btn1_pos, btn1_pos + ImVec2(btn1_w, btn_h));

        ImGuiID btn1_id = window->GetID((std::string(name) + "_title_btn1").c_str());
        gui->item_add(btn1_rect, btn1_id);
        bool hovered1 = btn1_rect.Contains(gui->mouse_pos());
        if (hovered1 && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
            button_clicked = true;

        struct btn_anim1 { float hover{ 0.f }; };
        btn_anim1* banim1 = gui->anim_container<btn_anim1>(btn1_id);
        gui->easing(banim1->hover, hovered1 ? 1.f : 0.f, 10.f, dynamic_easing);

        draw->rect_filled(window->DrawList, btn1_rect.Min, btn1_rect.Max, draw->get_clr(clr->widgets.sub_background), SCALE(4.f));
        if (banim1->hover > 0.01f)
        {
            ImVec4 acc_col = clr->accent.Value;
            acc_col.w = banim1->hover * 0.4f;
            draw->rect_filled(window->DrawList, btn1_rect.Min, btn1_rect.Max, draw->get_clr(acc_col), SCALE(4.f));
        }
        draw->rect(window->DrawList, btn1_rect.Min, btn1_rect.Max, draw->get_clr(clr->window.stroke), SCALE(4.f));
        draw->text_clipped(window->DrawList, btn_font, btn1_rect.Min, btn1_rect.Max, draw->get_clr(clr->widgets.text), button1_text.data(), NULL, NULL, ImVec2(0.5f, 0.5f));
    }

    gui->push_var(style_var_window_padding, SCALE(elements->child.padding));
    gui->set_screen_pos(pos, pos_all);
    gui->begin_def_child(name.data() + std::string("content_child"), ImVec2(state->width, state->height), 0, window_flags_always_use_window_padding | window_flags_no_move | window_flags_nav_flattened | window_flags_no_saved_settings | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
    gui->push_var(style_var_item_spacing, SCALE(elements->child.spacing));

    state->height = gui->get_window()->ContentSize.y + SCALE(elements->child.padding.y * 2);

    return button_clicked;
}

bool c_widgets::begin_child_toggle(std::string_view name, bool* header_switch, const char* switch_label, float custom_width, bool glowing_border)
{
    struct child_state
    {
        float height{ 0 };
        float width{ 0 };
    };

    ImGuiWindow* window = gui->get_window();
    const ImVec2 pos = window->DC.CursorPos + SCALE(0, elements->child.title_height);

    child_state* state = gui->anim_container<child_state>(window->GetID(name.data()));
    if (custom_width > 0.f)
        state->width = custom_width;
    else if (custom_width < 0.f)
        state->width = gui->content_avail().x;
    else
        state->width = (gui->content_max().x - var->style.item_spacing.x - var->style.window_padding.x) / 2.f;

    draw->rect_filled(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), draw->get_clr(clr->window.sub_background), SCALE(elements->child.rounding));

    if (glowing_border)
    {
        draw->rect(window->DrawList, window->DC.CursorPos - ImVec2(1.f, 1.f), pos + ImVec2(state->width, state->height) + ImVec2(1.f, 1.f), ImColor(0, 242, 254, 30), SCALE(elements->child.rounding + 1.f));
        draw->rect(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), ImColor(0, 242, 254, 150), SCALE(elements->child.rounding));
    }
    else
    {
        draw->rect(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), draw->get_clr(clr->window.stroke), SCALE(elements->child.rounding));
    }

    draw->text_clipped(window->DrawList, font->get(inter_semibold_data, 13), window->DC.CursorPos + SCALE(elements->child.padding.x, 0), pos + ImVec2(state->width, 0), draw->get_clr(clr->widgets.text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    if (header_switch)
    {
        float switch_w = SCALE(elements->checkbox.size.x);
        float switch_h = SCALE(elements->checkbox.size.y);
        float pad_knob = SCALE(elements->checkbox.padding);
        float r_knob   = SCALE(elements->checkbox.radius);

        const char* lbl = switch_label ? switch_label : "";
        bool has_lbl = (lbl && lbl[0] != '\0');
        ImVec2 txt_sz = has_lbl ? gui->text_size(font->get(inter_medium_data, 12), lbl) : ImVec2(0.f, 0.f);
        float total_toggle_w = has_lbl ? (txt_sz.x + SCALE(8.f) + switch_w) : switch_w;

        ImVec2 toggle_start = ImVec2(window->DC.CursorPos.x + state->width - SCALE(elements->child.padding.x) - total_toggle_w, window->DC.CursorPos.y + (SCALE(elements->child.title_height) - switch_h) * 0.5f);
        ImRect toggle_full_rect(toggle_start, toggle_start + ImVec2(total_toggle_w, switch_h));

        ImGuiID toggle_id = window->GetID((std::string(name) + "_header_switch").c_str());
        gui->item_add(toggle_full_rect, toggle_id);

        bool hovered = toggle_full_rect.Contains(gui->mouse_pos());
        if (hovered && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
        {
            *header_switch = !*header_switch;
        }

        struct hdr_anim_t { float alpha{ 0.f }; float offset{ 0.f }; ImVec4 circle{ clr->widgets.sub_background }; };
        hdr_anim_t* anim = gui->anim_container<hdr_anim_t>(toggle_id);
        gui->easing(anim->alpha, *header_switch ? 1.f : 0.f, 8.f, static_easing);
        float max_offset = switch_w - (pad_knob * 2.f + r_knob * 2.f);
        gui->easing(anim->offset, *header_switch ? max_offset : 0.f, 16.f, dynamic_easing);
        gui->easing(anim->circle, *header_switch ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.55f, 0.62f, 0.72f, 1.f), 16.f, dynamic_easing);

        // Label
        if (has_lbl)
        {
            ImVec2 txt_pos = ImVec2(toggle_start.x, toggle_start.y + (switch_h - txt_sz.y) * 0.5f);
            window->DrawList->AddText(font->get(inter_medium_data, 12), SCALE(12.f), txt_pos, draw->get_clr(clr->widgets.text_inactive), lbl);
        }

        // Switch capsule
        ImRect switch_rect(ImVec2(toggle_full_rect.Max.x - switch_w, toggle_start.y), ImVec2(toggle_full_rect.Max.x, toggle_start.y + switch_h));
        draw->rect_filled(window->DrawList, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->widgets.background), switch_h * 0.5f);

        if (anim->alpha > 0.01f)
        {
            draw->rect_filled(window->DrawList, switch_rect.Min - SCALE(2.f, 2.f), switch_rect.Max + SCALE(2.f, 2.f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), (switch_h + SCALE(4.f)) * 0.5f);
            draw->rect_filled(window->DrawList, switch_rect.Min, switch_rect.Max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), switch_h * 0.5f);
        }
        draw->rect(window->DrawList, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->window.stroke), switch_h * 0.5f);

        ImVec2 knob_center = ImVec2(switch_rect.Min.x + pad_knob + r_knob + anim->offset, switch_rect.GetCenter().y);
        draw->circle_filled(window->DrawList, knob_center, r_knob, draw->get_clr(anim->circle), 24);
    }

    gui->push_var(style_var_window_padding, SCALE(elements->child.padding));
    gui->set_screen_pos(pos, pos_all);
    gui->begin_def_child(name.data() + std::string("content_child"), ImVec2(state->width, state->height), 0, window_flags_always_use_window_padding | window_flags_no_move | window_flags_nav_flattened | window_flags_no_saved_settings | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
    gui->push_var(style_var_item_spacing, SCALE(elements->child.spacing));

    state->height = gui->get_window()->ContentSize.y + SCALE(elements->child.padding.y * 2);

    return true;
}

bool c_widgets::begin_child_scrollable(std::string_view name, float custom_width, float custom_height, bool glowing_border)
{
    struct child_state
    {
        float height{ 0 };
        float width{ 0 };
    };

    ImGuiWindow* window = gui->get_window();
    const ImVec2 pos = window->DC.CursorPos + SCALE(0, elements->child.title_height);

    child_state* state = gui->anim_container<child_state>(window->GetID(name.data()));
    if (custom_width > 0.f)
        state->width = custom_width;
    else if (custom_width < 0.f)
        state->width = gui->content_avail().x;
    else
        state->width = (gui->content_max().x - var->style.item_spacing.x - var->style.window_padding.x) / 2.f;

    float avail_h = gui->content_avail().y - SCALE(elements->child.title_height);
    if (avail_h < SCALE(150.f))
        avail_h = SCALE(440.f);

    if (custom_height > 0.f)
        state->height = custom_height;
    else
        state->height = avail_h;

    draw->rect_filled(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), draw->get_clr(clr->window.sub_background), SCALE(elements->child.rounding));

    if (glowing_border)
    {
        draw->rect(window->DrawList, window->DC.CursorPos - ImVec2(1.f, 1.f), pos + ImVec2(state->width, state->height) + ImVec2(1.f, 1.f), ImColor(0, 242, 254, 30), SCALE(elements->child.rounding + 1.f));
        draw->rect(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), ImColor(0, 242, 254, 150), SCALE(elements->child.rounding));
    }
    else
    {
        draw->rect(window->DrawList, window->DC.CursorPos, pos + ImVec2(state->width, state->height), draw->get_clr(clr->window.stroke), SCALE(elements->child.rounding));
    }

    draw->text_clipped(window->DrawList, font->get(inter_semibold_data, 13), window->DC.CursorPos + SCALE(elements->child.padding.x, 0), pos + ImVec2(state->width, 0), draw->get_clr(clr->widgets.text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    gui->push_var(style_var_window_padding, SCALE(elements->child.padding));
    gui->set_screen_pos(pos, pos_all);
    gui->begin_def_child(name.data() + std::string("content_child"), ImVec2(state->width, state->height), 0, window_flags_always_use_window_padding | window_flags_no_move | window_flags_nav_flattened | window_flags_no_saved_settings);
    gui->push_var(style_var_item_spacing, SCALE(elements->child.spacing));

    return true;
}

void c_widgets::end_child()
{
    gui->pop_var();
    gui->end_def_child();
    gui->pop_var();
}