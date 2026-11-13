#pragma once

#include <Utils/Dependencies/ImGui/imgui.h>
#include <Main/FivemSDK/Fivem.hpp>

namespace M_base
{
    namespace BoneLine
    {
        void DrawBoneLine(const Vector3D& bonePos, const ImVec2& center, ImU32 color, float thickness, Cheat::CFiveFramework& fiveFramework);
    }
}
