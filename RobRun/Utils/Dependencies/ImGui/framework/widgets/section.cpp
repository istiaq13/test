#include "../headers/functions.h"
#include "../headers/widgets.h"

widget_t c_widgets::section(std::string_view icon, int index, int& count)
{
    struct anim_t
    {
        float alpha{ 0 };
        float hover_alpha{ 0 };
        ImVec4 icon{ clr->widgets.text_inactive };
    };

    int total_sections = (int)var->gui.sections_data.second.size();
    float available_height = gui->window_height() - SCALE(var->gui.logo_height);
    float spacing = SCALE(8.f);
    float section_height = (available_height - (spacing * (total_sections - 1)) - SCALE(16.f)) / total_sections;
    
    widget_t data = gui->register_item("section_" + std::string(icon), ImVec2(gui->content_avail().x, section_height));
    anim_t* anim = gui->anim_container<anim_t>(data->id);
    bool active = index == count;

    data->value_changed = data->hovered && gui->mouse_clicked(mouse_button_left) && gui->is_window_hovered();
    if (data->value_changed)
        count = index;

    gui->easing(anim->alpha, active ? 1.f : 0.f, 8.f, static_easing);
    gui->easing(anim->hover_alpha, (data->hovered && !active) ? 1.f : 0.f, 10.f, dynamic_easing);

    ImVec4 target_icon = active ? ImVec4(1.f, 1.f, 1.f, 1.f) : (data->hovered ? ImVec4(0.85f, 0.95f, 1.f, 1.f) : clr->widgets.text_inactive.Value);
    gui->easing(anim->icon, target_icon, 14.f, dynamic_easing);

    ImDrawList* drawlist = gui->window_drawlist();
    float rounding = SCALE(elements->section.rounding);

    // Hover background (when inactive)
    if (anim->hover_alpha > 0.01f)
    {
        draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, ImColor(255, 255, 255, (int)(10.f * anim->hover_alpha)), rounding);
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(255, 255, 255, (int)(18.f * anim->hover_alpha)), rounding);
    }

    // Active state with GLOWING BOTH-SIDE EFFECT
    if (anim->alpha > 0.005f)
    {
        // 1. Dark Obsidian active card fill
        draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, draw->get_clr(clr->window.sub_background, anim->alpha), rounding);

        // 2. Ambient Ice Cyan inner glow wash
        draw->rect_filled(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(18.f * anim->alpha)), rounding);

        // 3. Delicate glowing cyan border
        draw->rect(drawlist, data->rect.Min, data->rect.Max, ImColor(0, 242, 254, (int)(55.f * anim->alpha)), rounding);

        // 4. BOTH-SIDE GLOWING CAPSULE PILLS (Left & Right)
        const float pill_width = SCALE(3.5f);
        const float pill_inset_y = SCALE(7.f);
        const float pill_rounding = SCALE(2.f);

        // --- LEFT GLOWING PILL ---
        ImVec2 left_min = ImVec2(data->rect.Min.x + SCALE(1.f), data->rect.Min.y + pill_inset_y);
        ImVec2 left_max = ImVec2(data->rect.Min.x + SCALE(1.f) + pill_width, data->rect.Max.y - pill_inset_y);

        // Layer 1: Wide soft neon bloom
        draw->rect_filled(drawlist, left_min - ImVec2(SCALE(3.f), SCALE(2.f)), left_max + ImVec2(SCALE(3.f), SCALE(2.f)), ImColor(0, 242, 254, (int)(25.f * anim->alpha)), pill_rounding + SCALE(2.f));
        // Layer 2: Mid neon aura
        draw->rect_filled(drawlist, left_min - ImVec2(SCALE(1.5f), SCALE(1.f)), left_max + ImVec2(SCALE(1.5f), SCALE(1.f)), ImColor(0, 242, 254, (int)(75.f * anim->alpha)), pill_rounding + SCALE(1.f));
        // Layer 3: Solid vibrant capsule core
        draw->rect_filled(drawlist, left_min, left_max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), pill_rounding);

        // --- RIGHT GLOWING PILL ---
        ImVec2 right_min = ImVec2(data->rect.Max.x - SCALE(1.f) - pill_width, data->rect.Min.y + pill_inset_y);
        ImVec2 right_max = ImVec2(data->rect.Max.x - SCALE(1.f), data->rect.Max.y - pill_inset_y);

        // Layer 1: Wide soft neon bloom
        draw->rect_filled(drawlist, right_min - ImVec2(SCALE(3.f), SCALE(2.f)), right_max + ImVec2(SCALE(3.f), SCALE(2.f)), ImColor(0, 242, 254, (int)(25.f * anim->alpha)), pill_rounding + SCALE(2.f));
        // Layer 2: Mid neon aura
        draw->rect_filled(drawlist, right_min - ImVec2(SCALE(1.5f), SCALE(1.f)), right_max + ImVec2(SCALE(1.5f), SCALE(1.f)), ImColor(0, 242, 254, (int)(75.f * anim->alpha)), pill_rounding + SCALE(1.f));
        // Layer 3: Solid vibrant capsule core
        draw->rect_filled(drawlist, right_min, right_max, ImColor(0, 242, 254, (int)(255.f * anim->alpha)), pill_rounding);
    }

    // Icon on the left
    ImVec2 icon_min = ImVec2(data->rect.Min.x + SCALE(12.f), data->rect.Min.y);
    ImVec2 icon_max = ImVec2(data->rect.Min.x + SCALE(36.f), data->rect.Max.y);
    draw->text_clipped(drawlist, font->get(icons_data, 15), icon_min, icon_max, draw->get_clr(anim->icon), icon.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    // Title on the right (vertically centered)
    if (index >= 0 && index < (int)var->gui.section_name.size())
    {
        const std::string& name = var->gui.section_name.at(index);

        ImVec2 title_min = ImVec2(data->rect.Min.x + SCALE(42.f), data->rect.Min.y);
        ImVec2 title_max = ImVec2(data->rect.Max.x - SCALE(10.f), data->rect.Max.y);
        draw->text_clipped(drawlist, font->get(inter_semibold_data, 13), title_min, title_max, draw->get_clr(anim->icon), name.c_str(), NULL, NULL, ImVec2(0.f, 0.5f));
    }

    if (data->hovered && gui->mouse_down(mouse_button_left) && gui->is_window_hovered())
        GImGui->IO.MouseDown[0] = false;

    return data;
}