#pragma once
#include <string>
#include "../../imgui.h"

class c_elements
{
public:
    struct
    {
        ImVec2 padding{ 10, 0 };
        ImVec2 spacing{ 0, 0 };
        float rounding{ 6 };
    } section;

    struct
    {
        ImVec2 padding{ 10, 10 };
        ImVec2 spacing{ 10, 10 };
    } content;

    struct
    {
        ImVec2 padding{ 12, 12 };
        ImVec2 spacing{ 12, 12 };
        float title_height{ 35 };
        float rounding{ 8 };
    } child;

    struct
    {
        ImVec2 size{ 34, 18 };
        float radius{ 5.5 };
        float padding{ 2.5 };
    } checkbox;

    struct
    {
        ImVec2 size{ 135, 15 };
        float radius{ 5.5 };
        float padding{ 2 };
        float value_padding{ 10 };
    } slider;

    struct
    {
        float height{ 30 };
        float rounding{ 4 };
    } button;

    struct
    {
        float height{ 52 };
        float size{ 30 };
        float rounding{ 4 };
        float padding{ 10 };
        ImVec2 scrollbar_border{ 8, 8 };
        float scrollbar_content{ 8 };
    } dropdown;

    struct
    {
        float size{ 30 };
        float rounding{ 4 };
        float padding{ 10 };
        float radius{ 2 };
    } selectable;

    struct
    {
        float height{ 28 };
        float size{ 10 };
        float sv_grab_size{ 7 };
        ImVec2 sv_rect{ 122, 122 };
        float rounding{ 2 };
        float window_rounding{ 4 };
        float bar_padding{ 1 };
        float bar_grab{ 10 };
        ImVec2 padding{ 10, 10 };
        ImVec2 spacing{ 10, 10 };
        float hex_height{ 30 };
        float title_bar{ 30 };
    } color_edit;

    struct
    {
        ImVec2 padding{ 10, 10 };
        ImVec2 spacing{ 10, 10 };
        float width{ 10 };
        float window_rounding{ 4 };
        float button_height{ 30 };
        float button_rounding{ 3 };
        float window_width{ 200 };
        float title_bar{ 30 };
    } keybind;

    struct
    {
        float height{ 30 };
        float rounding{ 2 };
        float padding{ 10 };
        float line_padding{ 7 };
    } text_field;

    struct
    {
        float title_height{ 50 };
        float height{ 64 };
        float rounding{ 4 };
        float button{ 34 };
        float padding{ 15 };
        ImVec2 text_field_size{ 180, 30 };
        float create_button{ 30 };
    } config;

    struct
    {
        float height{ 60 };
        float rounding{ 6 };
        ImVec2 padding{ 20, 0 };
        ImVec2 spacing{ 40, 40 };
        ImVec2 display_padding{ 20, 20 };
    } watermark;

    struct
    {
        float height{ 52 };
        float rounding{ 6 };
        float padding{ 14 };
    } notify;
};

inline std::unique_ptr<c_elements> elements = std::make_unique<c_elements>();
