#include "../headers/functions.h"
#include "../headers/widgets.h"
#include <algorithm>

void c_notify::add_notify(std::string_view text, notify_type type)
{
    std::lock_guard<std::mutex> lock(notify_mutex);
    notifications.push_back({ notify_count++, std::string(text), type });
}

void c_notify::setup_notify()
{
    std::lock_guard<std::mutex> lock(notify_mutex);
    // Clean up expired notifications
    notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(),
            [](const notify_state& n) { return !n.active_notify && n.notify_alpha <= 0.01f; }),
        notifications.end()
    );

    float accumulated_height = 0.f;

    for (auto& notification : notifications)
    {
        if (notification.active_notify)
            notification.notify_timer += ImGui::GetIO().DeltaTime;

        if (notification.notify_timer >= notify_time)
            notification.active_notify = false;

        gui->easing(notification.notify_alpha, notification.active_notify ? 1.f : 0.f, 6.f, static_easing);

        if (notification.notify_alpha > 0.f)
        {
            float target_position = accumulated_height + notify_padding.y;
            gui->easing(notification.notify_pos, target_position, 8.f, dynamic_easing);

            ImVec2 window_size = render_notify(notification.notify_id, notification.notify_alpha, notification.notify_timer, notification.notify_pos, notification.text, notification.type);

            accumulated_height += window_size.y + notify_spacing;
        }
    }
}

ImVec2 c_notify::render_notify(int notify_id, float notify_alpha, float notify_timer, float notify_pos, const std::string& text, notify_type type)
{
    ImVec2 window_size;

    const char* title = (type == error) ? "ERROR" : "SUCCESS";
    ImColor status_clr = (type == error) ? ImColor(255, 95, 95) : ImColor(75, 225, 145);

    ImFont* title_font = font->get(inter_semibold_data, 13);
    ImFont* text_font = font->get(inter_semibold_data, 13);

    ImVec2 title_size = gui->text_size(title_font, title, title + strlen(title));
    ImVec2 text_size = gui->text_size(text_font, text.c_str(), text.c_str() + text.length());

    float content_width = ImMax(title_size.x, text_size.x);
    float height = SCALE(elements->notify.height);
    float icon_box_w = height;
    float total_width = icon_box_w + content_width + SCALE(36.f);
    total_width = ImMax(total_width, SCALE(220.f));
    float rounding = SCALE(elements->notify.rounding);

    gui->push_var(style_var_alpha, notify_alpha);
    gui->push_var(style_var_popup_border_size, 0.f);
    SetNextWindowPos(ImVec2(notify_padding.x, notify_pos));
    gui->set_next_window_size(ImVec2(total_width, height));
    gui->begin("notify_" + std::to_string(notify_id), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    {
        ImVec2 pos = gui->window_pos();
        ImVec2 size = gui->window_size();
        ImDrawList* drawlist = gui->window_drawlist();

        // Subtle drop shadow behind the notification toast
        draw->shadow_rect(drawlist, pos, pos + size, draw->get_clr(ImColor(0, 0, 0, 140).Value), SCALE(12.f), ImVec2(0, SCALE(4.f)), draw_flags_round_corners_all, rounding);

        // Background: right content area
        draw->rect_filled(drawlist, pos + ImVec2(icon_box_w, 0), pos + size, draw->get_clr(clr->window.background), rounding, draw_flags_round_corners_right);
        // Background: left icon box area
        draw->rect_filled(drawlist, pos, pos + ImVec2(icon_box_w, size.y), draw->get_clr(clr->window.sub_background), rounding, draw_flags_round_corners_left);
        // Vertical divider line between icon box and content
        draw->line(drawlist, pos + ImVec2(icon_box_w, 0), pos + ImVec2(icon_box_w, size.y), draw->get_clr(clr->window.stroke));

        // Left accent bar (status indicator)
        draw->rect_filled(drawlist, pos, pos + ImVec2(SCALE(3.f), size.y), draw->get_clr(status_clr.Value), rounding, draw_flags_round_corners_left);

        // Outer border stroke for crisp contrast against any game background
        draw->rect(drawlist, pos, pos + size, draw->get_clr(clr->window.stroke), rounding, 0, SCALE(1.f));

        // Bottom progress bar (shrinks smoothly as notification times out)
        float progress = ImClamp(1.f - (notify_timer / notify_time), 0.f, 1.f);
        if (progress > 0.01f)
        {
            ImVec4 bar_clr = status_clr.Value;
            bar_clr.w = 0.8f;
            draw->rect_filled(drawlist, pos + ImVec2(SCALE(3.f), size.y - SCALE(2.f)), pos + ImVec2(size.x * progress, size.y), draw->get_clr(bar_clr), rounding, draw_flags_round_corners_bottom);
        }

        // Icon inside left box
        float cx = pos.x + icon_box_w / 2.f;
        float cy = pos.y + size.y / 2.f;
        float badge_r = SCALE(11.f);

        ImVec4 badge_bg = status_clr.Value;
        badge_bg.w = 0.15f;
        draw->circle_filled(drawlist, ImVec2(cx, cy), badge_r, draw->get_clr(badge_bg), 24);
        draw->circle(drawlist, ImVec2(cx, cy), badge_r, draw->get_clr(status_clr.Value), 24, SCALE(1.f));

        if (type == error)
        {
            // Cross icon
            float d = SCALE(3.8f);
            draw->line(drawlist, ImVec2(cx - d, cy - d), ImVec2(cx + d, cy + d), draw->get_clr(status_clr.Value), SCALE(1.5f));
            draw->line(drawlist, ImVec2(cx - d, cy + d), ImVec2(cx + d, cy - d), draw->get_clr(status_clr.Value), SCALE(1.5f));
        }
        else
        {
            // Checkmark icon
            ImVec2 p1 = ImVec2(cx - SCALE(4.2f), cy);
            ImVec2 p2 = ImVec2(cx - SCALE(1.2f), cy + SCALE(3.2f));
            ImVec2 p3 = ImVec2(cx + SCALE(4.5f), cy - SCALE(3.0f));
            draw->line(drawlist, p1, p2, draw->get_clr(status_clr.Value), SCALE(1.5f));
            draw->line(drawlist, p2, p3, draw->get_clr(status_clr.Value), SCALE(1.5f));
        }

        // Title text (e.g. SUCCESS / ERROR)
        draw->text_clipped(drawlist, title_font, pos + ImVec2(icon_box_w + SCALE(14.f), SCALE(8.f)), pos + ImVec2(size.x - SCALE(12.f), SCALE(24.f)), draw->get_clr(status_clr.Value), title, title + strlen(title), NULL, ImVec2(0.f, 0.5f));

        // Message text
        draw->text_clipped(drawlist, text_font, pos + ImVec2(icon_box_w + SCALE(14.f), SCALE(24.f)), pos + ImVec2(size.x - SCALE(12.f), size.y - SCALE(4.f)), draw->get_clr(clr->widgets.text), text.c_str(), text.c_str() + text.length(), NULL, ImVec2(0.f, 0.5f));

        window_size = gui->window_size();
    }
    gui->end();
    gui->pop_var(2);

    return window_size;
}