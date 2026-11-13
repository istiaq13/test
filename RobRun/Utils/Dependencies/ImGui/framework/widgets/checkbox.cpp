#include "../headers/functions.h"
#include "../headers/widgets.h"

widget_t checkbox_ex(std::string_view name, bool* callback, float* color, bool alpha, bool keybind, key_data_t* state)
{
    struct anim_t
    {
        float alpha{ 0 };
        float offset{ 0 };
        ImVec4 circle{ clr->widgets.sub_background };
        ImVec4 text{ clr->widgets.text_inactive };

        float keybind_alpha{ 0 };
        ImVec4 key_icon{ clr->widgets.text_inactive };
        bool keybind_hovered{ false };
        bool keybind_opened{ false };
        float window_height{ 0 };
    };

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, SCALE(elements->checkbox.size.y)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    const ImRect rect{ data->rect.GetTR() - SCALE(elements->checkbox.size.x, 0), data->rect.Max };
    const ImRect color_rect{ ImVec2(rect.Min.x - SCALE(elements->color_edit.padding.x + elements->color_edit.size), data->rect.GetCenter().y - SCALE(elements->color_edit.size / 2)), ImVec2(rect.Min.x - SCALE(elements->color_edit.padding.x), data->rect.GetCenter().y + SCALE(elements->color_edit.size / 2)) };
    const ImRect key_rect = color ? ImRect{ color_rect.Min - SCALE(elements->keybind.padding.x + elements->keybind.width, 0), color_rect.GetBL() - SCALE(elements->keybind.padding.x, 0) } : color_rect;

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered() && (color ? !color_rect.Contains(gui->mouse_pos()) : true) && (keybind ? !key_rect.Contains(gui->mouse_pos()) : true);

    if (data->value_changed)
        *callback = !*callback;

    gui->easing(anim->alpha, *callback ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->offset, *callback ? rect.GetWidth() - SCALE(elements->checkbox.padding * 2 + elements->checkbox.radius * 2) - 1 : 0, 16.f, dynamic_easing);
    gui->easing(anim->circle, *callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.55f, 0.62f, 0.72f, 1.f), 16.f, dynamic_easing);
    gui->easing(anim->text, *callback ? clr->widgets.text.Value : clr->widgets.text_inactive.Value, 16.f, dynamic_easing);

    // Dark inset base capsule
    draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->widgets.background), rect.GetHeight() / 2);

    // Active state: vibrant Ice Cyan neon capsule with soft outer bloom aura
    if (anim->alpha > 0.01f)
    {
        draw->rect_filled(gui->window_drawlist(), rect.Min - SCALE(2.5f, 2.5f), rect.Max + SCALE(2.5f, 2.5f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), (rect.GetHeight() + SCALE(5.f)) / 2);
        draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), rect.GetHeight() / 2);
    }
    draw->rect(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->window.stroke), rect.GetHeight() / 2);

    // Sliding knob (pure white when ON, smooth dynamic slide)
    draw->circle_filled(gui->window_drawlist(), ImVec2(rect.Min.x + SCALE(elements->checkbox.padding + elements->checkbox.radius) + anim->offset, rect.GetCenter().y), SCALE(elements->checkbox.radius), draw->get_clr(anim->circle), 30);
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, rect.GetBL(), draw->get_clr(anim->text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    if (color)
        widgets->color_edit_ex(name.data() + std::string("color_edit"), name.data(), color, alpha, color_rect, color_rect, &data->value_changed);
    
    if (keybind)
    {
        gui->easing(anim->key_icon, (key_rect.Contains(gui->mouse_pos()) && gui->is_window_hovered()) || anim->keybind_opened ? clr->widgets.text.Value : clr->widgets.text_inactive.Value, 16.f, dynamic_easing);
        gui->easing(anim->keybind_alpha, anim->keybind_opened ? 1.f : 0.f, 8.f, static_easing);

        draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 10), key_rect.Min, key_rect.Max, draw->get_clr(anim->key_icon), "G", NULL, NULL, ImVec2(0.5f, 0.5f));

        if ((key_rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered()) || (anim->keybind_opened && (gui->mouse_clicked(mouse_button_left) || gui->mouse_clicked(mouse_button_right)) && !anim->keybind_hovered))
            anim->keybind_opened = !anim->keybind_opened;

        if (anim->keybind_alpha >= 0.01f)
        {
            gui->push_var(style_var_alpha, anim->keybind_alpha);
            gui->push_var(style_var_item_spacing, SCALE(elements->keybind.spacing));
            gui->push_var(style_var_window_padding, SCALE(elements->keybind.padding));
            gui->push_var(style_var_window_rounding, SCALE(elements->keybind.window_rounding));
            gui->push_var(style_var_popup_border_size, SCALE(0));
            gui->push_color(style_col_popup_bg, draw->get_clr(clr->popup.background));
            gui->set_next_window_pos(gui->adjust_window_pos(key_rect.GetBL() - SCALE(elements->keybind.window_width / 2, -elements->keybind.padding.y), ImVec2(SCALE(elements->keybind.window_width), anim->window_height)));
            gui->set_next_window_size(SCALE(elements->keybind.window_width, 0));
            gui->begin("keybind_window" + std::string(name), nullptr, window_flags_tooltip | window_flags_always_use_window_padding | window_flags_no_saved_settings | window_flags_no_focus_on_appearing | window_flags_always_auto_resize | window_flags_no_decoration | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
            {
                gui->set_window_focus();
                anim->keybind_hovered = gui->is_window_hovered();

                draw->rect_filled(gui->window_drawlist(), gui->window_pos(), gui->window_pos() + ImVec2(gui->window_width(), SCALE(elements->keybind.title_bar)), draw->get_clr(clr->popup.sub_background), SCALE(elements->keybind.window_rounding), draw_flags_round_corners_top);
                draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), gui->window_pos() + SCALE(elements->keybind.padding.x, 0), gui->window_pos() + ImVec2(gui->window_width(), SCALE(elements->keybind.title_bar)), draw->get_clr(clr->widgets.text), (std::stringstream{} << "\"" << name << "\"").str().data(), NULL, NULL, ImVec2(0.f, 0.5f));

                gui->dummy(SCALE(0, elements->keybind.title_bar - elements->keybind.padding.y));

                widgets->key_select("key_select", &state->key);

                widgets->mode_select("mode_select", &state->mode);

                anim->window_height = gui->window_height();
            }
            gui->end();
            gui->pop_color();
            gui->pop_var(5);
        }
    }

    return data;
}

widget_t c_widgets::checkbox(std::string_view key, std::string_view custom_label)
{
    checkbox_t* data = cfg->fill<checkbox_t>(key.data());
    std::string_view display_name = custom_label.empty() ? data->name : custom_label;

    return checkbox_ex(display_name, &data->callback, nullptr, false, data->keybind, &data->state);
}

widget_t c_widgets::color_checkbox(std::string_view key, std::string_view custom_label)
{
    color_checkbox_t* data = cfg->fill<color_checkbox_t>(key.data());
    std::string_view display_name = custom_label.empty() ? data->name : custom_label;

    return checkbox_ex(display_name, &data->callback, data->color.data(), data->alpha, false, nullptr);
}

widget_t c_widgets::color_checkbox_dropdown(std::string_view checkbox_key, std::string_view dropdown_key, std::string_view custom_label)
{
    color_checkbox_t* cb_data = cfg->fill<color_checkbox_t>(checkbox_key.data());
    dropdown_t* drop_data = cfg->fill<dropdown_t>(dropdown_key.data());

    std::string_view display_name = custom_label.empty() ? cb_data->name : custom_label;

    struct anim_t
    {
        float alpha{ 0 };
        float offset{ 0 };
        ImVec4 circle{ clr->widgets.sub_background };
        ImVec4 text{ clr->widgets.text_inactive };

        float drop_alpha{ 0 };
        bool drop_opened{ false };
        bool drop_hovered{ false };
    };

    widget_t data = gui->register_item(checkbox_key, ImVec2(gui->content_avail().x, SCALE(elements->checkbox.size.y)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    // 1. Rightmost toggle switch
    const ImRect rect{ data->rect.GetTR() - SCALE(elements->checkbox.size.x, 0), data->rect.Max };

    // 2. Color picker chip (left of toggle switch)
    const ImRect color_rect{ ImVec2(rect.Min.x - SCALE(elements->color_edit.padding.x + elements->color_edit.size), data->rect.GetCenter().y - SCALE(elements->color_edit.size / 2)),
                             ImVec2(rect.Min.x - SCALE(elements->color_edit.padding.x), data->rect.GetCenter().y + SCALE(elements->color_edit.size / 2)) };

    // 3. Placement dropdown pill (in the middle between text label and color palette)
    const bool show_drop = cb_data->callback;
    float drop_w = SCALE(80.f);
    float drop_h = SCALE(21.f);
    float drop_gap = SCALE(8.f);
    const ImRect drop_rect{ ImVec2(color_rect.Min.x - drop_gap - drop_w, data->rect.GetCenter().y - drop_h * 0.5f),
                            ImVec2(color_rect.Min.x - drop_gap, data->rect.GetCenter().y + drop_h * 0.5f) };

    bool mouse_in_color = color_rect.Contains(gui->mouse_pos());
    bool mouse_in_drop = show_drop && drop_rect.Contains(gui->mouse_pos());

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered() && !mouse_in_color && !mouse_in_drop;
    if (data->value_changed)
    {
        cb_data->callback = !cb_data->callback;
        if (!cb_data->callback)
        {
            anim->drop_opened = false;
        }
    }

    gui->easing(anim->alpha, cb_data->callback ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->offset, cb_data->callback ? rect.GetWidth() - SCALE(elements->checkbox.padding * 2 + elements->checkbox.radius * 2) - 1 : 0, 16.f, dynamic_easing);
    gui->easing(anim->circle, cb_data->callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.55f, 0.62f, 0.72f, 1.f), 16.f, dynamic_easing);
    gui->easing(anim->text, cb_data->callback ? clr->widgets.text.Value : clr->widgets.text_inactive.Value, 16.f, dynamic_easing);

    // Dark inset base capsule
    draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->widgets.background), rect.GetHeight() / 2);

    // Active state: vibrant Ice Cyan neon capsule
    if (anim->alpha > 0.01f)
    {
        draw->rect_filled(gui->window_drawlist(), rect.Min - SCALE(2.5f, 2.5f), rect.Max + SCALE(2.5f, 2.5f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), (rect.GetHeight() + SCALE(5.f)) / 2);
        draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), rect.GetHeight() / 2);
    }
    draw->rect(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->window.stroke), rect.GetHeight() / 2);

    // Sliding knob
    draw->circle_filled(gui->window_drawlist(), ImVec2(rect.Min.x + SCALE(elements->checkbox.padding + elements->checkbox.radius) + anim->offset, rect.GetCenter().y), SCALE(elements->checkbox.radius), draw->get_clr(anim->circle), 30);

    // Label text on the left (clipped before drop_rect when enabled, or before color_rect when disabled)
    float text_end_x = show_drop ? (drop_rect.Min.x - SCALE(6.f)) : (color_rect.Min.x - SCALE(8.f));
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, ImVec2(text_end_x, data->rect.Max.y), draw->get_clr(anim->text), display_name.data(), gui->text_end(display_name.data()), NULL, ImVec2(0.f, 0.5f));

    // Color picker
    widgets->color_edit_ex(checkbox_key.data() + std::string("color_edit"), checkbox_key.data(), cb_data->color.data(), cb_data->alpha, color_rect, color_rect, &data->value_changed);

    // Placement Dropdown (appears in the middle only when the section is enabled)
    if (show_drop && drop_data && !drop_data->items.empty())
    {
        if (drop_data->callback < 0 || drop_data->callback >= (int)drop_data->items.size())
            drop_data->callback = 0;

        std::string preview = drop_data->items.at(drop_data->callback);

        bool drop_hover = mouse_in_drop && gui->is_window_hovered();
        if (drop_hover && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
        {
            anim->drop_opened = !anim->drop_opened;
        }
        else if (anim->drop_opened && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)) && !anim->drop_hovered)
        {
            anim->drop_opened = false;
        }

        gui->easing(anim->drop_alpha, anim->drop_opened ? 1.f : 0.f, 8.f, static_easing);

        // Box background (dark obsidian glass)
        ImU32 bg_col = (anim->drop_hovered || drop_hover) ? IM_COL32(20, 26, 32, 230) : IM_COL32(14, 18, 22, 210);
        draw->rect_filled(gui->window_drawlist(), drop_rect.Min, drop_rect.Max, bg_col, SCALE(4.0f));

        // Box border (glowing cyan when active or hovered)
        ImU32 border_col = anim->drop_opened ? IM_COL32(0, 242, 254, 220) : ((anim->drop_hovered || drop_hover) ? IM_COL32(0, 242, 254, 140) : (ImU32)draw->get_clr(clr->window.stroke));
        draw->rect(gui->window_drawlist(), drop_rect.Min, drop_rect.Max, border_col, SCALE(4.0f));

        // Preview text
        draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 11), drop_rect.Min + SCALE(7.f, 0.f), drop_rect.Max - SCALE(16.f, 0.f), (anim->drop_opened || drop_hover) ? IM_COL32(0, 242, 254, 255) : (ImU32)draw->get_clr(clr->widgets.text), preview.c_str(), NULL, NULL, ImVec2(0.f, 0.5f));

        // Crisp vector triangle arrow (No 'H' text glyph)
        ImDrawList* dl = gui->window_drawlist();
        ImVec2 arrow_center = ImVec2(drop_rect.Max.x - SCALE(9.f), drop_rect.GetCenter().y);
        ImU32 arrow_col = (anim->drop_opened || drop_hover) ? IM_COL32(0, 242, 254, 240) : (ImU32)draw->get_clr(clr->widgets.text_inactive);
        if (anim->drop_opened)
        {
            dl->AddTriangleFilled(
                arrow_center + SCALE(0.f, -2.5f),
                arrow_center + SCALE(-3.5f, 2.5f),
                arrow_center + SCALE(3.5f, 2.5f),
                arrow_col);
        }
        else
        {
            dl->AddTriangleFilled(
                arrow_center + SCALE(-3.5f, -2.5f),
                arrow_center + SCALE(3.5f, -2.5f),
                arrow_center + SCALE(0.f, 2.5f),
                arrow_col);
        }

        // Dropdown popup window
        if (anim->drop_opened || anim->drop_alpha > 0.01f)
        {
            float pop_w = drop_w + SCALE(16.f);
            int total_items = (int)drop_data->items.size();
            int visible_items = total_items > 6 ? 6 : total_items;
            float item_h = SCALE(22.f);
            float pop_h = visible_items * item_h;

            gui->push_var(style_var_alpha, anim->drop_alpha);
            gui->push_var(style_var_window_rounding, SCALE(elements->dropdown.rounding));
            gui->push_var(style_var_window_padding, SCALE(0, 0));
            gui->push_var(style_var_item_spacing, SCALE(0, 0));
            gui->push_var(style_var_popup_border_size, 0.f);
            gui->push_var(style_var_scrollbar_border_padding, SCALE(elements->dropdown.scrollbar_border));
            gui->push_var(style_var_scrollbar_content_padding, SCALE(elements->dropdown.scrollbar_content));
            gui->push_color(style_col_popup_bg, draw->get_clr(clr->popup.background));

            gui->set_next_window_pos(gui->adjust_window_pos(drop_rect.GetBL() + SCALE(0, 2), ImVec2(pop_w, pop_h)));
            gui->set_next_window_size(ImVec2(pop_w, pop_h));
            gui->begin("drop_pop_" + std::to_string(data->id), nullptr, window_flags_tooltip | window_flags_always_use_window_padding | window_flags_no_saved_settings | window_flags_no_focus_on_appearing | window_flags_always_auto_resize | window_flags_no_title_bar | window_flags_no_resize | window_flags_no_collapse | (total_items <= 6 ? (window_flags_no_scrollbar | window_flags_no_scroll_with_mouse) : window_flags_always_vertical_scrollbar));
            {
                gui->set_window_focus();
                anim->drop_hovered = gui->is_window_hovered();

                for (int i = 0; i < total_items; ++i)
                {
                    bool is_sel = (drop_data->callback == i);
                    ImVec2 item_start = gui->get_pos();
                    ImRect item_rect(gui->window_pos() + item_start, gui->window_pos() + item_start + ImVec2(pop_w, item_h));

                    bool item_hovered = item_rect.Contains(gui->mouse_pos());
                    if (item_hovered && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
                    {
                        drop_data->callback = i;
                        data->value_changed = true;
                        anim->drop_opened = false;
                    }

                    if (is_sel)
                    {
                        draw->rect_filled(gui->window_drawlist(), item_rect.Min, item_rect.Max, IM_COL32(0, 242, 254, 40));
                        draw->rect(gui->window_drawlist(), item_rect.Min, item_rect.Max, IM_COL32(0, 242, 254, 120), 0, 1.0f);
                    }
                    else if (item_hovered)
                    {
                        draw->rect_filled(gui->window_drawlist(), item_rect.Min, item_rect.Max, draw->get_clr(clr->popup.sub_background));
                    }

                    ImU32 item_col = is_sel ? IM_COL32(0, 242, 254, 255) : draw->get_clr(item_hovered ? clr->widgets.text : clr->widgets.text_inactive);
                    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 11), item_rect.Min + SCALE(8.f, 0.f), item_rect.Max, item_col, drop_data->items.at(i).c_str(), NULL, NULL, ImVec2(0.f, 0.5f));

                    gui->dummy(ImVec2(pop_w, item_h));
                }
            }
            gui->end();
            gui->pop_color();
            gui->pop_var(7);
        }
    }

    return data;
}

widget_t c_widgets::switch_toggle(std::string_view name, bool* callback, std::string_view label_right, std::string_view label_left, float custom_width)
{
    struct anim_t
    {
        float alpha{ 0.f };
        float offset{ 0.f };
        ImVec4 circle{ clr->widgets.sub_background };
    };

    float switch_w = SCALE(elements->checkbox.size.x);
    float switch_h = SCALE(elements->checkbox.size.y);
    float pad_knob = SCALE(elements->checkbox.padding);
    float r_knob   = SCALE(elements->checkbox.radius);

    float total_w = custom_width;
    if (total_w <= 0.f)
    {
        if (!label_right.empty())
        {
            ImVec2 txt_sz = gui->text_size(font->get(inter_medium_data, 13), label_right.data());
            total_w = switch_w + SCALE(8.f) + txt_sz.x;
        }
        else if (!label_left.empty())
        {
            total_w = gui->content_avail().x;
        }
        else
        {
            total_w = switch_w;
        }
    }

    widget_t data = gui->register_item(name, ImVec2(total_w, switch_h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();
    if (data->value_changed)
        *callback = !*callback;

    gui->easing(anim->alpha, *callback ? 1.f : 0.f, 8.f, static_easing);
    float max_offset = switch_w - (pad_knob * 2.f + r_knob * 2.f);
    gui->easing(anim->offset, *callback ? max_offset : 0.f, 16.f, dynamic_easing);
    gui->easing(anim->circle, *callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.55f, 0.62f, 0.72f, 1.f), 16.f, dynamic_easing);

    ImDrawList* drawlist = gui->window_drawlist();

    ImRect switch_rect;
    if (!label_right.empty())
    {
        switch_rect = ImRect(data->rect.Min, ImVec2(data->rect.Min.x + switch_w, data->rect.Max.y));
        ImVec2 text_min = ImVec2(switch_rect.Max.x + SCALE(8.f), data->rect.Min.y);
        ImVec4 text_col = *callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : clr->widgets.text_inactive.Value;
        draw->text_clipped(drawlist, font->get(inter_medium_data, 13), text_min, data->rect.Max, draw->get_clr(text_col), label_right.data(), NULL, NULL, ImVec2(0.f, 0.5f));
    }
    else if (!label_left.empty())
    {
        switch_rect = ImRect(ImVec2(data->rect.Max.x - switch_w, data->rect.Min.y), data->rect.Max);
        ImVec2 text_max = ImVec2(switch_rect.Min.x - SCALE(8.f), data->rect.Max.y);
        draw->text_clipped(drawlist, font->get(inter_medium_data, 13), data->rect.Min, text_max, draw->get_clr(clr->widgets.text_inactive), label_left.data(), NULL, NULL, ImVec2(0.f, 0.5f));
    }
    else
    {
        switch_rect = data->rect;
    }

    // Inset dark background
    draw->rect_filled(drawlist, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->widgets.background), switch_h * 0.5f);

    // Active cyan pill fill with glow bloom
    if (anim->alpha > 0.01f)
    {
        draw->rect_filled(drawlist, switch_rect.Min - SCALE(2.5f, 2.5f), switch_rect.Max + SCALE(2.5f, 2.5f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), (switch_h + SCALE(5.f)) * 0.5f);
        draw->rect_filled(drawlist, switch_rect.Min, switch_rect.Max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), switch_h * 0.5f);
    }
    draw->rect(drawlist, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->window.stroke), switch_h * 0.5f);

    // Sliding knob
    ImVec2 knob_center = ImVec2(switch_rect.Min.x + pad_knob + r_knob + anim->offset, switch_rect.GetCenter().y);
    draw->circle_filled(drawlist, knob_center, r_knob, draw->get_clr(anim->circle), 24);

    return data;
}

widget_t c_widgets::pill_toggle(std::string_view name, bool* callback, float custom_width, float custom_height)
{
    struct anim_t
    {
        float alpha{ 0.f };
        float offset{ 0.f };
        float hover{ 0.f };
        ImVec4 circle{ clr->widgets.sub_background };
    };

    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    float h = (custom_height > 0.f) ? custom_height : SCALE(34.f);

    widget_t data = gui->register_item(name, ImVec2(w, h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();
    if (data->value_changed)
        *callback = !*callback;

    gui->easing(anim->alpha, *callback ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->hover, data->hovered ? 1.f : 0.f, 10.f, dynamic_easing);

    float switch_w = SCALE(elements->checkbox.size.x);
    float switch_h = SCALE(elements->checkbox.size.y);
    float pad_knob = SCALE(elements->checkbox.padding);
    float r_knob   = SCALE(elements->checkbox.radius);
    float max_offset = switch_w - (pad_knob * 2.f + r_knob * 2.f);

    gui->easing(anim->offset, *callback ? max_offset : 0.f, 16.f, dynamic_easing);
    gui->easing(anim->circle, *callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.55f, 0.62f, 0.72f, 1.f), 16.f, dynamic_easing);

    ImDrawList* drawlist = gui->window_drawlist();
    float round = SCALE(6.f);

    // Box background
    draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.background), round);

    if (anim->alpha > 0.01f)
    {
        // Glowing cyan border & fill (matching screenshot)
        draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(18.f * anim->alpha)), round);
        draw->rect(drawlist, data->rect.Min - SCALE(1.f, 1.f), data->rect.Max + SCALE(1.f, 1.f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), round + 1.f);
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(200.f * anim->alpha)), round);
    }
    else
    {
        draw->rect(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->window.stroke), round);
    }

    if (anim->hover > 0.01f && anim->alpha <= 0.01f)
    {
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(255, 255, 255, (int)(30.f * anim->hover)), round);
    }

    // Label text on the left
    ImVec4 text_col = *callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : clr->widgets.text_inactive.Value;
    float pad_x = SCALE(10.f);
    ImVec2 txt_min = ImVec2(data->rect.Min.x + pad_x, data->rect.Min.y);
    ImVec2 txt_max = ImVec2(data->rect.Max.x - switch_w - pad_x, data->rect.Max.y);
    draw->text_clipped(drawlist, font->get(inter_medium_data, 13), txt_min, txt_max, draw->get_clr(text_col), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    // Switch capsule on the right
    ImRect switch_rect(ImVec2(data->rect.Max.x - pad_x - switch_w, data->rect.Min.y + (h - switch_h) * 0.5f),
                       ImVec2(data->rect.Max.x - pad_x, data->rect.Min.y + (h + switch_h) * 0.5f));

    draw->rect_filled(drawlist, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->window.sub_background), switch_h * 0.5f);
    if (anim->alpha > 0.01f)
    {
        draw->rect_filled(drawlist, switch_rect.Min - SCALE(2.f, 2.f), switch_rect.Max + SCALE(2.f, 2.f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), (switch_h + SCALE(4.f)) * 0.5f);
        draw->rect_filled(drawlist, switch_rect.Min, switch_rect.Max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), switch_h * 0.5f);
    }
    draw->rect(drawlist, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->window.stroke), switch_h * 0.5f);

    // Switch knob
    ImVec2 knob_center = ImVec2(switch_rect.Min.x + pad_knob + r_knob + anim->offset, switch_rect.GetCenter().y);
    draw->circle_filled(drawlist, knob_center, r_knob, draw->get_clr(anim->circle), 24);

    return data;
}

widget_t c_widgets::checkbox_slider(std::string_view key_cb, std::string_view key_slider, std::string_view custom_label, float force_min)
{
    checkbox_t* cb_data = cfg->fill<checkbox_t>(key_cb.data());
    slider_float_t* sl_data = cfg->fill<slider_float_t>(key_slider.data());

    if (force_min > 0.f)
    {
        sl_data->min = force_min;
        if (sl_data->callback < force_min)
            sl_data->callback = force_min;
    }

    std::string_view label = custom_label.empty() ? cb_data->name : custom_label;

    float row_h = SCALE(elements->checkbox.size.y);
    float avail_w = gui->content_avail().x;

    widget_t data = gui->register_item(std::string(key_cb) + "_cbsl", ImVec2(avail_w, row_h));
    ImDrawList* drawlist = gui->window_drawlist();

    struct cbsl_anim_t
    {
        float switch_alpha{ 0.f };
        float switch_offset{ 0.f };
        ImVec4 circle{ clr->widgets.sub_background };
        float slider_alpha{ 0.f };
        float grab_ratio{ -1.f };
    };
    cbsl_anim_t* anim = gui->anim_container<cbsl_anim_t>(data->id);

    float switch_w = SCALE(elements->checkbox.size.x);
    float switch_h = SCALE(elements->checkbox.size.y);
    float pad_knob = SCALE(elements->checkbox.padding);
    float r_knob   = SCALE(elements->checkbox.radius);

    ImRect switch_rect(ImVec2(data->rect.Max.x - switch_w, data->rect.Min.y + (row_h - switch_h) * 0.5f),
                       ImVec2(data->rect.Max.x, data->rect.Min.y + (row_h + switch_h) * 0.5f));

    ImFont* lbl_font = font->get(inter_medium_data, 13);
    ImVec2 lbl_sz = gui->text_size(lbl_font, label.data());
    float text_right_x = data->rect.Min.x + lbl_sz.x;

    // Adjuster area between text and switch (with enough distance)
    float adj_left_x = text_right_x + SCALE(14.f);
    float adj_right_x = switch_rect.Min.x - SCALE(14.f);
    float adj_w = adj_right_x - adj_left_x;

    char val_buf[64];
    gui->get_fmt(val_buf, &sl_data->callback, sl_data->format);
    ImFont* val_font = font->get(inter_semibold_data, 11);
    ImVec2 val_sz = gui->text_size(val_font, val_buf);
    float val_w = val_sz.x + SCALE(4.f);

    float track_w = adj_w - val_w - SCALE(10.f);
    if (track_w < SCALE(20.f)) track_w = SCALE(20.f);

    float track_h = SCALE(3.5f);
    float track_y = data->rect.GetCenter().y;
    ImRect track_rect(ImVec2(adj_left_x, track_y - track_h * 0.5f), ImVec2(adj_left_x + track_w, track_y + track_h * 0.5f));
    ImRect slider_interactive_rect(ImVec2(track_rect.Min.x - SCALE(8.f), data->rect.Min.y), ImVec2(adj_right_x, data->rect.Max.y));

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = gui->get_window();
    ImGuiID slider_id = window->GetID((std::string(key_slider) + "_inline").c_str());

    bool slider_active = cb_data->callback;
    bool slider_held = false;

    // Do not activate or drag slider if the application window itself is being moved
    if (g.MovingWindow != NULL)
    {
        if (g.ActiveId == slider_id)
            ImGui::ClearActiveID();
        slider_held = false;
    }
    else if (slider_active)
    {
        gui->item_add(slider_interactive_rect, slider_id);

        if ((g.ActiveId == 0 || g.ActiveId == slider_id) && slider_interactive_rect.Contains(g.IO.MousePos) && (g.IO.MouseClicked[0] || (gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())))
        {
            ImGui::SetActiveID(slider_id, window);
            ImGui::SetFocusID(slider_id, window);
            slider_held = true;
        }

        if (g.ActiveId == slider_id)
        {
            if (g.IO.MouseDown[0])
            {
                slider_held = true;
                float t = ImSaturate((g.IO.MousePos.x - track_rect.Min.x) / track_rect.GetWidth());
                sl_data->callback = sl_data->min + t * (sl_data->max - sl_data->min);
                sl_data->callback = ImClamp(sl_data->callback, sl_data->min, sl_data->max);
                data->value_changed = true;

                // Re-format buffer for live zero-latency display
                gui->get_fmt(val_buf, &sl_data->callback, sl_data->format);
            }
            else
            {
                ImGui::ClearActiveID();
                slider_held = false;
            }
        }
    }

    if (g.MovingWindow == NULL && !slider_held && g.ActiveId != slider_id && data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())
    {
        if (!slider_active || !slider_interactive_rect.Contains(gui->mouse_pos()))
        {
            cb_data->callback = !cb_data->callback;
            data->value_changed = true;
        }
    }

    gui->easing(anim->switch_alpha, cb_data->callback ? 1.f : 0.f, 8.f, static_easing);
    float max_switch_offset = switch_w - (pad_knob * 2.f + r_knob * 2.f);
    gui->easing(anim->switch_offset, cb_data->callback ? max_switch_offset : 0.f, 16.f, dynamic_easing);
    gui->easing(anim->circle, cb_data->callback ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.55f, 0.62f, 0.72f, 1.f), 16.f, dynamic_easing);
    gui->easing(anim->slider_alpha, cb_data->callback ? 1.f : 0.f, 12.f, dynamic_easing);

    // Use relative 0..1 ratio instead of absolute screen coordinates so moving the window never displaces the knob
    float t_val = (sl_data->max > sl_data->min) ? (sl_data->callback - sl_data->min) / (sl_data->max - sl_data->min) : 0.f;
    t_val = ImClamp(t_val, 0.f, 1.f);

    if (anim->grab_ratio < 0.f)
    {
        anim->grab_ratio = t_val;
    }

    if (slider_held)
    {
        anim->grab_ratio = t_val;
    }
    else
    {
        gui->easing(anim->grab_ratio, t_val, 20.f, dynamic_easing);
    }

    // Label on the left
    ImVec4 text_col = cb_data->callback ? clr->widgets.text.Value : clr->widgets.text_inactive.Value;
    draw->text_clipped(drawlist, lbl_font, data->rect.Min, ImVec2(text_right_x + SCALE(4.f), data->rect.Max.y), draw->get_clr(text_col), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    // Inline Adjuster in the middle
    if (anim->slider_alpha > 0.01f)
    {
        // Dark track
        draw->rect_filled(drawlist, track_rect.Min, track_rect.Max, draw->get_clr(clr->widgets.background, anim->slider_alpha), track_h * 0.5f);
        draw->rect(drawlist, track_rect.Min, track_rect.Max, draw->get_clr(clr->window.stroke, anim->slider_alpha * 0.6f), track_h * 0.5f);

        // Neon cyan progress (strictly relative to track_rect)
        float cur_knob_x = track_rect.Min.x + ImClamp(anim->grab_ratio, 0.f, 1.f) * track_rect.GetWidth();
        draw->rect_filled(drawlist, track_rect.Min, ImVec2(cur_knob_x, track_rect.Max.y), ImColor(0, 242, 254, (int)(255.f * anim->slider_alpha)), track_h * 0.5f);

        // Knob with soft glow
        float knob_r = slider_held ? SCALE(6.f) : SCALE(5.f);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r + SCALE(2.5f), ImColor(0, 242, 254, (int)(slider_held ? 80.f : 45.f * anim->slider_alpha)), 20);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r, ImColor(0, 242, 254, (int)(255.f * anim->slider_alpha)), 20);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r * 0.5f, ImColor(255, 255, 255, (int)(255.f * anim->slider_alpha)), 20);

        // Value text
        ImVec2 val_min(track_rect.Max.x + SCALE(6.f), data->rect.Min.y);
        ImVec2 val_max(adj_right_x, data->rect.Max.y);
        draw->text_clipped(drawlist, val_font, val_min, val_max, ImColor(0, 242, 254, (int)(240.f * anim->slider_alpha)), val_buf, NULL, NULL, ImVec2(0.5f, 0.5f));
    }

    // Switch capsule on the far right
    draw->rect_filled(drawlist, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->widgets.background), switch_h * 0.5f);
    if (anim->switch_alpha > 0.01f)
    {
        draw->rect_filled(drawlist, switch_rect.Min - SCALE(2.f, 2.f), switch_rect.Max + SCALE(2.f, 2.f), ImColor(0, 242, 254, (int)(45.f * anim->switch_alpha)), (switch_h + SCALE(4.f)) * 0.5f);
        draw->rect_filled(drawlist, switch_rect.Min, switch_rect.Max, ImColor(0, 242, 254, (int)(255.f * anim->switch_alpha)), switch_h * 0.5f);
    }
    draw->rect(drawlist, switch_rect.Min, switch_rect.Max, draw->get_clr(clr->window.stroke), switch_h * 0.5f);

    // Sliding knob
    ImVec2 knob_center = ImVec2(switch_rect.Min.x + pad_knob + r_knob + anim->switch_offset, switch_rect.GetCenter().y);
    draw->circle_filled(drawlist, knob_center, r_knob, draw->get_clr(anim->circle), 24);

    return data;
}