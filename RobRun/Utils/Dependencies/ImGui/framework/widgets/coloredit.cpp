#define _CRT_SECURE_NO_WARNINGS
#include "../headers/functions.h"
#include "../headers/widgets.h"

void render_color_rect_with_alpha_checkboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, ImDrawFlags flags)
{
    if ((flags & ImDrawFlags_RoundCornersMask_) == 0)
        flags = ImDrawFlags_RoundCornersDefault_;
    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
    {
        ImU32 col_bg1 = draw->get_clr(ImColor(27, 27, 32));
        ImU32 col_bg2 = draw->get_clr(ImColor(46, 46, 56));
        draw_list->AddRectFilled(p_min, p_max, col_bg1, rounding, flags);

        int yi = 0;
        for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
        {
            float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
            if (y2 <= y1)
                continue;
            for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
            {
                float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
                if (x2 <= x1)
                    continue;
                ImDrawFlags cell_flags = ImDrawFlags_RoundCornersNone;
                if (y1 <= p_min.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersTopLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersTopRight; }
                if (y2 >= p_max.y) { if (x1 <= p_min.x) cell_flags |= ImDrawFlags_RoundCornersBottomLeft; if (x2 >= p_max.x) cell_flags |= ImDrawFlags_RoundCornersBottomRight; }

                // Combine flags
                cell_flags = (flags == ImDrawFlags_RoundCornersNone || cell_flags == ImDrawFlags_RoundCornersNone) ? ImDrawFlags_RoundCornersNone : (cell_flags & flags);
                draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding, cell_flags);
            }
        }
    }
    else
    {
        draw_list->AddRectFilled(p_min, p_max, col, rounding, flags);
    }
}

bool input_hex(const char* label, float col[4], bool alpha)
{
    char buf[10];

    if (alpha)
        snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X",
            (int)(col[0] * 255.0f),
            (int)(col[1] * 255.0f),
            (int)(col[2] * 255.0f),
            (int)(col[3] * 255.0f));
    else
        snprintf(buf, sizeof(buf), "#%02X%02X%02X",
            (int)(col[0] * 255.0f),
            (int)(col[1] * 255.0f),
            (int)(col[2] * 255.0f));

    bool value_changed = widgets->hex_field(label, buf, sizeof(buf)).value_changed;

    if (value_changed)
    {
        if (buf[0] == '#')
        {
            memmove(buf, buf + 1, strlen(buf));
        }

        for (int i = 0; i < (int)strlen(buf); ++i)
        {
            if (!isxdigit((unsigned char)buf[i]))
            {
                buf[i] = 'F';
            }
        }

        int expected_len = alpha ? 8 : 6;
        size_t current_len = strlen(buf);
        if (current_len < (size_t)expected_len)
        {
            memset(buf + current_len, 'F', expected_len - current_len);
            buf[expected_len] = '\0';
        }

        int r, g, b, a = 255;
        if (alpha)
            sscanf(buf, "%02X%02X%02X%02X", &r, &g, &b, &a);
        else
            sscanf(buf, "%02X%02X%02X", &r, &g, &b);

        col[0] = r / 255.0f;
        col[1] = g / 255.0f;
        col[2] = b / 255.0f;
        col[3] = a / 255.0f;
    }

    return value_changed;
}

void c_widgets::color_edit_ex(std::string_view name, std::string_view widget_name, float* color, bool alpha, const ImRect& rect, const ImRect& clickable_rect, bool* value_changed)
{
    struct anim_t
    {
        bool opened{ false };
        bool hovered{ false };
        float alpha{ 0.f };
        float h{ -1 }, s{ -1 }, v{ -1 };
        float grab[4]{ 0.f };
        bool update_hsv{ false };
        float stored_size{ 0 };
    };

    anim_t* anim = gui->anim_container<anim_t>(gui->get_window()->GetID(name.data()));
    ImColor col_hues[7] = { ImColor(255, 0, 0), ImColor(255, 255, 0), ImColor(0, 255, 0), ImColor(0, 255, 255), ImColor(0, 0, 255), ImColor(255, 0, 255), ImColor(255, 0, 0) };

    if ((clickable_rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered()) || (anim->opened && (gui->mouse_clicked(mouse_button_left) || gui->mouse_clicked(mouse_button_right)) && !anim->hovered))
        anim->opened = !anim->opened;

    gui->easing(anim->alpha, anim->opened ? 1.f : 0.f, 8.f, static_easing);

    draw->circle_filled(gui->window_drawlist(), rect.GetCenter(), SCALE(8.5f), draw->get_clr({ color[0], color[1], color[2], 0.1f}), 30);
    render_color_rect_with_alpha_checkboard(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr({ 0.f, 0.f, 0.f, 0.f }), rect.GetWidth() / 2, ImVec2(0, 0), rect.GetWidth() / 2, 0);
    draw->circle_filled(gui->window_drawlist(), rect.GetCenter(), rect.GetWidth() / 2, draw->get_clr({ color[0], color[1], color[2], color[3] }), 30);

    if ((anim->h < 0.f || anim->s < 0.f || anim->v < 0.f) || anim->update_hsv)
    {
        gui->rgb_to_hsv(color[0], color[1], color[2], anim->h, anim->s, anim->v);
        anim->update_hsv = false;
    }

    if (anim->alpha >= 0.01f)
    {
        gui->push_var(style_var_alpha, anim->alpha);
        gui->push_var(style_var_item_spacing, SCALE(elements->color_edit.spacing));
        gui->push_var(style_var_window_padding, SCALE(elements->color_edit.padding));
        gui->push_var(style_var_window_rounding, SCALE(elements->color_edit.window_rounding));
        gui->push_var(style_var_popup_border_size, SCALE(0));
        gui->push_color(style_col_popup_bg, draw->get_clr(clr->popup.background));
        gui->set_next_window_pos(gui->adjust_window_pos(rect.GetBL() + SCALE(-(elements->color_edit.padding.x * 2 + elements->color_edit.sv_rect.x + elements->color_edit.bar_grab + elements->color_edit.spacing.x) / 2, elements->color_edit.padding.y), ImVec2(SCALE(elements->color_edit.padding.x * 2 + elements->color_edit.sv_rect.x + elements->color_edit.bar_grab + elements->color_edit.spacing.x), anim->stored_size)));
        gui->set_next_window_size(SCALE(elements->color_edit.padding.x * 2 + elements->color_edit.sv_rect.x + elements->color_edit.bar_grab + elements->color_edit.spacing.x, 0));
        gui->begin("coloredit_window" + std::string(name), nullptr, window_flags_tooltip | window_flags_always_use_window_padding | window_flags_no_saved_settings | window_flags_no_focus_on_appearing | window_flags_always_auto_resize | window_flags_no_decoration | window_flags_no_scrollbar | window_flags_no_scroll_with_mouse);
        {
            gui->set_window_focus();
            anim->hovered = gui->is_window_hovered();

            draw->rect_filled(gui->window_drawlist(), gui->window_pos(), gui->window_pos() + ImVec2(gui->window_width(), SCALE(elements->color_edit.title_bar)), draw->get_clr(clr->popup.sub_background), SCALE(elements->color_edit.window_rounding), draw_flags_round_corners_top);
            draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), gui->window_pos() + SCALE(elements->color_edit.padding.x, 0), gui->window_pos() + ImVec2(gui->window_width(), SCALE(elements->color_edit.title_bar)), draw->get_clr(clr->widgets.text), (std::stringstream{} << "\"" << widget_name << "\"").str().data(), NULL, NULL, ImVec2(0.f, 0.5f));

            gui->dummy(SCALE(0, elements->keybind.title_bar - elements->keybind.padding.y));

            // sv_rect
            gui->invisible_button("sv_rect", SCALE(elements->color_edit.sv_rect));

            if (gui->is_item_active())
            {
                anim->s = ImSaturate((gui->mouse_pos().x - (GImGui->LastItemData.Rect.Min.x + SCALE(elements->color_edit.sv_grab_size) / 2)) / (GImGui->LastItemData.Rect.GetWidth() - SCALE(elements->color_edit.sv_grab_size)));
                anim->v = 1.f - ImSaturate((gui->mouse_pos().y - (GImGui->LastItemData.Rect.Min.y + SCALE(elements->color_edit.sv_grab_size) / 2)) / (GImGui->LastItemData.Rect.GetHeight() - SCALE(elements->color_edit.sv_grab_size)));
                *value_changed = true;
            }

            gui->easing(anim->grab[0], SCALE(elements->color_edit.sv_grab_size) / 2 + anim->s * (GImGui->LastItemData.Rect.GetWidth() - SCALE(elements->color_edit.sv_grab_size)), 20.f, dynamic_easing);
            gui->easing(anim->grab[1], SCALE(elements->color_edit.sv_grab_size) / 2 + (1.f - anim->v) * (GImGui->LastItemData.Rect.GetHeight() - SCALE(elements->color_edit.sv_grab_size)), 20.f, dynamic_easing);

            float R, G, B;
            ColorConvertHSVtoRGB(anim->h, 1.f, 1.f, R, G, B);

            draw->rect_filled_multi_color(gui->window_drawlist(), GImGui->LastItemData.Rect.Min, GImGui->LastItemData.Rect.Max, draw->get_clr({ 1.f, 1.f, 1.f, 1.f }), draw->get_clr({ R, G, B, 1.f }), draw->get_clr({ R, G, B, 1.f }), draw->get_clr({ 1.f, 1.f, 1.f, 1.f }), SCALE(elements->color_edit.rounding));
            draw->rect_filled_multi_color(gui->window_drawlist(), GImGui->LastItemData.Rect.Min, GImGui->LastItemData.Rect.Max, draw->get_clr({ 0.f, 0.f, 0.f, 0.f }), draw->get_clr({ 0.f, 0.f, 0.f, 0.f }), draw->get_clr({ 0.f, 0.f, 0.f, 1.f }), draw->get_clr({ 0.f, 0.f, 0.f, 1.f }), SCALE(elements->color_edit.rounding) - 1);

            draw->shadow_circle(gui->window_drawlist(), GImGui->LastItemData.Rect.Min + ImVec2(anim->grab[0], anim->grab[1]), SCALE(elements->color_edit.sv_grab_size / 2), draw->get_clr({ 0.f, 0.f, 0.f, 1.f }), SCALE(15), ImVec2(0, 0), draw_flags_shadow_cut_out_shape_background);
            draw->circle(gui->window_drawlist(), GImGui->LastItemData.Rect.Min + ImVec2(anim->grab[0], anim->grab[1]), SCALE(elements->color_edit.sv_grab_size / 2), draw->get_clr(clr->widgets.text), 30, SCALE(1));

            gui->sameline();

            // hue_bar
            gui->invisible_button("hue_bar", SCALE(elements->color_edit.bar_grab, elements->color_edit.sv_rect.y));

            if (gui->is_item_active())
            {
                anim->h = ImSaturate((gui->mouse_pos().y - (GImGui->LastItemData.Rect.Min.y + SCALE(elements->color_edit.sv_grab_size) / 2)) / (GImGui->LastItemData.Rect.GetHeight() - SCALE(elements->color_edit.sv_grab_size)));
                *value_changed = true;
            }

            gui->easing(anim->grab[2], SCALE(elements->color_edit.bar_grab) / 2 + anim->h * (GImGui->LastItemData.Rect.GetHeight() - SCALE(elements->color_edit.bar_grab)), 20.f, dynamic_easing);

            for (int i = 0; i < IM_ARRAYSIZE(col_hues) - 1; ++i)
                draw->rect_filled_multi_color(gui->window_drawlist(), GImGui->LastItemData.Rect.Min + ImVec2(SCALE(elements->color_edit.bar_padding), roundf(i * (GImGui->LastItemData.Rect.GetHeight() / 6))), ImVec2(GImGui->LastItemData.Rect.Max.x - SCALE(elements->color_edit.bar_padding), GImGui->LastItemData.Rect.Min.y + roundf((i + 1) * (GImGui->LastItemData.Rect.GetHeight() / 6))), draw->get_clr(col_hues[i]), draw->get_clr(col_hues[i]), draw->get_clr(col_hues[i + 1]), draw->get_clr(col_hues[i + 1]), 100, (i == 0) ? ImDrawFlags_RoundCornersTop : (i == 5) ? ImDrawFlags_RoundCornersBottom : ImDrawFlags_RoundCornersNone);

            draw->rect_filled(gui->window_drawlist(), GImGui->LastItemData.Rect.Min + ImVec2(0, anim->grab[2] - SCALE(elements->color_edit.bar_grab / 2)), GImGui->LastItemData.Rect.GetTR() + ImVec2(0, anim->grab[2] + SCALE(elements->color_edit.bar_grab / 2)), draw->get_clr(clr->widgets.text), 100);

            // alpha_bar
            if (alpha)
            {
                gui->invisible_button("alpha_bar", ImVec2(gui->content_avail().x, SCALE(elements->color_edit.bar_grab)));

                if (gui->is_item_active())
                    color[3] = ImSaturate((gui->mouse_pos().x - (GImGui->LastItemData.Rect.Min.x + SCALE(elements->color_edit.bar_grab) / 2)) / (GImGui->LastItemData.Rect.GetWidth() - SCALE(elements->color_edit.bar_grab)));

                gui->easing(anim->grab[3], SCALE(elements->color_edit.bar_grab) / 2 + color[3] * (GImGui->LastItemData.Rect.GetWidth() - SCALE(elements->color_edit.bar_grab)), 20.f, dynamic_easing);

                draw->rect_filled_multi_color(gui->window_drawlist(), GImGui->LastItemData.Rect.Min + SCALE(0, elements->color_edit.bar_padding), GImGui->LastItemData.Rect.Max - SCALE(0, elements->color_edit.bar_padding), draw->get_clr(clr->widgets.text), draw->get_clr({ R, G, B, 1.f }), draw->get_clr({ R, G, B, 1.f }), draw->get_clr(clr->widgets.text), 100);

                draw->rect_filled(gui->window_drawlist(), GImGui->LastItemData.Rect.Min + ImVec2(anim->grab[3] - SCALE(elements->color_edit.bar_grab / 2), 0), GImGui->LastItemData.Rect.GetBL() + ImVec2(anim->grab[3] + SCALE(elements->color_edit.bar_grab / 2), 0), draw->get_clr(clr->widgets.text), 100);
            }

            if (input_hex("#HEX", color, alpha))
                anim->update_hsv = true;

            anim->stored_size = gui->window_height();
        }
        gui->end();
        gui->pop_color();
        gui->pop_var(5);
    }

    if (*value_changed)
    {
        gui->hsv_to_rgb(anim->h, anim->s, anim->v, color[0], color[1], color[2]);
        *value_changed = false;
    }
}