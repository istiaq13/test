#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <unordered_map>

#include "Classes.hpp"
#include <Utils/FrameWork.hpp>
#include <Utils/Math/Vectors/Vector3D.hpp>
#include <Utils/Dependencies/ImGui/imgui.h>

// =====================================================================
//           ROB RUN Framework - CRobRunFramework
// =====================================================================

namespace Cheat
{
    // Per-entity cached data used by ESP renderer
    struct RREntity
    {
        uintptr_t   Ptr         = 0;
        int         Id          = 0;
        std::string Name        = "";   // display name
        bool        IsLocal     = false;
        bool        Dead        = false;
        bool        IsFriend    = false;

        Vector3D    WorldPos;
        int         Health      = 100;
        int         MaxHealth   = 100;
        int         Distance    = 0;

        // Projected screen positions
        ImVec2      ScreenPos;          // feet / bottom-centre
        ImVec2      HeadPos;
        bool        OnScreen    = false;

        // Box corners (top-left, bottom-right in screen space)
        ImVec2      BoxMin;
        ImVec2      BoxMax;
        float       BoxHeight   = 0.f;
        float       BoxWidth    = 0.f;
    };

    // ------------------------------------------------------------------
    class CRobRunFramework
    {
    public:
        // --- Lifecycle -------------------------------------------------
        bool    Initialize();
        bool    IsInitialized() const { return m_Initialized; }
        DWORD   GetPid()        const { return m_Pid; }

        // --- Entity list (called from background threads) --------------
        void    GetEntityList();
        void    UpdateEntityList();     // ~8 Hz loop

        // --- Per-frame (called on render thread) -----------------------
        void    UpdateViewMatrix();
        bool    IsViewMatrixValid() const { return m_bMatrixValid; }

        // --- ESP projection -------------------------------------------
        ImVec2  WorldToScreen(Vector3D pos) const;
        bool    IsOnScreen(ImVec2 pos) const;

        // --- Data access (render-thread safe, locked) -----------------
        std::vector<RREntity>& GetEntities()   { return m_EntityList; }
        std::mutex& GetLock()                  { return m_Lock; }

        // --- Local player position (for distance calc) ----------------
        Vector3D GetLocalPos() const { return m_LocalPos; }

        // --- Friend list ----------------------------------------------
        std::unordered_map<int, bool> FriendIdList;      // keyed by game ID

        // --- Process info ---------------------------------------------
        DWORD    m_Pid            = 0;
        uint64_t m_ModuleBase     = 0;
        uint64_t m_UnityPlayerBase = 0;

    private:
        bool    m_Initialized   = false;
        bool    m_bMatrixValid  = false;
        bool    m_bHasVPMatrix  = false;    // true when full VP matrix is available

        // WorldEntitiesModel pointer (found each init cycle)
        uintptr_t m_WorldEntitiesPtr = 0;

        // Cached entity list (updated in background)
        std::vector<RREntity> m_EntityList;
        std::mutex m_Lock;

        // Local player world position
        Vector3D m_LocalPos;

        // Screen dimensions (read from game window)
        float m_ScreenW = 0.f;
        float m_ScreenH = 0.f;

        // View-projection matrix (column-major, 4x4)
        float m_VP[16] = {};

        // --- Internal helpers -----------------------------------------
        uintptr_t FindGameInstance();
        uintptr_t FindWorldEntities();
        void      ReadDictionary(uintptr_t dictPtr,
                                 std::function<void(uintptr_t entryPtr)> cb);
    };

    inline CRobRunFramework C_RobRun;

} // namespace Cheat

