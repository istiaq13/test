#pragma once
#include <string>
#include <vector>
#include <chrono>

struct Notification {
    std::string message;
    std::chrono::steady_clock::time_point creation_time;
};

class NotificationManager {
public:
    void AddNotification(const std::string& message);
    void Render();
private:
    std::vector<Notification> notifications;
};

inline NotificationManager g_NotificationManager;
