#include "../headers/functions.h"
#include "../headers/widgets.h"

template <typename T>
widget_t range_ex(std::string_view name, T* callback, T* callback_two, T min, T max, T range, std::string_view format)
{
    struct anim_t
    {
        float offset[2]{ 0, 0 };
        bool grab_active[2]{ false, false };
    };

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, SCALE(elements->slider.size.y)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    const ImRect rect{ data->rect.GetTR() - SCALE(elements->slider.size.x, 0), data->rect.Max };

    bool hovered, held;
    bool pressed = gui->button_behavior(rect, data->id, &hovered, &held);

    const float padding = SCALE(elements->slider.padding);
    const float grab_width = SCALE(elements->slider.radius * 2);
    const float width_with_grab = rect.GetWidth() - grab_width - padding * 2;

    const float grab_offset = padding + grab_width / 2 + static_cast<float>((*callback - min)) / static_cast<float>(max - min) * width_with_grab;
    float grab_offset2 = padding + grab_width / 2 + static_cast<float>((*callback_two - min)) / static_cast<float>(max - min) * width_with_grab;

    char value_buf[64]; const char* value_buf_end = gui->get_fmt(value_buf, callback, format);
    char value_buf2[64]; const char* value_buf_end2 = gui->get_fmt(value_buf2, callback_two, format);

    const float t = ImSaturate((gui->mouse_pos().x - (rect.Min.x + padding + grab_width / 2)) / width_with_grab);

    data->value_changed = held;

    if (data->value_changed)
    {
        float dist_to_grab1 = std::abs(gui->mouse_pos().x - (rect.Min.x + anim->offset[0]));
        float dist_to_grab2 = std::abs(gui->mouse_pos().x - (rect.Min.x + anim->offset[1]));

        if (dist_to_grab1 < dist_to_grab2 && !anim->grab_active[1])
            anim->grab_active[0] = true;

        if (dist_to_grab2 < dist_to_grab1 && !anim->grab_active[0])
            anim->grab_active[1] = true;
    }
    else
    {
        anim->grab_active[0] = false;
        anim->grab_active[1] = false;
    }

    if (anim->grab_active[0])
    {
        *callback = static_cast<T>(min + t * (max - min));
        *callback = ImClamp(*callback, min, *callback_two - range);
    }

    if (anim->grab_active[1])
    {
        *callback_two = static_cast<T>(min + t * (max - min));
        *callback_two = ImClamp(*callback_two, *callback + range, max);
    }

    gui->easing(anim->offset[0], grab_offset, 20.f, dynamic_easing);
    gui->easing(anim->offset[1], grab_offset2, 20.f, dynamic_easing);

    draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->widgets.background), rect.GetHeight() / 2);
    draw->rect_filled_multi_color(gui->window_drawlist(), rect.Min + ImVec2(anim->offset[0] - SCALE(elements->slider.radius + elements->slider.padding), 0), ImVec2(rect.Min.x + anim->offset[1] + SCALE(elements->slider.radius + elements->slider.padding), rect.Max.y), draw->get_clr(clr->accent), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent, 0.5f), draw->get_clr(clr->accent), rect.GetHeight() / 2);

    draw->circle_filled(gui->window_drawlist(), ImVec2(rect.Min.x + anim->offset[0], rect.GetCenter().y), SCALE(elements->slider.radius), draw->get_clr(clr->widgets.text), 30);
    draw->circle_filled(gui->window_drawlist(), ImVec2(rect.Min.x + anim->offset[1], rect.GetCenter().y), SCALE(elements->slider.radius), draw->get_clr(clr->widgets.text), 30);

    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, rect.GetBL(), draw->get_clr(clr->widgets.text_inactive), name.data(), gui->text_end(name.data()), NULL, ImVec2(0.f, 0.5f));
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 13), data->rect.Min, rect.GetBL() - SCALE(elements->slider.value_padding, 0), draw->get_clr(clr->widgets.text), (std::stringstream{} << value_buf << value_buf_end << ", " << value_buf2 << value_buf_end2).str().data(), NULL, NULL, ImVec2(1.f, 0.5f));

    return data;
}

widget_t c_widgets::range_int(std::string_view key)
{
    range_int_t* data = cfg->fill<range_int_t>(key.data());

    return range_ex(data->name, &data->callback, &data->callback_two, data->min, data->max, data->range, data->format);
}

widget_t c_widgets::range_float(std::string_view key)
{
    range_float_t* data = cfg->fill<range_float_t>(key.data());

    return range_ex(data->name, &data->callback, &data->callback_two, data->min, data->max, data->range, data->format);
}