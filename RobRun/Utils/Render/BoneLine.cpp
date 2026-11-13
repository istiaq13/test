#include "BoneLine.hpp"
#include <Main/FivemSDK/Fivem.hpp>

namespace M_base
{
    namespace BoneLine
    {
    void DrawBoneLine(const Vector3D& bonePos, const ImVec2& center, ImU32 color, float thickness, Cheat::CFiveFramework& fiveFramework) {
        try {
            if (bonePos.x == 0.0f && bonePos.y == 0.0f && bonePos.z == 0.0f) {
                return;
            }
            
            ImVec2 boneScreenPos = fiveFramework.WorldToScreen(bonePos);
            
            if (boneScreenPos.x == 0.0f && boneScreenPos.y == 0.0f) {
                return;
            }
            
            if (fiveFramework.IsOnScreen(boneScreenPos)) {
                ImGui::GetBackgroundDrawList()->AddLine(center, boneScreenPos, color, thickness);
            }
        }
        catch (...) {
            return;
        }
    }
    }
}
