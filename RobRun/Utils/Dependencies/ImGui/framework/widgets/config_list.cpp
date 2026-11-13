#include "../headers/functions.h"
#include "../headers/widgets.h"

widget_t cfg_button(std::string_view name, std::string_view icon)
{
    widget_t data = gui->register_item(name, SCALE(elements->config.button, elements->config.button));
    ImVec4* anim = gui->anim_container<ImVec4>(data->id);

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left);

    gui->easing(*anim, data->hovered ? clr->widgets.text.Value : clr->widgets.text_inactive.Value, 16.f, dynamic_easing);

    draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 14), data->rect.Min, data->rect.Max, draw->get_clr(*anim), icon.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    return data;
}

widget_t c_widgets::config(std::string_view name, std::string_view date, int index, int& count, int ind)
{
    struct anim_t
    {
        float alpha{ 0 };
    };

    widget_t data = gui->register_item(name, ImVec2(gui->content_avail().x, SCALE(elements->config.height)));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    ImRect rect{ ImVec2(data->rect.Max.x - SCALE(elements->config.button + elements->config.padding), data->rect.GetCenter().y - SCALE(elements->config.button / 2)), ImVec2(data->rect.Max.x - SCALE(elements->config.padding), data->rect.GetCenter().y + SCALE(elements->config.button / 2)) };
    bool active = index == count;

    data->value_changed = rect.Contains(gui->mouse_pos()) && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();
    if (data->value_changed)
        count = index;

    gui->easing(anim->alpha, active ? 1.f : 0.f, 8.f, static_easing);

    draw->rect_filled(gui->window_drawlist(), data->rect.Min, data->rect.Max, draw->get_clr(clr->window.sub_background), SCALE(elements->config.rounding));

    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 15), data->rect.Min + SCALE(elements->config.padding, 0), data->rect.Max, draw->get_clr(clr->widgets.text), name.data(), NULL, NULL, ImVec2(0.f, 0.3f));
    draw->text_clipped(gui->window_drawlist(), font->get(inter_medium_data, 15), data->rect.Min + SCALE(elements->config.padding, 0), data->rect.Max, draw->get_clr(clr->widgets.text_inactive), date.data(), NULL, NULL, ImVec2(0.f, 0.7f));

    draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->widgets.background), SCALE(elements->config.rounding));
    draw->rect_filled(gui->window_drawlist(), rect.Min, rect.Max, draw->get_clr(clr->accent, anim->alpha), SCALE(elements->config.rounding));
    draw->text_clipped(gui->window_drawlist(), font->get(icons_data, 10), rect.Min, rect.Max, draw->get_clr(clr->widgets.background, anim->alpha), "K", NULL, NULL, ImVec2(0.5f, 0.5f));

    ImVec2 stored_pos = gui->get_screen_pos();
    gui->set_screen_pos(rect.GetTL() - SCALE(elements->config.button + elements->config.padding, 0), pos_all);
    if (cfg_button("delete_button" + std::string(name), "L").value_changed)
    {
        var->gui.configs.erase(var->gui.configs.begin() + ind);
    }

    gui->set_screen_pos(stored_pos, pos_all);

    return data;
}  