#pragma once
#include <cstdint>
#include <string>
#include <Utils/Utilities/Memory.hpp>
#include <Utils/Math/Vectors/Vector3D.hpp>
#include "Offsets.hpp"

// =====================================================================
//           ROB RUN - Unity IL2CPP Class Wrappers
// =====================================================================

namespace Cheat
{
    // ------------------------------------------------------------------
    // Helper: read a Unity string object at address
    // ------------------------------------------------------------------
    inline std::string ReadUnityString(uintptr_t strPtr)
    {
        if (!strPtr) return "";
        int32_t len = M_base::mem::Read<int32_t>(strPtr + RobRun::IL2CPP::String_length);
        if (len <= 0 || len > 256) return "";
        // Unity strings are UTF-16
        std::wstring ws(len, L'\0');
        M_base::mem::ReadProcessMemoryImpl(strPtr + RobRun::IL2CPP::String_chars, ws.data(), len * sizeof(wchar_t));
        // Convert to UTF-8 narrow string (ASCII safe for player names)
        std::string result(len, '\0');
        for (int i = 0; i < len; i++)
            result[i] = (ws[i] < 128) ? static_cast<char>(ws[i]) : '?';
        return result;
    }

    // ------------------------------------------------------------------
    // IPrimitiveModel<T> — reads the boxed value
    // ------------------------------------------------------------------
    template<typename T>
    inline T ReadPrimitive(uintptr_t primitivePtr)
    {
        if (!primitivePtr) return T{};
        return M_base::mem::Read<T>(primitivePtr + RobRun::IL2CPP::Primitive_value);
    }

    // ------------------------------------------------------------------
    // LastPositionModel — world position
    // ------------------------------------------------------------------
    struct LastPosition
    {
        float X, Y, Z;
        Vector3D ToVec3() const { return Vector3D(X, Y, Z); }
    };

    inline LastPosition ReadLastPosition(uintptr_t ptr)
    {
        LastPosition lp{};
        if (!ptr) return lp;
        lp.X = M_base::mem::Read<float>(ptr + RobRun::LastPositionModel::X);
        lp.Y = M_base::mem::Read<float>(ptr + RobRun::LastPositionModel::Y);
        lp.Z = M_base::mem::Read<float>(ptr + RobRun::LastPositionModel::Z);
        return lp;
    }

    // ------------------------------------------------------------------
    // BattleCharacterModel — a single player/enemy entry
    // ------------------------------------------------------------------
    struct BattleCharacter
    {
        uintptr_t Ptr = 0;

        int         GetId()     const { return M_base::mem::Read<int32_t>(Ptr + RobRun::BattleCharacterModel::Id); }
        bool        IsDead()    const
        {
            uintptr_t deadModel = M_base::mem::Read<uintptr_t>(Ptr + RobRun::BattleCharacterModel::IsDead);
            return ReadPrimitive<bool>(deadModel);
        }
        Vector3D    GetPosition() const
        {
            uintptr_t lastPos = M_base::mem::Read<uintptr_t>(Ptr + RobRun::BattleCharacterModel::LastPositionModel);
            return ReadLastPosition(lastPos).ToVec3();
        }

        // Health: BattleCharacterModel has HealthModel somewhere; use HealthData
        int GetHealth() const
        {
            // HealthModel is not a direct child offset in the provided offsets.
            // We'll return 100 as a placeholder until the HealthModel pointer offset is confirmed.
            return 100;
        }
    };

    // ------------------------------------------------------------------
    // WorldEntitiesModel — holds all entity dictionaries
    // ------------------------------------------------------------------
    struct WorldEntities
    {
        uintptr_t Ptr = 0;

        // Returns pointer to BattleCharacters Dictionary object
        uintptr_t GetBattleCharactersDict() const
        {
            return M_base::mem::Read<uintptr_t>(Ptr + RobRun::WorldEntitiesModel::BattleCharacters);
        }
    };

} // namespace Cheat

