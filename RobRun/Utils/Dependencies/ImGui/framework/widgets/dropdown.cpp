#include "../headers/functions.h"
#include "../headers/widgets.h"

widget_t c_widgets::selectable(std::string_view name, bool active)
{
    struct anim_t
    {
        float alpha{ 0 };
        float offset{ 0 };
        ImVec4 text{ clr->widgets.text_inactive };
    };

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, SCALE(elements->selectable.size)));

    anim_t* anim = gui->anim_container<anim_t>(data->id);
    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();

    gui->easing(anim->text, active ? clr->widgets.text.Value : clr->widgets.text_inactive.Value, 16.f, dynamic_easing);
    gui->easing(anim->alpha, active ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->offset, active ? elements->selectable.padding + elements->selectable.radius : 0, 16.f, dynamic_easing);

    draw->rect_filled(gui->window_drawlist(), data->rect.Min, data->rect.Max, draw->get_clr(clr->popup.sub_background, anim->alpha), SCALE(elements->selectable.rounding));
    draw->circle_filled(gui->window_drawlist(), ImVec2(data->rect.Min.x + SCALE(elements->selectable.padding), data->rect.GetCenter().y), SCALE(elements->selectable.radius), draw->get_clr(clr->accent, anim->alpha), 30);
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min + SCALE(elements->selectable.padding + anim->offset, 0), data->rect.Max, draw->get_clr(anim->text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    return data;
}

bool begin_dropdown(std::string_view name, std::string& preview, int val, widget_t& data, bool sub = false, float custom_width = 0.f)
{
    struct anim_t
    {
        bool opened{ false };
        bool hovered{ false };
        int stored_val{ -1 };
        float alpha{ 0 };
    };

    float w = (custom_width > 0.f) ? custom_width : (sub ? SCALE(180) : gui->content_avail().x);
    data = gui->register_item(name, ImVec2(w, SCALE(sub ? elements->dropdown.size : elements->dropdown.height)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    const ImRect icon{ ImVec2(data->rect.Max.x - SCALE(elements->dropdown.padding * 2) - gui->text_size(font->get(icons_data, 8), "H").x, sub ? data->rect.Min.y : (data->rect.Max.y - SCALE(elements->dropdown.size))), data->rect.Max };
    const ImRect rect{ ImVec2(data->rect.Min.x, icon.Min.y), icon.GetBL() };

    if (anim->stored_val < 0)
        anim->stored_val = val;

    val = val > 5 ? 5 : val;

    if ((ImRect{ rect.Min, icon.Max }.Contains(gui->mouse_pos()) && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered()) || (anim->opened && gui->mouse_clicked(mouse_button_left) && !anim->hovered))
        anim->opened = !anim->opened;

    gui->easing(anim->alpha, anim->opened ? 1.f : 0.f, 8.f, static_easing);


    draw->rect_filled(gui->window_drawlist(), rect.Min, icon.Max, draw->get_clr(sub ? clr->window.sub_background : clr->widgets.background), SCALE(elements->dropdown.rounding));
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), rect.Min + SCALE(elements->dropdown.padding, 0), rect.Max, draw->get_clr(clr->widgets.text), preview.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    draw->rect_filled_multi_color(gui->window_drawlist(), icon.Min - SCALE(elements->dropdown.padding, 0), icon.GetBL(), draw->get_clr(sub ? clr->window.sub_background : clr->widgets.background, 0.f), draw->get_clr(sub ? clr->window.sub_background : clr->widgets.background), draw->get_clr(sub ? clr->window.sub_background : clr->widgets.background), draw->get_clr(sub ? clr->window.sub_background : clr->widgets.background, 0.f));
    draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 8), icon.Min, icon.Max, draw->get_clr(clr->widgets.text), "H", NULL, NULL, ImVec2(0.5f, 0.5f));

    if (!sub)
        draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min - SCALE(0, 3), icon.GetTR(), draw->get_clr(clr->widgets.text_inactive), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.f));

    	if (!anim->opened && (anim->alpha <= 0.01f || !gui->is_rect_visible(ImRect{ rect.GetBL(), icon.GetBL() + SCALE(0, 2) })))
	{
		anim->hovered = false;
		anim->alpha = 0;
		return false;
	}

    gui->push_var(style_var_alpha, anim->alpha);
    gui->push_var(style_var_window_rounding, SCALE(elements->dropdown.rounding));
    gui->push_var(style_var_window_padding, SCALE(0, 0));
    gui->push_var(style_var_item_spacing, SCALE(0, 0));
    gui->push_var(style_var_popup_border_size, 0.f);
    gui->push_var(style_var_scrollbar_border_padding, SCALE(elements->dropdown.scrollbar_border));
    gui->push_var(style_var_scrollbar_content_padding, SCALE(elements->dropdown.scrollbar_content));
    gui->push_color(style_col_popup_bg, draw->get_clr(clr->popup.background));

    gui->set_next_window_pos(gui->adjust_window_pos(rect.GetTL(), ImVec2(data->rect.GetWidth(), val * SCALE(elements->selectable.size))));
    gui->set_next_window_size(ImVec2(data->rect.GetWidth(), val * SCALE(elements->selectable.size)));
    gui->begin("dropdown_window" + std::to_string(data->id), nullptr, window_flags_tooltip | window_flags_always_use_window_padding | window_flags_no_saved_settings | window_flags_no_focus_on_appearing | window_flags_always_auto_resize | window_flags_no_title_bar | window_flags_no_resize | window_flags_no_collapse | (anim->stored_val <= 5 ? (window_flags_no_scrollbar | window_flags_no_scroll_with_mouse) : window_flags_always_vertical_scrollbar));

    gui->set_window_focus();
    anim->hovered = gui->is_window_hovered();

    return true;
}

void end_dropdown()
{
    gui->end();
    gui->pop_color();
    gui->pop_var(7);
}

widget_t c_widgets::dropdown(std::string_view key)
{
    dropdown_t* data = cfg->fill<dropdown_t>(key.data());
    widget_t item_data;

    std::string preview{ data->items.at(data->callback) };

    if (begin_dropdown(data->name, preview, data->items.size(), item_data))
    {
        for (int i = 0; i < data->items.size(); ++i)
        {
            if (selectable(data->items.at(i), data->callback == i).value_changed)
            {
                data->callback = i;
                item_data->value_changed = gui->is_item_clicked(mouse_button_left);
            }
        }

        end_dropdown();
    }

    return item_data;
}

widget_t c_widgets::multi_dropdown(std::string_view key)
{
    multi_dropdown_t* data = cfg->fill<multi_dropdown_t>(key.data());
    widget_t item_data;

    std::string preview = "-";

    for (size_t i = 0; i < data->items.size(); ++i)
    {
        if (data->callback.at(i))
        {
            if (preview == "-")
                preview = data->items[i];
            else
                preview += (", ") + data->items[i];
        }
    }

    if (begin_dropdown(data->name, preview, data->items.size(), item_data))
    {
        for (size_t i = 0; i < data->items.size(); ++i)
        {
            if (selectable(data->items.at(i), data->callback.at(i)).value_changed)
            {
                data->callback.at(i) = !data->callback.at(i);
                item_data->value_changed = gui->is_item_clicked(mouse_button_left);
            }
        }
        end_dropdown();
    }

    preview = ("-");

    return item_data;
}

widget_t c_widgets::sub_dropdown(std::string_view name, int* callback, std::vector<std::string> items, float custom_width)
{
    widget_t item_data;

    std::string preview{ items.at(*callback) };

    if (begin_dropdown(name, preview, items.size(), item_data, true, custom_width))
    {
        for (int i = 0; i < items.size(); ++i)
        {
            if (selectable(items.at(i), *callback == i).value_changed)
            {
                *callback = i;
                item_data->value_changed = gui->is_item_clicked(mouse_button_left);
            }
        }

        end_dropdown();
    }

    return item_data;
}

widget_t c_widgets::placement_dropdown(std::string_view dropdown_key, std::string_view label)
{
    dropdown_t* drop_data = cfg->fill<dropdown_t>(dropdown_key.data());
    if (!drop_data || drop_data->items.empty()) return {};

    if (drop_data->callback < 0 || drop_data->callback >= (int)drop_data->items.size())
        drop_data->callback = 0;

    struct anim_t
    {
        bool opened{ false };
        bool hovered{ false };
        float alpha{ 0 };
    };

    float total_w = gui->content_avail().x;
    float row_h = SCALE(24.f);
    float indent_x = SCALE(18.f);

    // Register item
    widget_t data = gui->register_item(dropdown_key, ImVec2(total_w, row_h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    // Draw tree branch connector "└─" connecting from above row
    ImDrawList* dl = gui->window_drawlist();
    ImVec2 branch_start = ImVec2(data->rect.Min.x + SCALE(8.f), data->rect.Min.y - SCALE(4.f));
    ImVec2 branch_corner = ImVec2(data->rect.Min.x + SCALE(8.f), data->rect.GetCenter().y);
    ImVec2 branch_end = ImVec2(data->rect.Min.x + indent_x - SCALE(3.f), data->rect.GetCenter().y);
    dl->AddLine(branch_start, branch_corner, IM_COL32(55, 70, 85, 180), 1.2f);
    dl->AddLine(branch_corner, branch_end, IM_COL32(55, 70, 85, 180), 1.2f);

    // Sub-label e.g. "Placement"
    float label_w = gui->text_size(font->get(inter_medium_data, 12), label.data()).x;
    draw->text_clipped(dl, font->get(inter_medium_data, 12), 
                       ImVec2(data->rect.Min.x + indent_x, data->rect.Min.y), 
                       ImVec2(data->rect.Min.x + indent_x + label_w + SCALE(6.f), data->rect.Max.y), 
                       draw->get_clr(clr->widgets.text_inactive), label.data(), gui->text_end(label.data()), NULL, ImVec2(0.f, 0.5f));

    // Right-aligned dropdown pill box
    float box_w = SCALE(105.f);
    float box_h = SCALE(22.f);
    ImRect box_rect(ImVec2(data->rect.Max.x - box_w, data->rect.GetCenter().y - box_h * 0.5f),
                    ImVec2(data->rect.Max.x, data->rect.GetCenter().y + box_h * 0.5f));

    bool box_hovered = box_rect.Contains(gui->mouse_pos()) && gui->is_window_hovered();

    if (box_hovered && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
    {
        anim->opened = !anim->opened;
    }
    else if (anim->opened && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)) && !anim->hovered)
    {
        anim->opened = false;
    }

    gui->easing(anim->alpha, anim->opened ? 1.f : 0.f, 8.f, static_easing);

    // Box background
    ImU32 bg_col = box_hovered ? IM_COL32(20, 26, 32, 230) : IM_COL32(14, 18, 22, 210);
    draw->rect_filled(dl, box_rect.Min, box_rect.Max, bg_col, SCALE(4.0f));

    // Box border (glowing cyan when active/opened or hovered)
    ImU32 border_col = anim->opened ? IM_COL32(0, 242, 254, 220) : (box_hovered ? IM_COL32(0, 242, 254, 140) : (ImU32)draw->get_clr(clr->window.stroke));
    draw->rect(dl, box_rect.Min, box_rect.Max, border_col, SCALE(4.0f));

    // Selected text preview
    std::string preview = drop_data->items.at(drop_data->callback);
    draw->text_clipped(dl, font->get(inter_medium_data, 11), 
                       box_rect.Min + SCALE(8.f, 0.f), box_rect.Max - SCALE(18.f, 0.f), 
                       (anim->opened || box_hovered) ? IM_COL32(0, 242, 254, 255) : (ImU32)draw->get_clr(clr->widgets.text), 
                       preview.c_str(), NULL, NULL, ImVec2(0.f, 0.5f));

    // Downward chevron icon
    draw->text_clipped(dl, font->get(icons_data, 7), 
                       box_rect.Max - SCALE(18.f, box_h), box_rect.Max - SCALE(2.f, 0.f), 
                       (anim->opened || box_hovered) ? IM_COL32(0, 242, 254, 240) : (ImU32)draw->get_clr(clr->widgets.text_inactive), 
                       "H", NULL, NULL, ImVec2(0.5f, 0.5f));

    // Popup window
    if (anim->opened || anim->alpha > 0.01f)
    {
        float pop_w = box_w + SCALE(10.f);
        int total_items = (int)drop_data->items.size();
        int visible_items = total_items > 6 ? 6 : total_items;
        float item_h = SCALE(22.f);
        float pop_h = visible_items * item_h;

        gui->push_var(style_var_alpha, anim->alpha);
        gui->push_var(style_var_window_rounding, SCALE(elements->dropdown.rounding));
        gui->push_var(style_var_window_padding, SCALE(0, 0));
        gui->push_var(style_var_item_spacing, SCALE(0, 0));
        gui->push_var(style_var_popup_border_size, 0.f);
        gui->push_var(style_var_scrollbar_border_padding, SCALE(elements->dropdown.scrollbar_border));
        gui->push_var(style_var_scrollbar_content_padding, SCALE(elements->dropdown.scrollbar_content));
        gui->push_color(style_col_popup_bg, draw->get_clr(clr->popup.background));

        gui->set_next_window_pos(gui->adjust_window_pos(box_rect.GetBL() + SCALE(0, 2), ImVec2(pop_w, pop_h)));
        gui->set_next_window_size(ImVec2(pop_w, pop_h));
        gui->begin("drop_pop_" + std::to_string(data->id), nullptr, window_flags_tooltip | window_flags_always_use_window_padding | window_flags_no_saved_settings | window_flags_no_focus_on_appearing | window_flags_always_auto_resize | window_flags_no_title_bar | window_flags_no_resize | window_flags_no_collapse | (total_items <= 6 ? (window_flags_no_scrollbar | window_flags_no_scroll_with_mouse) : window_flags_always_vertical_scrollbar));
        {
            gui->set_window_focus();
            anim->hovered = gui->is_window_hovered();

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
                    anim->opened = false;
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

                draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 12), 
                                   item_rect.Min + SCALE(elements->dropdown.padding, 0), item_rect.Max, 
                                   is_sel ? IM_COL32(0, 242, 254, 255) : draw->get_clr(item_hovered ? clr->widgets.text : clr->widgets.text_inactive), 
                                   drop_data->items.at(i).data(), NULL, NULL, ImVec2(0.f, 0.5f));

                gui->dummy(ImVec2(pop_w, item_h));
            }
        }
        gui->end();
        gui->pop_color();
        gui->pop_var(7);
    }

    return data;
}
