#include "../headers/functions.h"
#include "../headers/widgets.h"
#include <cmath>
#include <type_traits>

template <typename T>
widget_t slider_ex(std::string_view name, T* callback, T min, T max, std::string_view format, bool enabled = true)
{
    struct anim_t
    {
        float offset{ 0 };
    };

    float track_h = SCALE(elements->slider.size.y);
    float badge_h = SCALE(18.f);
    float total_h = track_h + badge_h + SCALE(4.f);

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, total_h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    const ImRect rect{ ImVec2(data->rect.Min.x, data->rect.Max.y - track_h), data->rect.Max };

    bool hovered = false, held = false;
    bool pressed = false;
    if (enabled)
    {
        pressed = gui->button_behavior(rect, data->id, &hovered, &held);
    }

    const float padding = SCALE(elements->slider.padding);
    const float grab_width = SCALE(elements->slider.radius * 2);
    const float width_with_grab = rect.GetWidth() - grab_width - padding * 2;
    const float grab_offset = padding + grab_width / 2 + static_cast<float>((*callback - min)) / static_cast<float>(max - min) * width_with_grab;

    char value_buf[64]; const char* value_buf_end = gui->get_fmt(value_buf, callback, format);
    const float t = ImSaturate((gui->mouse_pos().x - (rect.Min.x + padding + grab_width / 2)) / width_with_grab);

    data->value_changed = enabled && held;

    if (data->value_changed)
    {
        *callback += static_cast<T>(((min + t * (max - min)) - *callback));
        *callback = ImClamp(*callback, min, max);
    }

    gui->easing(anim->offset, grab_offset, 20.f, dynamic_easing);

    // Track Background
    draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->widgets.background), rect.GetHeight() / 2);
    
    if (enabled)
    {
        // Active track in vibrant Ice Cyan
        draw->rect_filled(gui->window_drawlist(), rect.Min, ImVec2(rect.Min.x + anim->offset + SCALE(elements->slider.radius + elements->slider.padding), rect.Max.y), ImColor(0, 242, 254), rect.GetHeight() / 2);
        
        // Knob with subtle cyan outer glow and pure white center
        ImVec2 knob_center = ImVec2(rect.Min.x + anim->offset, rect.GetCenter().y);
        draw->circle_filled(gui->window_drawlist(), knob_center, SCALE(elements->slider.radius + 2.f), ImColor(0, 242, 254, 45), 30);
        draw->circle_filled(gui->window_drawlist(), knob_center, SCALE(elements->slider.radius), ImColor(0, 242, 254), 30);
        draw->circle_filled(gui->window_drawlist(), knob_center, SCALE(elements->slider.radius * 0.5f), IM_COL32_WHITE, 30);

        // Label on the left above track
        ImVec2 label_max = ImVec2(data->rect.Min.x + SCALE(140.f), rect.Min.y);
        draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, label_max, draw->get_clr(clr->widgets.text_inactive), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

        // Floating value badge pill [ 12.0 ] positioned directly above knob (from screenshot)
        ImVec2 val_size = gui->text_size(font->get(inter_semibold_data, 11), value_buf);
        float badge_w = val_size.x + SCALE(14.f);
        float knob_x = knob_center.x;
        ImVec2 badge_min = ImVec2(knob_x - badge_w * 0.5f, rect.Min.y - badge_h - SCALE(3.f));
        ImVec2 badge_max = ImVec2(knob_x + badge_w * 0.5f, rect.Min.y - SCALE(3.f));

        if (badge_min.x < data->rect.Min.x) {
            float shift = data->rect.Min.x - badge_min.x;
            badge_min.x += shift; badge_max.x += shift;
        }
        if (badge_max.x > data->rect.Max.x) {
            float shift = badge_max.x - data->rect.Max.x;
            badge_min.x -= shift; badge_max.x -= shift;
        }

        draw->rect_filled(gui->window_drawlist(), badge_min, badge_max, draw->get_clr(clr->widgets.background), SCALE(4.f));
        draw->rect(gui->window_drawlist(), badge_min, badge_max, draw->get_clr(clr->window.stroke), SCALE(4.f));
        draw->text_clipped(gui->window_drawlist(), font->get(inter_semibold_data, 11), badge_min, badge_max, draw->get_clr(clr->widgets.text), value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

        // Downward caret pointer
        ImVec2 p1 = ImVec2(knob_x - SCALE(3.f), badge_max.y);
        ImVec2 p2 = ImVec2(knob_x + SCALE(3.f), badge_max.y);
        ImVec2 p3 = ImVec2(knob_x, badge_max.y + SCALE(3.f));
        gui->window_drawlist()->AddTriangleFilled(p1, p2, p3, draw->get_clr(clr->widgets.background));
    }
    else
    {
        // Disabled / Inactive: keeps full size without reshaping, rendered dimmed
        draw->rect_filled(gui->window_drawlist(), rect.Min, ImVec2(rect.Min.x + anim->offset + SCALE(elements->slider.radius + elements->slider.padding), rect.Max.y), ImColor(55, 68, 85, 80), rect.GetHeight() / 2);
        
        ImVec2 knob_center = ImVec2(rect.Min.x + anim->offset, rect.GetCenter().y);
        draw->circle_filled(gui->window_drawlist(), knob_center, SCALE(elements->slider.radius), ImColor(65, 80, 100, 120), 30);
        draw->circle_filled(gui->window_drawlist(), knob_center, SCALE(elements->slider.radius * 0.5f), ImColor(90, 105, 130, 160), 30);

        ImVec2 label_max = ImVec2(data->rect.Min.x + SCALE(140.f), rect.Min.y);
        draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, label_max, ImColor(70, 85, 105, 140), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

        ImVec2 val_size = gui->text_size(font->get(inter_semibold_data, 11), value_buf);
        float badge_w = val_size.x + SCALE(14.f);
        float knob_x = knob_center.x;
        ImVec2 badge_min = ImVec2(knob_x - badge_w * 0.5f, rect.Min.y - badge_h - SCALE(3.f));
        ImVec2 badge_max = ImVec2(knob_x + badge_w * 0.5f, rect.Min.y - SCALE(3.f));

        if (badge_min.x < data->rect.Min.x) {
            float shift = data->rect.Min.x - badge_min.x;
            badge_min.x += shift; badge_max.x += shift;
        }
        if (badge_max.x > data->rect.Max.x) {
            float shift = badge_max.x - data->rect.Max.x;
            badge_min.x -= shift; badge_max.x -= shift;
        }

        draw->rect_filled(gui->window_drawlist(), badge_min, badge_max, draw->get_clr(clr->widgets.background), SCALE(4.f));
        draw->rect(gui->window_drawlist(), badge_min, badge_max, ImColor(45, 55, 75, 100), SCALE(4.f));
        draw->text_clipped(gui->window_drawlist(), font->get(inter_semibold_data, 11), badge_min, badge_max, ImColor(70, 85, 105, 150), value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

        ImVec2 p1 = ImVec2(knob_x - SCALE(3.f), badge_max.y);
        ImVec2 p2 = ImVec2(knob_x + SCALE(3.f), badge_max.y);
        ImVec2 p3 = ImVec2(knob_x, badge_max.y + SCALE(3.f));
        gui->window_drawlist()->AddTriangleFilled(p1, p2, p3, draw->get_clr(clr->widgets.background));
    }

    return data;
}

widget_t c_widgets::slider_int(std::string_view key, bool enabled)
{
    slider_int_t* data = cfg->fill<slider_int_t>(key.data());
    return slider_ex(data->name, &data->callback, data->min, data->max, data->format, enabled);
}

widget_t c_widgets::slider_float(std::string_view key, std::string_view custom_label)
{
    slider_float_t* data = cfg->fill<slider_float_t>(key.data());
    return slider_ex(custom_label.empty() ? data->name : custom_label, &data->callback, data->min, data->max, data->format);
}

widget_t c_widgets::inline_slider(std::string_view label, std::string_view key_slider, bool enabled)
{
    struct anim_t
    {
        float grab_ratio{ -1.f };
        float alpha{ 0.f };
    };

    slider_int_t* sl_data = cfg->fill<slider_int_t>(key_slider.data());
    if (!sl_data) return widget_t{};

    float total_w = gui->content_avail().x;
    float row_h = SCALE(elements->checkbox.size.y);

    widget_t data = gui->register_item(std::string(label) + key_slider.data(), ImVec2(total_w, row_h));
    anim_t* anim = gui->anim_container<anim_t>(data->id);

    ImFont* lbl_font = font->get(inter_medium_data, 13);
    ImVec2 txt_sz = gui->text_size(lbl_font, label.data());

    char val_buf[64];
    gui->get_fmt(val_buf, &sl_data->callback, sl_data->format);
    ImFont* val_font = font->get(inter_semibold_data, 11);
    ImVec2 val_sz = gui->text_size(val_font, val_buf);
    float val_w = val_sz.x + SCALE(6.f);

    float text_right_x = data->rect.Min.x + txt_sz.x;
    float adj_left_x = text_right_x + SCALE(14.f);
    float adj_right_x = data->rect.Max.x;
    float adj_w = adj_right_x - adj_left_x;

    float track_w = adj_w - val_w - SCALE(10.f);
    if (track_w < SCALE(20.f)) track_w = SCALE(20.f);

    float track_h = SCALE(3.5f);
    float track_y = data->rect.GetCenter().y;
    ImRect track_rect(ImVec2(adj_left_x, track_y - track_h * 0.5f), ImVec2(adj_left_x + track_w, track_y + track_h * 0.5f));
    ImRect slider_interactive_rect(ImVec2(track_rect.Min.x - SCALE(8.f), data->rect.Min.y), ImVec2(track_rect.Max.x + SCALE(8.f), data->rect.Max.y));

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = gui->get_window();
    ImGuiID slider_id = window->GetID((std::string(key_slider) + "_inline").c_str());

    bool slider_held = false;

    // Do not activate or drag slider if the application window itself is being moved
    if (g.MovingWindow != NULL)
    {
        if (g.ActiveId == slider_id)
            ImGui::ClearActiveID();
        slider_held = false;
    }
    else if (enabled)
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
                sl_data->callback = sl_data->min + static_cast<int>(t * (sl_data->max - sl_data->min) + 0.5f);
                sl_data->callback = ImClamp(sl_data->callback, sl_data->min, sl_data->max);
                data->value_changed = true;

                gui->get_fmt(val_buf, &sl_data->callback, sl_data->format);
            }
            else
            {
                ImGui::ClearActiveID();
                slider_held = false;
            }
        }
    }

    gui->easing(anim->alpha, enabled ? 1.f : 0.f, 10.f, dynamic_easing);

    // Use relative 0..1 ratio instead of absolute screen coordinates so moving the window never displaces the knob
    float t_val = (sl_data->max > sl_data->min) ? (float)(sl_data->callback - sl_data->min) / (float)(sl_data->max - sl_data->min) : 0.f;
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

    ImDrawList* drawlist = gui->window_drawlist();

    // 1. Label on the left
    ImVec4 text_col = enabled ? clr->widgets.text.Value : clr->widgets.text_inactive.Value;
    draw->text_clipped(drawlist, lbl_font, data->rect.Min, ImVec2(text_right_x + SCALE(4.f), data->rect.Max.y), draw->get_clr(text_col), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    // 2. Adjuster Track & Knob on SAME LINE (strictly relative to track_rect)
    float cur_knob_x = track_rect.Min.x + ImClamp(anim->grab_ratio, 0.f, 1.f) * track_rect.GetWidth();

    if (enabled)
    {
        // Active dark track
        draw->rect_filled(drawlist, track_rect.Min, track_rect.Max, draw->get_clr(clr->widgets.background), track_h * 0.5f);
        draw->rect(drawlist, track_rect.Min, track_rect.Max, draw->get_clr(clr->window.stroke, 0.6f), track_h * 0.5f);

        // Neon cyan progress
        draw->rect_filled(drawlist, track_rect.Min, ImVec2(cur_knob_x, track_rect.Max.y), ImColor(0, 242, 254), track_h * 0.5f);

        // Knob with soft glow
        float knob_r = slider_held ? SCALE(6.f) : SCALE(5.f);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r + SCALE(2.5f), ImColor(0, 242, 254, slider_held ? 80 : 45), 20);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r, ImColor(0, 242, 254), 20);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r * 0.5f, IM_COL32_WHITE, 20);

        // Value text
        ImVec2 val_min(track_rect.Max.x + SCALE(8.f), data->rect.Min.y);
        ImVec2 val_max(adj_right_x, data->rect.Max.y);
        draw->text_clipped(drawlist, val_font, val_min, val_max, ImColor(0, 242, 254, 240), val_buf, NULL, NULL, ImVec2(0.5f, 0.5f));
    }
    else
    {
        // Dimmed / Disabled state (occupies exact same line)
        draw->rect_filled(drawlist, track_rect.Min, track_rect.Max, draw->get_clr(clr->widgets.background), track_h * 0.5f);
        draw->rect(drawlist, track_rect.Min, track_rect.Max, ImColor(45, 55, 75, 100), track_h * 0.5f);

        draw->rect_filled(drawlist, track_rect.Min, ImVec2(cur_knob_x, track_rect.Max.y), ImColor(55, 68, 85, 80), track_h * 0.5f);

        float knob_r = SCALE(5.f);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r, ImColor(65, 80, 100, 120), 20);
        draw->circle_filled(drawlist, ImVec2(cur_knob_x, track_y), knob_r * 0.5f, ImColor(90, 105, 130, 160), 20);

        ImVec2 val_min(track_rect.Max.x + SCALE(8.f), data->rect.Min.y);
        ImVec2 val_max(adj_right_x, data->rect.Max.y);
        draw->text_clipped(drawlist, val_font, val_min, val_max, ImColor(70, 85, 105, 140), val_buf, NULL, NULL, ImVec2(0.5f, 0.5f));
    }

    return data;
}

// -----------------------------------------------------------------------------
// Status Badge: [ ENABLED ] / [ DISABLED ] pill with glowing cyan border
// -----------------------------------------------------------------------------
widget_t c_widgets::status_badge(std::string_view name, bool active)
{
    const char* text = active ? "ENABLED" : "DISABLED";
    ImFont* badge_font = font->get(inter_semibold_data, 11);
    ImVec2 txt_sz = gui->text_size(badge_font, text);
    float pad_x = SCALE(12.f);
    float pad_y = SCALE(4.f);
    ImVec2 badge_size(txt_sz.x + pad_x * 2.f, txt_sz.y + pad_y * 2.f);

    widget_t data = gui->register_item(name, badge_size);
    ImDrawList* drawlist = gui->window_drawlist();
    float rounding = data->rect.GetHeight() * 0.5f;

    if (active)
    {
        drawlist->AddRectFilled(data->rect.Min, data->rect.Max, ImColor(0, 242, 254, 25), rounding);
        drawlist->AddRect(data->rect.Min, data->rect.Max, ImColor(0, 242, 254, 180), rounding, 0, 1.2f);
        draw->text_clipped(drawlist, badge_font, data->rect.Min, data->rect.Max, ImColor(0, 242, 254), text, NULL, NULL, ImVec2(0.5f, 0.5f));
    }
    else
    {
        drawlist->AddRectFilled(data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.background), rounding);
        drawlist->AddRect(data->rect.Min, data->rect.Max, draw->get_clr(clr->window.stroke), rounding, 0, 1.0f);
        draw->text_clipped(drawlist, badge_font, data->rect.Min, data->rect.Max, draw->get_clr(clr->widgets.text_inactive), text, NULL, NULL, ImVec2(0.5f, 0.5f));
    }

    return data;
}

extern bool text_field_ex(std::string_view label, std::string_view hint, char* buf, int buf_size, float text_padding, float line_padding, ImU32 text_col, bool centered, const ImRect& rect, bool* active, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data, bool fg = false);

// -----------------------------------------------------------------------------
// Circular Radial Arc FOV Increaser (from screenshot)
// -----------------------------------------------------------------------------
template <typename T>
static widget_t radial_fov_template(std::string_view name, T* callback, T min, T max, const char* unit, float custom_radius, const char* sub_label, const char* fmt_str, bool title_at_bottom = false, float custom_width = 0.f)
{
    float radius = (custom_radius > 0.f) ? custom_radius : SCALE(34.f);
    float gauge_w = (custom_width > 0.f) ? custom_width : (title_at_bottom ? gui->content_avail().x : (radius * 2.f + SCALE(16.f)));
    float gauge_h = title_at_bottom ? (radius * 2.f + SCALE(26.f)) : (radius * 2.f + SCALE(16.f));

    widget_t data = gui->register_item(name, ImVec2(gauge_w, gauge_h));
    
    bool hovered = data->hovered;
    bool held = false;
    ImRect circle_rect(data->rect.Min, data->rect.Max);
    gui->button_behavior(circle_rect, data->id, &hovered, &held);

    ImVec2 center = title_at_bottom ? ImVec2(data->rect.Min.x + data->rect.GetWidth() * 0.5f, data->rect.Min.y + radius + SCALE(3.f)) : data->rect.GetCenter();
    const float min_angle = -225.0f * (3.1415926535f / 180.0f);
    const float max_angle = 45.0f * (3.1415926535f / 180.0f);
    const float sweep = max_angle - min_angle;

    ImVec2 delta = gui->mouse_pos() - center;
    float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);

    bool clicked = hovered && (gui->mouse_clicked(mouse_button_left) || ImGui::IsMouseClicked(ImGuiMouseButton_Left));
    bool right_clicked = hovered && (gui->mouse_clicked(mouse_button_right) || ImGui::IsMouseClicked(ImGuiMouseButton_Right));
    bool double_clicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    struct radial_state_t {
        float angle{ 0.f };
        bool is_dragging{ false };
        bool is_resetting{ false };
    };
    radial_state_t* anim = gui->anim_container<radial_state_t>(data->id);

    // Differentiate adjuster arc ring vs text/value/title
    bool is_on_arc = (dist >= radius * 0.68f && dist <= radius + SCALE(14.f) && !(delta.y > radius * 0.72f && fabsf(delta.x) < radius * 0.65f));

    if (right_clicked || double_clicked)
    {
        *callback = static_cast<T>(0);
        data->value_changed = true;
        anim->is_dragging = false;
        anim->is_resetting = true;
    }
    else if (clicked)
    {
        if (is_on_arc)
        {
            anim->is_dragging = true;
            anim->is_resetting = false;

            float ang = atan2f(delta.y, delta.x);
            float diff = ang - min_angle;
            while (diff < 0.0f) diff += 6.28318530718f;
            while (diff >= 6.28318530718f) diff -= 6.28318530718f;
            float t = ImClamp(diff / sweep, 0.0f, 1.0f);
            if constexpr (std::is_integral_v<T>)
                *callback = min + static_cast<T>(t * (max - min) + 0.5f);
            else
            {
                *callback = min + static_cast<T>(t * (max - min));
                if (fmt_str && strstr(fmt_str, ".1f"))
                    *callback = roundf(*callback * 10.0f) * 0.1f;
            }
            *callback = ImClamp(*callback, min, max);
            data->value_changed = true;
        }
        else
        {
            // Clicked directly on value text, title text, or outside arc -> instantly reset to zero!
            *callback = static_cast<T>(0);
            data->value_changed = true;
            anim->is_dragging = false;
            anim->is_resetting = true;
        }
    }

    if (anim->is_dragging && held && !anim->is_resetting)
    {
        float ang = atan2f(delta.y, delta.x);
        float diff = ang - min_angle;
        while (diff < 0.0f) diff += 6.28318530718f;
        while (diff >= 6.28318530718f) diff -= 6.28318530718f;
        float t = ImClamp(diff / sweep, 0.0f, 1.0f);
        if constexpr (std::is_integral_v<T>)
            *callback = min + static_cast<T>(t * (max - min) + 0.5f);
        else
        {
            *callback = min + static_cast<T>(t * (max - min));
            if (fmt_str && strstr(fmt_str, ".1f"))
                *callback = roundf(*callback * 10.0f) * 0.1f;
        }
        *callback = ImClamp(*callback, min, max);
        data->value_changed = true;
    }

    if (!gui->mouse_down(mouse_button_left) && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        anim->is_dragging = false;
        anim->is_resetting = false;
    }

    float target_t = ImClamp((float)(*callback - min) / (float)(max - min), 0.0f, 1.0f);
    float target_angle = min_angle + target_t * sweep;
    gui->easing(anim->angle, target_angle, 20.f, dynamic_easing);

    ImDrawList* drawlist = gui->window_drawlist();
    float thickness = SCALE(4.5f);

    // Track arc
    drawlist->PathArcTo(center, radius, min_angle, max_angle, 40);
    drawlist->PathStroke(ImColor(28, 35, 48), false, thickness);

    // Active cyan arc with glow
    if (anim->angle > min_angle + 0.02f)
    {
        drawlist->PathArcTo(center, radius, min_angle, anim->angle, 32);
        drawlist->PathStroke(ImColor(0, 242, 254, 45), false, thickness + SCALE(4.f));

        drawlist->PathArcTo(center, radius, min_angle, anim->angle, 32);
        drawlist->PathStroke(ImColor(0, 242, 254, 255), false, thickness);

        ImVec2 knob = ImVec2(center.x + cosf(anim->angle) * radius, center.y + sinf(anim->angle) * radius);
        drawlist->AddCircleFilled(knob, SCALE(6.5f), ImColor(0, 242, 254, 50), 24);
        drawlist->AddCircleFilled(knob, SCALE(4.5f), ImColor(0, 242, 254), 24);
        drawlist->AddCircleFilled(knob, SCALE(2.5f), IM_COL32_WHITE, 24);
    }

    char val_str[32];
    if constexpr (std::is_integral_v<T>)
        sprintf_s(val_str, fmt_str ? fmt_str : "%d%s", *callback, unit);
    else
        sprintf_s(val_str, fmt_str ? fmt_str : "%.0f%s", *callback, unit);

    ImFont* val_font = font->get(inter_semibold_data, 15);
    ImVec2 val_sz = gui->text_size(val_font, val_str);

    if (title_at_bottom)
    {
        // Value centered inside dial
        drawlist->AddText(val_font, SCALE(15.f), center - ImVec2(val_sz.x * 0.5f, val_sz.y * 0.5f), IM_COL32_WHITE, val_str);

        // Title at bottom with same text size (inter_medium_data, 13)
        if (sub_label && sub_label[0])
        {
            ImFont* title_font = font->get(inter_medium_data, 13);
            ImVec2 title_sz = gui->text_size(title_font, sub_label);
            ImVec2 title_pos = ImVec2(center.x - title_sz.x * 0.5f, center.y + radius + SCALE(4.f));
            drawlist->AddText(title_font, SCALE(13.f), title_pos, draw->get_clr(clr->widgets.text_inactive), sub_label);
        }
    }
    else
    {
        drawlist->AddText(val_font, SCALE(15.f), center - ImVec2(val_sz.x * 0.5f, val_sz.y * 0.8f), IM_COL32_WHITE, val_str);

        ImFont* sub_font = font->get(inter_medium_data, 10);
        const char* sub_str = sub_label ? sub_label : "FOV";
        ImVec2 sub_sz = gui->text_size(sub_font, sub_str);
        drawlist->AddText(sub_font, SCALE(10.f), center + ImVec2(-sub_sz.x * 0.5f, SCALE(2.f)), ImColor(100, 116, 139), sub_str);
    }

    return data;
}

widget_t c_widgets::radial_fov(std::string_view name, int* callback, int min, int max, const char* unit, float custom_radius, const char* sub_label, bool title_at_bottom, float custom_width)
{
    return radial_fov_template<int>(name, callback, min, max, unit, custom_radius, sub_label, "%d%s", title_at_bottom, custom_width);
}

widget_t c_widgets::radial_fov(std::string_view name, float* callback, float min, float max, const char* unit, float custom_radius, const char* sub_label, bool title_at_bottom, const char* fmt_str, float custom_width)
{
    return radial_fov_template<float>(name, callback, min, max, unit, custom_radius, sub_label, fmt_str ? fmt_str : "%.0f%s", title_at_bottom, custom_width);
}

// -----------------------------------------------------------------------------
// Interactive Stepper Widget: [ - ]  [ 85 ]  [ + ] (Direct Input Box!)
// -----------------------------------------------------------------------------
widget_t c_widgets::stepper_int(std::string_view name, int* callback, int min, int max, int step, const char* format, float custom_width)
{
    float total_w = (custom_width > 0.f) ? custom_width : SCALE(110.f);
    float btn_w = SCALE(24.f);
    float h = SCALE(elements->slider.size.y + 4.f);

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, h));

    ImRect stepper_rect(ImVec2(data->rect.Max.x - total_w, data->rect.Min.y), data->rect.Max);
    ImRect minus_rect(stepper_rect.Min, ImVec2(stepper_rect.Min.x + btn_w, stepper_rect.Max.y));
    ImRect plus_rect(ImVec2(stepper_rect.Max.x - btn_w, stepper_rect.Min.y), stepper_rect.Max);
    ImRect val_rect(ImVec2(minus_rect.Max.x + SCALE(2.f), stepper_rect.Min.y), ImVec2(plus_rect.Min.x - SCALE(2.f), stepper_rect.Max.y));

    bool minus_hover = minus_rect.Contains(gui->mouse_pos());
    bool plus_hover = plus_rect.Contains(gui->mouse_pos());
    bool val_hover = val_rect.Contains(gui->mouse_pos());

    struct stepper_state_t
    {
        char buf[32]{ 0 };
        bool editing{ false };
        int last_val{ -999999 };
    };
    ImGuiID val_id = gui->get_window()->GetID((std::string(name) + "_val_box").c_str());
    stepper_state_t* s_state = gui->anim_container<stepper_state_t>(val_id);

    if (minus_hover && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())
    {
        *callback = ImClamp(*callback - step, min, max);
        sprintf_s(s_state->buf, "%d", *callback);
        s_state->last_val = *callback;
        s_state->editing = false;
        data->value_changed = true;
    }
    if (plus_hover && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())
    {
        *callback = ImClamp(*callback + step, min, max);
        sprintf_s(s_state->buf, "%d", *callback);
        s_state->last_val = *callback;
        s_state->editing = false;
        data->value_changed = true;
    }

    if (!s_state->editing && s_state->last_val != *callback)
    {
        sprintf_s(s_state->buf, "%d", *callback);
        s_state->last_val = *callback;
    }

    ImDrawList* drawlist = gui->window_drawlist();
    float r = SCALE(4.f);

    // Label on the left
    draw->text_clipped(drawlist, font->get(inter_medium_data, 13), data->rect.Min, stepper_rect.GetBL(), draw->get_clr(clr->widgets.text_inactive), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));

    // Minus Button
    draw->rect_filled(drawlist, minus_rect.Min, minus_rect.Max, draw->get_clr(clr->widgets.background), r);
    draw->rect(drawlist, minus_rect.Min, minus_rect.Max, minus_hover ? IM_COL32(0, 242, 254, 180) : draw->get_clr(clr->window.stroke), r);
    draw->text_clipped(drawlist, font->get(inter_semibold_data, 13), minus_rect.Min, minus_rect.Max, draw->get_clr(clr->widgets.text), "-", NULL, NULL, ImVec2(0.5f, 0.5f));

    // Plus Button
    draw->rect_filled(drawlist, plus_rect.Min, plus_rect.Max, draw->get_clr(clr->widgets.background), r);
    draw->rect(drawlist, plus_rect.Min, plus_rect.Max, plus_hover ? IM_COL32(0, 242, 254, 180) : draw->get_clr(clr->window.stroke), r);
    draw->text_clipped(drawlist, font->get(inter_semibold_data, 13), plus_rect.Min, plus_rect.Max, draw->get_clr(clr->widgets.text), "+", NULL, NULL, ImVec2(0.5f, 0.5f));

    // Center Value Box: Interactive Live Text Input Box with ms unit display!
    draw->rect_filled(drawlist, val_rect.Min, val_rect.Max, draw->get_clr(clr->widgets.background), r);
    draw->rect(drawlist, val_rect.Min, val_rect.Max, (val_hover || s_state->editing) ? IM_COL32(0, 242, 254, 200) : draw->get_clr(clr->window.stroke), r);

    if (!s_state->editing && val_hover && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered())
    {
        s_state->editing = true;
        sprintf_s(s_state->buf, "%d", *callback);
        ImGui::SetKeyboardFocusHere();
    }

    if (s_state->editing)
    {
        gui->push_font(font->get(inter_semibold_data, 12));
        gui->push_var(style_var_frame_rounding, r);
        text_field_ex((std::string(name) + "_input").c_str(), "", s_state->buf, sizeof(s_state->buf), SCALE(2.f), SCALE(2.f), (val_hover || s_state->editing) ? IM_COL32(0, 242, 254, 255) : IM_COL32_WHITE, true, val_rect, &s_state->editing, ImGuiInputTextFlags_CharsDecimal, NULL, NULL);
        gui->pop_var();
        gui->pop_font();

        if (s_state->buf[0] != '\0')
        {
            int parsed = atoi(s_state->buf);
            *callback = ImClamp(parsed, min, max);
            s_state->last_val = *callback;
            data->value_changed = true;
        }
    }
    else
    {
        char display_str[32];
        if (format && format[0])
            sprintf_s(display_str, format, *callback);
        else
            sprintf_s(display_str, "%dms", *callback);

        ImU32 text_col = val_hover ? IM_COL32(0, 242, 254, 255) : IM_COL32(230, 240, 255, 255);
        draw->text_clipped(drawlist, font->get(inter_semibold_data, 12), val_rect.Min, val_rect.Max, text_col, display_str, NULL, NULL, ImVec2(0.5f, 0.5f));
        s_state->last_val = *callback;
    }

    return data;
}

// -----------------------------------------------------------------------------
// Interactive Vector Anatomical Mannequin Body Selector
// -----------------------------------------------------------------------------
widget_t c_widgets::skeleton_selector(std::string_view name, multi_dropdown_t* bones, const ImVec2& custom_size, bool show_labels)
{
    float w = (custom_size.x > 0.f) ? custom_size.x : SCALE(150.f);
    float h = (custom_size.y > 0.f) ? custom_size.y : SCALE(100.f);
    widget_t data = gui->register_item(name, ImVec2(w, h));
    ImDrawList* drawlist = gui->window_drawlist();

    ImVec2 center = data->rect.GetCenter();
    float cx = show_labels ? (center.x - SCALE(8.f)) : center.x;
    float cy = show_labels ? (center.y) : (data->rect.Min.y + SCALE(48.f));
    float s  = SCALE(0.82f);

    // Anatomical Bone Keypoints
    ImVec2 head_pt (cx, cy - 36.f * s);
    ImVec2 neck_pt (cx, cy - 22.f * s);
    ImVec2 chest_pt(cx, cy - 9.f * s);
    ImVec2 groin_pt(cx, cy + 18.f * s);

    auto is_bone_active = [&](int idx) -> bool {
        return (bones && idx < (int)bones->callback.size()) ? bones->callback[idx] : false;
    };

    bool head_active  = is_bone_active(0);
    bool neck_active  = is_bone_active(1);
    bool chest_active = is_bone_active(2);
    bool legs_active  = is_bone_active(3) || is_bone_active(4);
    bool arms_active  = is_bone_active(5) || is_bone_active(6);

    // Palette Colors
    ImU32 col_dim_wire    = IM_COL32(65, 80, 105, 95);
    ImU32 col_dim_fill    = IM_COL32(20, 26, 38, 70);
    ImU32 col_active_glow = IM_COL32(0, 242, 254, 45);
    ImU32 col_active_neon = IM_COL32(0, 242, 254, 255);
    ImU32 col_active_soft = IM_COL32(0, 242, 254, 180);

    // -------------------------------------------------------------
    // 0. Background Subtle Cyber Axis & Scan Guides
    // -------------------------------------------------------------
    ImU32 col_grid = IM_COL32(0, 242, 254, 12);
    drawlist->AddLine(ImVec2(cx - 36.f * s, cy), ImVec2(cx + 36.f * s, cy), col_grid, 1.0f);
    drawlist->AddLine(ImVec2(cx, cy - 44.f * s), ImVec2(cx, cy + 50.f * s), col_grid, 1.0f);

    // -------------------------------------------------------------
    // 1. ARMS & LIMBS (Connected to indices 5 & 6)
    // -------------------------------------------------------------
    ImVec2 l_sh(cx - 24.f * s, cy - 18.f * s);
    ImVec2 r_sh(cx + 24.f * s, cy - 18.f * s);
    ImVec2 l_elb(cx - 27.f * s, cy + 3.f * s);
    ImVec2 r_elb(cx + 27.f * s, cy + 3.f * s);
    ImVec2 l_wri(cx - 25.f * s, cy + 22.f * s);
    ImVec2 r_wri(cx + 25.f * s, cy + 22.f * s);

    ImU32 arm_c = arms_active ? col_active_neon : col_dim_wire;
    ImU32 arm_glow = arms_active ? col_active_glow : IM_COL32(65, 80, 105, 45);

    // Left Arm (bicep + forearm)
    drawlist->AddLine(l_sh, l_elb, arm_c, 1.4f);
    drawlist->AddLine(l_sh - ImVec2(2.f * s, 0), l_elb - ImVec2(2.f * s, 0), arm_glow, 1.0f);
    drawlist->AddLine(l_elb, l_wri, arm_c, 1.2f);
    drawlist->AddCircleFilled(l_elb, 1.8f * s, arm_c);
    drawlist->AddCircleFilled(l_wri, 2.2f * s, arm_c);

    // Right Arm (bicep + forearm)
    drawlist->AddLine(r_sh, r_elb, arm_c, 1.4f);
    drawlist->AddLine(r_sh + ImVec2(2.f * s, 0), r_elb + ImVec2(2.f * s, 0), arm_glow, 1.0f);
    drawlist->AddLine(r_elb, r_wri, arm_c, 1.2f);
    drawlist->AddCircleFilled(r_elb, 1.8f * s, arm_c);
    drawlist->AddCircleFilled(r_wri, 2.2f * s, arm_c);

    // -------------------------------------------------------------
    // 2. LEGS & LOWER BODY (Connected to indices 3 & 4)
    // -------------------------------------------------------------
    ImVec2 l_hip(cx - 13.f * s, cy + 18.f * s);
    ImVec2 r_hip(cx + 13.f * s, cy + 18.f * s);
    ImVec2 l_knee(cx - 10.f * s, cy + 38.f * s);
    ImVec2 r_knee(cx + 10.f * s, cy + 38.f * s);
    ImVec2 l_foot(cx - 9.f * s, cy + 50.f * s);
    ImVec2 r_foot(cx + 9.f * s, cy + 50.f * s);

    ImU32 leg_c = legs_active ? col_active_neon : col_dim_wire;
    ImU32 leg_glow = legs_active ? col_active_glow : IM_COL32(65, 80, 105, 55);

    // Thighs (outer and inner muscle lines)
    drawlist->AddLine(l_hip, l_knee, leg_c, 1.4f);
    drawlist->AddLine(r_hip, r_knee, leg_c, 1.4f);
    drawlist->AddLine(ImVec2(cx - 2.5f * s, cy + 24.f * s), ImVec2(cx - 6.5f * s, cy + 38.f * s), leg_glow, 1.0f);
    drawlist->AddLine(ImVec2(cx + 2.5f * s, cy + 24.f * s), ImVec2(cx + 6.5f * s, cy + 38.f * s), leg_glow, 1.0f);

    // Knee joints
    drawlist->AddCircle(l_knee, 2.2f * s, leg_c, 12, 1.0f);
    drawlist->AddCircle(r_knee, 2.2f * s, leg_c, 12, 1.0f);

    // Calves
    drawlist->AddLine(l_knee, l_foot, leg_c, 1.2f);
    drawlist->AddLine(r_knee, r_foot, leg_c, 1.2f);

    // -------------------------------------------------------------
    // 3. WAIST & PELVIC ARCH (Harmonized with Legs)
    // -------------------------------------------------------------
    ImU32 groin_line_c = legs_active ? col_active_neon : col_dim_wire;
    ImU32 groin_fill_c = legs_active ? col_active_glow : col_dim_fill;

    ImVec2 pubic_pt(cx, cy + 25.f * s);
    ImVec2 l_waist(cx - 10.f * s, cy + 11.f * s);
    ImVec2 r_waist(cx + 10.f * s, cy + 11.f * s);

    drawlist->AddTriangleFilled(l_hip, pubic_pt, r_hip, groin_fill_c);
    drawlist->AddQuadFilled(l_waist, r_waist, r_hip, l_hip, groin_fill_c);

    drawlist->AddLine(l_waist, l_hip, groin_line_c, 1.2f);
    drawlist->AddLine(r_waist, r_hip, groin_line_c, 1.2f);
    drawlist->AddLine(l_hip, pubic_pt, groin_line_c, 1.3f);
    drawlist->AddLine(r_hip, pubic_pt, groin_line_c, 1.3f);
    drawlist->AddLine(l_hip, r_hip, legs_active ? col_active_soft : IM_COL32(65, 80, 105, 60), 1.0f);

    // -------------------------------------------------------------
    // 4. CHEST & TORSO (Bone Index 2)
    // -------------------------------------------------------------
    ImU32 chest_line_c = chest_active ? col_active_neon : col_dim_wire;
    ImU32 chest_fill_c = chest_active ? col_active_glow : col_dim_fill;

    // Central spine
    drawlist->AddLine(ImVec2(cx, cy - 18.f * s), ImVec2(cx, cy + 11.f * s), chest_active ? col_active_soft : IM_COL32(65, 80, 105, 80), 1.2f);

    // Clavicles (collarbones)
    drawlist->AddLine(ImVec2(cx - 22.f * s, cy - 18.f * s), ImVec2(cx, cy - 17.f * s), chest_line_c, 1.3f);
    drawlist->AddLine(ImVec2(cx + 22.f * s, cy - 18.f * s), ImVec2(cx, cy - 17.f * s), chest_line_c, 1.3f);

    // Left Pectoral Plate
    ImVec2 lp1(cx - 2.5f * s, cy - 16.f * s);
    ImVec2 lp2(cx - 19.f * s, cy - 16.f * s);
    ImVec2 lp3(cx - 15.f * s, cy - 6.f * s);
    ImVec2 lp4(cx - 2.5f * s, cy - 7.f * s);
    drawlist->AddQuadFilled(lp1, lp2, lp3, lp4, chest_fill_c);
    drawlist->AddLine(lp1, lp2, chest_line_c, 1.2f);
    drawlist->AddLine(lp2, lp3, chest_line_c, 1.2f);
    drawlist->AddLine(lp3, lp4, chest_line_c, 1.2f);
    drawlist->AddLine(lp4, lp1, chest_line_c, 1.0f);

    // Right Pectoral Plate
    ImVec2 rp1(cx + 2.5f * s, cy - 16.f * s);
    ImVec2 rp2(cx + 19.f * s, cy - 16.f * s);
    ImVec2 rp3(cx + 15.f * s, cy - 6.f * s);
    ImVec2 rp4(cx + 2.5f * s, cy - 7.f * s);
    drawlist->AddQuadFilled(rp1, rp2, rp3, rp4, chest_fill_c);
    drawlist->AddLine(rp1, rp2, chest_line_c, 1.2f);
    drawlist->AddLine(rp2, rp3, chest_line_c, 1.2f);
    drawlist->AddLine(rp3, rp4, chest_line_c, 1.2f);
    drawlist->AddLine(rp4, rp1, chest_line_c, 1.0f);

    // Ribcage arcs
    drawlist->AddLine(ImVec2(cx - 15.f * s, cy - 4.f * s), ImVec2(cx - 11.f * s, cy + 2.f * s), chest_line_c, 1.0f);
    drawlist->AddLine(ImVec2(cx + 15.f * s, cy - 4.f * s), ImVec2(cx + 11.f * s, cy + 2.f * s), chest_line_c, 1.0f);
    drawlist->AddLine(ImVec2(cx - 13.f * s, cy + 3.f * s), ImVec2(cx - 9.f * s, cy + 8.f * s), chest_line_c, 1.0f);
    drawlist->AddLine(ImVec2(cx + 13.f * s, cy + 3.f * s), ImVec2(cx + 9.f * s, cy + 8.f * s), chest_line_c, 1.0f);

    // Torso outer flanks (ribs to waist)
    drawlist->AddLine(ImVec2(cx - 19.f * s, cy - 14.f * s), l_waist, chest_line_c, 1.2f);
    drawlist->AddLine(ImVec2(cx + 19.f * s, cy - 14.f * s), r_waist, chest_line_c, 1.2f);

    // -------------------------------------------------------------
    // 5. NECK & TRAPEZIUS (Bone Index 1)
    // -------------------------------------------------------------
    ImU32 neck_line_c = neck_active ? col_active_neon : col_dim_wire;
    ImU32 neck_fill_c = neck_active ? col_active_glow : col_dim_fill;

    // Neck column
    ImVec2 n_tl(cx - 3.8f * s, cy - 28.f * s);
    ImVec2 n_tr(cx + 3.8f * s, cy - 28.f * s);
    ImVec2 n_bl(cx - 4.8f * s, cy - 18.f * s);
    ImVec2 n_br(cx + 4.8f * s, cy - 18.f * s);
    drawlist->AddQuadFilled(n_tl, n_tr, n_br, n_bl, neck_fill_c);
    drawlist->AddLine(n_tl, n_bl, neck_line_c, 1.2f);
    drawlist->AddLine(n_tr, n_br, neck_line_c, 1.2f);

    // Trapezius muscle slope to shoulders
    drawlist->AddLine(ImVec2(cx - 4.f * s, cy - 26.f * s), l_sh, neck_line_c, 1.2f);
    drawlist->AddLine(ImVec2(cx + 4.f * s, cy - 26.f * s), r_sh, neck_line_c, 1.2f);

    // -------------------------------------------------------------
    // 6. HEAD & CRANIUM (Bone Index 0)
    // -------------------------------------------------------------
    ImU32 head_line_c = head_active ? col_active_neon : col_dim_wire;
    ImU32 head_fill_c = head_active ? col_active_glow : col_dim_fill;

    // Sculpted Skull: cranium + jaw
    float cranium_r = 7.5f * s;
    ImVec2 cranium_c(cx, cy - 36.5f * s);
    drawlist->AddCircleFilled(cranium_c, cranium_r, head_fill_c, 24);
    drawlist->AddCircle(cranium_c, cranium_r, head_line_c, 24, 1.3f);

    // Jawline tapering down to chin
    ImVec2 l_ear(cx - 6.5f * s, cy - 36.5f * s);
    ImVec2 r_ear(cx + 6.5f * s, cy - 36.5f * s);
    ImVec2 l_jaw(cx - 4.5f * s, cy - 31.0f * s);
    ImVec2 r_jaw(cx + 4.5f * s, cy - 31.0f * s);
    ImVec2 chin (cx, cy - 28.5f * s);

    drawlist->AddTriangleFilled(l_jaw, chin, r_jaw, head_fill_c);
    drawlist->AddLine(l_ear, l_jaw, head_line_c, 1.2f);
    drawlist->AddLine(r_ear, r_jaw, head_line_c, 1.2f);
    drawlist->AddLine(l_jaw, chin, head_line_c, 1.3f);
    drawlist->AddLine(r_jaw, chin, head_line_c, 1.3f);

    // Cyber visor slit across eyes
    ImU32 visor_c = head_active ? IM_COL32(220, 255, 255, 255) : IM_COL32(100, 120, 150, 150);
    drawlist->AddLine(ImVec2(cx - 4.5f * s, cy - 36.5f * s), ImVec2(cx + 4.5f * s, cy - 36.5f * s), visor_c, 1.5f);

    // -------------------------------------------------------------
    // 7. Interactive Targeting Nodes & Callouts
    // -------------------------------------------------------------
    struct target_node_t {
        int group; // 0: Head, 1: Neck, 2: Chest, 3: Legs (indices 3, 4), 4: Arms (indices 5, 6)
        const char* label;
        ImVec2 pt;
        ImVec2 label_pos;
        bool align_right;
    };

    target_node_t nodes[] = {
        { 0, "Head",  head_pt,  ImVec2(cx + 26.f * s, head_pt.y  - 6.f * s), true },
        { 1, "Neck",  neck_pt,  ImVec2(cx - 48.f * s, neck_pt.y  - 6.f * s), false },
        { 2, "Chest", chest_pt, ImVec2(cx + 26.f * s, chest_pt.y - 6.f * s), true },
        { 3, "Legs",  ImVec2(cx, cy + 32.f * s), ImVec2(cx + 26.f * s, cy + 30.f * s), true },
        { 4, "Arms",  r_wri,    ImVec2(cx + 28.f * s, r_wri.y - 6.f * s), true }
    };

    ImFont* lbl_font = font->get(inter_medium_data, 11);

    for (const auto& n : nodes)
    {
        bool active = false;
        if (n.group == 0) active = head_active;
        else if (n.group == 1) active = neck_active;
        else if (n.group == 2) active = chest_active;
        else if (n.group == 3) active = legs_active;
        else if (n.group == 4) active = arms_active;

        ImVec2 txt_sz = gui->text_size(lbl_font, n.label);
        ImRect click_rect(n.pt - ImVec2(9.f * s, 9.f * s), n.pt + ImVec2(9.f * s, 9.f * s));
        ImRect text_rect(n.label_pos, n.label_pos + txt_sz);

        bool hovered = (click_rect.Contains(gui->mouse_pos()) || (show_labels && text_rect.Contains(gui->mouse_pos()))) && gui->is_window_hovered();
        if (hovered && gui->mouse_clicked(mouse_button_left))
        {
            if (bones)
            {
                if (n.group == 0 && 0 < (int)bones->callback.size()) bones->callback[0] = !bones->callback[0];
                else if (n.group == 1 && 1 < (int)bones->callback.size()) bones->callback[1] = !bones->callback[1];
                else if (n.group == 2 && 2 < (int)bones->callback.size()) bones->callback[2] = !bones->callback[2];
                else if (n.group == 3) {
                    bool cur = is_bone_active(3) || is_bone_active(4);
                    if (3 < (int)bones->callback.size()) bones->callback[3] = !cur;
                    if (4 < (int)bones->callback.size()) bones->callback[4] = !cur;
                }
                else if (n.group == 4) {
                    bool cur = is_bone_active(5) || is_bone_active(6);
                    if (5 < (int)bones->callback.size()) bones->callback[5] = !cur;
                    if (6 < (int)bones->callback.size()) bones->callback[6] = !cur;
                }
                data->value_changed = true;
            }
        }

        // Leader line & Text callouts only when show_labels is TRUE
        if (show_labels)
        {
            ImVec2 leader_target = n.align_right ? (n.label_pos - ImVec2(4.f * s, -txt_sz.y * 0.5f)) : (n.label_pos + ImVec2(txt_sz.x + 4.f * s, txt_sz.y * 0.5f));
            drawlist->AddLine(n.pt, leader_target, active ? ImColor(0, 242, 254, 180) : ImColor(60, 70, 90, 150), 1.0f);

            ImVec4 text_col = active ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.45f, 0.52f, 0.65f, 1.f);
            if (hovered) text_col = ImVec4(0.f, 0.95f, 1.f, 1.f);
            drawlist->AddText(lbl_font, SCALE(11.f), n.label_pos, draw->get_clr(text_col), n.label);
        }

        // Glowing interactive Reticle Ring on the node
        if (active)
        {
            drawlist->AddCircleFilled(n.pt, 6.5f * s, ImColor(0, 242, 254, 45), 20);
            drawlist->AddCircle(n.pt, 5.0f * s, ImColor(0, 242, 254, 200), 20, 1.2f);
            drawlist->AddCircleFilled(n.pt, 3.0f * s, ImColor(0, 242, 254, 255), 16);
            drawlist->AddCircleFilled(n.pt, 1.5f * s, IM_COL32_WHITE, 16);
        }
        else
        {
            if (hovered)
            {
                drawlist->AddCircleFilled(n.pt, 5.0f * s, ImColor(0, 242, 254, 30), 16);
                drawlist->AddCircle(n.pt, 4.0f * s, ImColor(0, 242, 254, 150), 16, 1.0f);
            }
            drawlist->AddCircleFilled(n.pt, 2.5f * s, ImColor(70, 85, 110, 180), 16);
        }
    }

    return data;
}