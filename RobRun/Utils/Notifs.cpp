#include "Notifs.hpp"
#include <Utils/Dependencies/ImGui/imgui.h>

void NotificationManager::AddNotification(const std::string& message) {
    notifications.push_back({ message, std::chrono::steady_clock::now() });
}

void NotificationManager::Render() {
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("Notifications", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    auto now = std::chrono::steady_clock::now();
    for (auto it = notifications.begin(); it != notifications.end();) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->creation_time).count();
        if (duration > 5) {
            it = notifications.erase(it);
        } else {
            ImGui::Text("%s", it->message.c_str());
            ++it;
        }
    }

    ImGui::End();
}
