#pragma once
#include "../../imgui.h"
#include <memory>

class c_colors
{
public:

    ImColor accent{ 46, 204, 142 }; // Sea Green (#2ECC8E) — ROB RUN theme

    struct
    {
        ImColor background{ 10, 10, 11 };      // Deep Obsidian Void (#0A0A0B)
        ImColor sub_background{ 25, 28, 27 };  // Dark Obsidian Card (#191C1B)
        ImColor stroke{ 35, 45, 50 };          // Card Border Inactive (#232D32)
        ImColor glowing_stroke{ 46, 204, 142 };// Glowing Sea Green (#2ECC8E)
    } window;

    struct
    {
        ImColor text{ 255, 255, 255 };          // Crisp White (#FFFFFF)
        ImColor text_inactive{ 148, 151, 151 }; // Exact Label Slate (#949797)
        ImColor background{ 18, 21, 22 };       // Dark Inset Input/Dropdown (#121516)
        ImColor sub_background{ 28, 34, 38 };   // Widget Sub / Hover (#1C2226)
    } widgets;

    struct
    {
        ImColor background{ 16, 18, 20 };       // Popup Void (#101214)
        ImColor sub_background{ 24, 28, 30 };   // Popup Sub (#181C1E)
    } popup;
};

inline std::unique_ptr<c_colors> clr = std::make_unique<c_colors>();
