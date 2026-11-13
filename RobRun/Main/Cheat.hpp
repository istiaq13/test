#pragma once
#include <thread>
#include <Utils/Dependencies/ImGui/imgui.h>
#include <Utils/Dependencies/ImGui/imgui_impl_dx11.h>
#include <Utils/Dependencies/ImGui/imgui_impl_win32.h>

#include "GameSDK/RobRun.hpp"
#include "Features/esp.hpp"

#include "Options.hpp"

namespace Cheat
{
    void C_Start();
    void C_Stop();
}

