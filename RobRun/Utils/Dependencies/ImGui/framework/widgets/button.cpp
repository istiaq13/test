#include "../headers/functions.h"
#include "../headers/widgets.h"

widget_t c_widgets::button(std::string_view name, float custom_width)
{
    struct anim_t
    {
        float timer{ 0 };
        bool active{ false };
        ImVec4 rect{ clr->widgets.background };
        ImVec4 text{ clr->widgets.text };
    };

    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    widget_t data = gui->register_item(name, ImVec2(w, SCALE(elements->button.height)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    ImGuiContext& g = *GImGui;
    bool raw_click = data->hovered && g.IO.MouseClicked[0];
    bool normal_click = data->hovered && gui->mouse_clicked(mouse_button_left);
    bool win_hovered = gui->is_window_hovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || data->hovered;

    data->value_changed = (raw_click || normal_click) && win_hovered;

    if (data->value_changed)
    {
        anim->active = true;
        ImGui::ClearActiveID();
    }

    gui->easing(anim->timer, anim->active ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->rect, data->hovered && win_hovered && !anim->active ? clr->widgets.sub_background.Value : anim->active ? clr->accent.Value : clr->widgets.background.Value, 16.f, dynamic_easing);
    gui->easing(anim->text, anim->active ? ImVec4{ 0, 0, 0, 1 } : clr->widgets.text.Value, 16.f, dynamic_easing);

    if (anim->timer >= 0.99f)
        anim->active = false;

    float r = SCALE(elements->button.rounding);
    draw->rect_filled(gui->window_drawlist(), data->rect.Min, data->rect.Max, draw->get_clr(anim->rect), r);

    ImU32 stroke_col = anim->active 
        ? (ImU32)draw->get_clr(clr->accent, 0.86f) 
        : (data->hovered ? IM_COL32(255, 255, 255, 80) : IM_COL32(55, 68, 90, 180));
    draw->rect(gui->window_drawlist(), data->rect.Min, data->rect.Max, stroke_col, r);

    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, data->rect.Max, draw->get_clr(anim->text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::create_button(std::string_view name)
{
    struct anim_t
    {

        ImVec4 rect{ clr->widgets.background };
        ImVec4 text{ clr->widgets.text };
    };

    widget_t data = gui->register_item(name, SCALE(elements->config.create_button, elements->config.create_button));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();

    gui->easing(anim->rect, data->hovered ? clr->accent.Value : clr->window.sub_background.Value, 16.f, dynamic_easing);
    gui->easing(anim->text, data->hovered ? ImVec4{ 0, 0, 0, 1 } : clr->widgets.text.Value, 16.f, dynamic_easing);

    draw->rect_filled(gui->window_drawlist(), data->rect.Min, data->rect.Max, draw->get_clr(anim->rect), SCALE(elements->button.rounding));
    draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 8), data->rect.Min, data->rect.Max, draw->get_clr(anim->text), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::button_toggle(std::string_view name, bool* callback, float custom_width, float custom_height)
{
    struct anim_t
    {
        float alpha{ 0.f };
        float hover{ 0.f };
    };

    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    float h = (custom_height > 0.f) ? custom_height : SCALE(36.f);

    widget_t data = gui->register_item(name, ImVec2(w, h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();
    if (data->value_changed)
        *callback = !*callback;

    gui->easing(anim->alpha, *callback ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->hover, data->hovered ? 1.f : 0.f, 10.f, dynamic_easing);

    float r = SCALE(6.f);
    ImDrawList* drawlist = gui->window_drawlist();

    // Dark base button
    draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.background), r);

    if (anim->alpha > 0.01f)
    {
        // Glowing cyan border & subtle fill when active
        draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(22.f * anim->alpha)), r);
        draw->rect(drawlist, data->rect.Min - SCALE(1.f, 1.f), data->rect.Max + SCALE(1.f, 1.f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), r + 1.f);
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(220.f * anim->alpha)), r);
    }
    else
    {
        draw->rect(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->window.stroke), r);
    }

    if (anim->hover > 0.01f && anim->alpha <= 0.01f)
    {
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(255, 255, 255, (int)(35.f * anim->hover)), r);
    }

    // Centered label - bright cyan when active, inactive gray when off
    ImVec4 text_col = *callback ? ImVec4(0.f, 0.95f, 1.f, 1.f) : clr->widgets.text_inactive.Value;
    ImFont* btn_font = (name.length() > 14) ? font->get(inter_medium_data, 11) : font->get(inter_medium_data, 13);
    draw->text_clipped(drawlist, btn_font, data->rect.Min, data->rect.Max, draw->get_clr(text_col), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::color_button_toggle(std::string_view name, bool* callback, float* color, bool alpha, float custom_width, float custom_height)
{
    struct anim_t
    {
        float alpha{ 0.f };
        float hover{ 0.f };
    };

    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    float h = (custom_height > 0.f) ? custom_height : SCALE(36.f);

    widget_t data = gui->register_item(name, ImVec2(w, h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    float color_sz = SCALE(elements->color_edit.size);
    float pad_r = SCALE(elements->color_edit.padding.x);
    ImRect color_rect(ImVec2(data->rect.Max.x - pad_r - color_sz, data->rect.GetCenter().y - color_sz * 0.5f),
                      ImVec2(data->rect.Max.x - pad_r, data->rect.GetCenter().y + color_sz * 0.5f));

    bool color_hovered = color_rect.Contains(gui->mouse_pos());
    if (color)
    {
        color_edit_ex(name.data() + std::string("_col_edit"), name.data(), color, alpha, color_rect, color_rect, &data->value_changed);
    }

    data->value_changed = data->hovered && !color_hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();
    if (data->value_changed)
        *callback = !*callback;

    gui->easing(anim->alpha, *callback ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->hover, data->hovered ? 1.f : 0.f, 10.f, dynamic_easing);

    float r = SCALE(6.f);
    ImDrawList* drawlist = gui->window_drawlist();

    // Dark base button
    draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.background), r);

    if (anim->alpha > 0.01f)
    {
        // Glowing cyan border & subtle fill when active
        draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(22.f * anim->alpha)), r);
        draw->rect(drawlist, data->rect.Min - SCALE(1.f, 1.f), data->rect.Max + SCALE(1.f, 1.f), ImColor(0, 242, 254, (int)(45.f * anim->alpha)), r + 1.f);
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(220.f * anim->alpha)), r);
    }
    else
    {
        draw->rect(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->window.stroke), r);
    }

    if (anim->hover > 0.01f && anim->alpha <= 0.01f)
    {
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(255, 255, 255, (int)(35.f * anim->hover)), r);
    }

    // Label with space for color edit on right
    ImVec4 text_col = *callback ? ImVec4(0.f, 0.95f, 1.f, 1.f) : clr->widgets.text_inactive.Value;
    ImRect text_rect(data->rect.Min + SCALE(elements->child.padding.x, 0), ImVec2(color_rect.Min.x - SCALE(4.f), data->rect.Max.y));
    ImFont* btn_font = (name.length() > 14) ? font->get(inter_medium_data, 11) : font->get(inter_medium_data, 13);
    draw->text_clipped(drawlist, btn_font, text_rect.Min, text_rect.Max, draw->get_clr(text_col), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::button_dummy(std::string_view name, float custom_width, float custom_height)
{
    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    float h = (custom_height > 0.f) ? custom_height : SCALE(elements->button.height);

    widget_t data = gui->register_item(name, ImVec2(w, h));
    data->value_changed = false;

    float r = SCALE(elements->button.rounding);
    ImDrawList* drawlist = gui->window_drawlist();

    // Dark base button
    draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.background), r);
    draw->rect(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->window.stroke), r);

    // Centered muted label (unclickable, disabled look)
    draw->text_clipped(drawlist, font->get(inter_medium_data, 13), data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.text_inactive.Value), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::button_toggle_3(std::string_view name, int* callback, float custom_width, float custom_height)
{
    struct anim_t
    {
        float active[3]{ 0.f, 0.f, 0.f };
        float hover[3]{ 0.f, 0.f, 0.f };
    };

    float w = (custom_width > 0.f) ? custom_width : gui->content_avail().x;
    float h = (custom_height > 0.f) ? custom_height : SCALE(50.f);

    widget_t data = gui->register_item(name, ImVec2(w, h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    const char* labels[3] = { "OFF", "PARTIAL", "FULL" };
    ImVec4 colors[3] = {
        clr->widgets.text_inactive.Value,  // 0: OFF -> God Mode disabled color (no red)
        ImVec4(0.96f, 0.62f, 0.05f, 1.0f), // 1: PARTIAL -> Electric Amber
        ImVec4(0.00f, 0.95f, 1.00f, 1.0f)  // 2: FULL -> Neon Cyan
    };

    int current_val = *callback;
    if (current_val < 0) current_val = 0;
    if (current_val > 2) current_val = 2;

    ImDrawList* drawlist = gui->window_drawlist();

    // 1. Top Title Row: "BULLET MISS" on left (status text removed per request)
    float title_h = SCALE(16.f);
    ImVec2 title_min = data->rect.Min;
    ImVec2 title_max = ImVec2(data->rect.Max.x, data->rect.Min.y + title_h);

    std::string title_str = std::string(name);
    for (auto& ch : title_str) ch = (char)toupper(ch);

    draw->text_clipped(drawlist, font->get(inter_semibold_data, 12), title_min, title_max, draw->get_clr(clr->widgets.text_inactive), title_str.c_str(), NULL, NULL, ImVec2(0.f, 0.5f));

    // 2. Bottom Segmented Button Container
    float seg_btn_y = data->rect.Min.y + title_h + SCALE(4.f);
    ImRect btn_box(ImVec2(data->rect.Min.x, seg_btn_y), data->rect.Max);

    float r = SCALE(elements->button.rounding);
    draw->rect_filled(drawlist, btn_box.Min, btn_box.Max, draw->get_clr(clr->widgets.background), r);
    draw->rect(drawlist, btn_box.Min, btn_box.Max, draw->get_clr(clr->window.stroke), r);

    float pad = SCALE(2.5f);
    float total_inner_w = btn_box.GetWidth() - pad * 2.f;
    float seg_gap = SCALE(3.5f);
    float seg_w = (total_inner_w - seg_gap * 2.f) / 3.f;
    float seg_h = btn_box.GetHeight() - pad * 2.f;

    for (int i = 0; i < 3; ++i)
    {
        ImVec2 seg_min = ImVec2(btn_box.Min.x + pad + i * (seg_w + seg_gap), btn_box.Min.y + pad);
        ImVec2 seg_max = ImVec2(seg_min.x + seg_w, seg_min.y + seg_h);
        ImRect seg_rect(seg_min, seg_max);

        bool hovered = seg_rect.Contains(gui->mouse_pos()) && gui->is_window_hovered();
        if (hovered && gui->mouse_clicked(mouse_button_left))
        {
            *callback = i;
            data->value_changed = true;
        }

        gui->easing(anim->active[i], (*callback == i) ? 1.f : 0.f, 10.f, dynamic_easing);
        gui->easing(anim->hover[i], (hovered && *callback != i) ? 1.f : 0.f, 12.f, dynamic_easing);

        float seg_r = SCALE(elements->button.rounding - 1.f);
        ImVec4 c = colors[i];

        if (i == 0 && anim->active[0] > 0.01f)
        {
            // OFF enabled: matches disabled Godmode button color (no red)
            float act = anim->active[0];
            draw->rect_filled(drawlist, seg_min, seg_max, draw->get_clr(clr->widgets.background), seg_r);
            draw->rect(drawlist, seg_min, seg_max, draw->get_clr(clr->window.stroke, 0.8f + 0.2f * act), seg_r);
            draw->text_clipped(drawlist, font->get(inter_medium_data, 12), seg_min, seg_max, draw->get_clr(clr->widgets.text_inactive.Value), labels[0], NULL, NULL, ImVec2(0.5f, 0.5f));
        }
        else if (anim->active[i] > 0.01f)
        {
            // PARTIAL (Amber) or FULL (Cyan)
            float act = anim->active[i];
            draw->rect_filled(drawlist, seg_min, seg_max, ImColor(c.x, c.y, c.z, 0.20f * act), seg_r);
            draw->rect(drawlist, seg_min - SCALE(1.f, 1.f), seg_max + SCALE(1.f, 1.f), ImColor(c.x, c.y, c.z, 0.35f * act), seg_r + 1.f);
            draw->rect(drawlist, seg_min, seg_max, ImColor(c.x, c.y, c.z, 0.95f * act), seg_r);
            draw->text_clipped(drawlist, font->get(inter_semibold_data, 12), seg_min, seg_max, ImColor(c.x, c.y, c.z, 1.0f), labels[i], NULL, NULL, ImVec2(0.5f, 0.5f));
        }
        else
        {
            if (anim->hover[i] > 0.01f)
            {
                float hov = anim->hover[i];
                draw->rect_filled(drawlist, seg_min, seg_max, ImColor(255, 255, 255, (int)(15.f * hov)), seg_r);
                draw->rect(drawlist, seg_min, seg_max, ImColor(255, 255, 255, (int)(40.f * hov)), seg_r);
                draw->text_clipped(drawlist, font->get(inter_medium_data, 12), seg_min, seg_max, ImColor(220, 230, 245, 255), labels[i], NULL, NULL, ImVec2(0.5f, 0.5f));
            }
            else
            {
                draw->text_clipped(drawlist, font->get(inter_medium_data, 12), seg_min, seg_max, ImColor(70, 85, 105, 140), labels[i], NULL, NULL, ImVec2(0.5f, 0.5f));
            }
        }
    }

    return data;
}