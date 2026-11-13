#pragma once
#include <cstdint>

// =====================================================================
//                 ROB RUN - ESP & GAMEPLAY OFFSETS
//             Engine: Unity 6000 (IL2CPP 64-bit, Metadata v39)
// =====================================================================

namespace RobRun
{
    // =================================================================
    // 1. ENTITY LIST & WORLD (WorldEntitiesModel in Models.dll)
    // =================================================================
    namespace WorldEntitiesModel
    {
        constexpr uint32_t BattleCharacters = 0x18;  // Dictionary<int, BattleCharacterModel*> -> ALL PLAYERS / ENEMIES
        constexpr uint32_t Containers       = 0x20;  // Dictionary<int, ContainerModel*>       -> CHESTS / CRATES ESP
        constexpr uint32_t GroundLoots      = 0x38;  // Dictionary<int, ILootModel*>           -> ITEMS ON GROUND ESP
        constexpr uint32_t VehicleLoots     = 0x40;  // Dictionary<int, IVehicleLootModel*>
        constexpr uint32_t Doors            = 0x48;  // Dictionary<int, DoorModel*>
        constexpr uint32_t Vehicles         = 0x58;  // Dictionary<int, VehicleModel*>         -> VEHICLE ESP
        constexpr uint32_t Cars             = 0x60;  // Dictionary<int, CarModel*>
        constexpr uint32_t Helicopters      = 0x68;  // Dictionary<int, HelicopterModel*>
        constexpr uint32_t StaticLoot       = 0x98;  // Dictionary<int, StaticLootObjectModel*>
        constexpr uint32_t RandomLoot       = 0xA0;  // Dictionary<int, RandomLootObjectModel*>
        constexpr uint32_t SafeDoors        = 0x110; // Dictionary<int, SafeDoorModel*>
        constexpr uint32_t CopsNpc          = 0x120; // Dictionary<int, VigilantNpcModel*>     -> POLICE / NPC ESP
    }

    // =================================================================
    // 2. CHARACTER COMPONENT (BattleCharacterModel)
    // =================================================================
    namespace BattleCharacterModel
    {
        constexpr uint32_t Id                  = 0x38;  // int32_t
        constexpr uint32_t Transform           = 0x58;  // ITransformModel* (Unity Transform)
        constexpr uint32_t ReplicatedTransform = 0x60;  // CharacterReplicatedTransformModel*
        constexpr uint32_t LastPositionModel   = 0x68;  // LastPositionModel* (Vector3 X, Y, Z)
        constexpr uint32_t LookDirection       = 0x70;  // LookDirectionModel*
        constexpr uint32_t Velocity            = 0xB8;  // CharacterVelocityModel*
        constexpr uint32_t IsDead              = 0xF0;  // IPrimitiveModel<bool>*
        constexpr uint32_t DamageImmuneModel   = 0x110; // DamageImmuneModel*
        constexpr uint32_t SkeletonModel       = 0x138; // SkeletonModel*
        constexpr uint32_t Animator            = 0x140; // BattleCharacterAnimatorModel*
        constexpr uint32_t IsSprintingModel    = 0x148; // IPrimitiveModel<bool>*
        constexpr uint32_t Weapon              = 0x150; // IWeaponModel*
        constexpr uint32_t WeaponReload        = 0x158; // WeaponReloadModel*
    }

    // =================================================================
    // 3. ESP POSITION / 3D COORDINATES
    // =================================================================
    namespace LastPositionModel
    {
        constexpr uint32_t X = 0x10; // float
        constexpr uint32_t Y = 0x14; // float
        constexpr uint32_t Z = 0x18; // float
    }

    namespace CharacterReplicatedTransformModel
    {
        constexpr uint32_t PositionData = 0x10; // IPrimitiveData<Vector3>*
        constexpr uint32_t RotationX    = 0x18; // float
        constexpr uint32_t RotationY    = 0x20; // float
        constexpr uint32_t RotationZ    = 0x28; // float
    }

    // =================================================================
    // 4. SKELETON ESP / BONES
    // =================================================================
    namespace SkeletonModel
    {
        constexpr uint32_t Description = 0x10; // SkeletonConfig*
        constexpr uint32_t BonesData   = 0x18; // float[] (Bone array)
    }

    // =================================================================
    // 5. HEALTH & STATUS
    // =================================================================
    namespace HealthModel
    {
        constexpr uint32_t HealthData     = 0x10; // IPrimitiveData<int>*
        constexpr uint32_t IsDead         = 0x18; // IPrimitiveModel<bool>*
        constexpr uint32_t MaxAmount      = 0x20; // int32_t
        constexpr uint32_t CriticalHealth = 0x24; // int32_t

        namespace Functions
        {
            constexpr uintptr_t GetAmount  = 0x4907FD0; // int get_Amount()
            constexpr uintptr_t SetHealth  = 0x4907F70; // void SetHealth(int value)
            constexpr uintptr_t Damage     = 0x4907E00; // void Damage(int damage)
            constexpr uintptr_t Heal       = 0x4907EB0; // void Heal(int amount)
            constexpr uintptr_t GetIsDead  = 0x4908070; // bool get_IsDead()
        }
    }

    // =================================================================
    // 6. IL2CPP / UNITY INTERNALS
    // =================================================================
    namespace IL2CPP
    {
        // Unity IL2CPP Dictionary<TKey,TValue> internals
        constexpr uint32_t Dict_entries    = 0x18; // Entry[] array ptr
        constexpr uint32_t Dict_count      = 0x40; // int count
        constexpr uint32_t Entry_key       = 0x10; // TKey at entry start
        constexpr uint32_t Entry_value     = 0x18; // TValue after key
        constexpr uint32_t Entry_stride    = 0x28; // sizeof(Dictionary.Entry)

        // IPrimitiveModel<T> / IPrimitiveData<T>
        constexpr uint32_t Primitive_value = 0x10; // T value

        // Unity string (System.String)
        constexpr uint32_t String_length   = 0x10; // int32_t
        constexpr uint32_t String_chars    = 0x14; // char16_t[]

        // GameInstance -> WorldEntitiesModel
        constexpr uintptr_t GameInstanceStaticPtr = 0x0; // Set at runtime via GameInstance scan
    }
}

