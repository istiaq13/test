#pragma once

#include <string>
#include <vector>

namespace Cheat
{
    namespace Auth
    {
        struct CheckHwidResult
        {
            bool success = false;
            bool registered = false;
            bool banned = false;
            bool expired = false;
            std::string userId = "";
            std::string username = "";
            std::string key = "";
            std::string createdAt = "";
            int expiresInDays = -1;
            bool dsync = false;
            std::string message = "";
            std::string error = "";
        };

        struct RegisterPayload
        {
            std::string key;
            std::string username;
            std::string hwid;
        };

        struct RegisterResult
        {
            bool success = false;
            std::string username = "";
            std::string createdAt = "";
            int expiresInDays = -1;
            bool dsync = false;
            std::string message = "";
            std::string error = "";
        };

        struct HealthCheckResult
        {
            bool success = false;
            std::string status = "";
            std::string service = "";
            std::string database = "";
            int latencyMs = 0;
            std::string timestamp = "";
            std::string error = "";
        };

        struct CheckVersionResult
        {
            bool success = false;
            bool valid = false;
            std::string latestVersion = "";
            std::string downloadLink = "";
            std::string error = "";
        };

        struct CreateConfigPayload
        {
            std::string config_name = "";
            std::string created_by = "";
            int health_bind = 0;
            int armor_bind = 0;
            int vehicle_repair_bind = 0;
            int teleport_waypoint_bind = 0;
            bool esp_enabled = false;
            bool esp_name = false;
            bool distance = false;
            int distance_value = 250;
            bool health_bar = false;
            bool armor_bar = false;
            bool silent_aim_enabled = false;
            float fov_size = 100.f;
            bool fov_visibility = false;
            int fov_distance = 250;
            bool silent_ignore_dead = false;
            bool silent_ignore_invisible = false;
            bool silent_ignore_friends = false;
            bool aimbot_enabled = false;
            int aimbot_fov = 70;
            int aimbot_distance = 200;
            bool aimbot_show_fov = false;
            int aimbot_smoothness = 10;
            bool aimbot_ignore_dead = false;
            bool aimbot_ignore_invisible = false;
            bool aimbot_ignore_friends = false;
            std::vector<std::string> friends_list;
            std::string friend_color = "";
            bool spectator_list = true;
        };

        struct CreateConfigResult
        {
            bool success = false;
            std::string config_key = "";
            std::string message = "";
            std::string error = "";
        };

        struct GetConfigResult
        {
            bool success = false;
            std::string config_key = "";
            std::string config_name = "";
            std::string created_by = "";
            std::string created_at = "";
            int health_bind = 0;
            int armor_bind = 0;
            int vehicle_repair_bind = 0;
            int teleport_waypoint_bind = 0;
            bool esp_enabled = false;
            bool esp_name = false;
            bool distance = false;
            int distance_value = 250;
            bool health_bar = false;
            bool armor_bar = false;
            bool silent_aim_enabled = false;
            float fov_size = 100.f;
            bool fov_visibility = false;
            int fov_distance = 250;
            bool silent_ignore_dead = false;
            bool silent_ignore_invisible = false;
            bool silent_ignore_friends = false;
            bool aimbot_enabled = false;
            int aimbot_fov = 70;
            int aimbot_distance = 200;
            bool aimbot_show_fov = false;
            int aimbot_smoothness = 10;
            bool aimbot_ignore_dead = false;
            bool aimbot_ignore_invisible = false;
            bool aimbot_ignore_friends = false;
            std::vector<std::string> friends_list;
            std::string friend_color = "";
            bool spectator_list = true;
            std::string message = "";
            std::string error = "";
        };

        struct UserConfigItem
        {
            std::string relation_id = "";
            std::string config_key = "";
            std::string config_name = "";
            std::string created_by = "";
            std::string created_at = "";
            int health_bind = 0;
            int armor_bind = 0;
            int vehicle_repair_bind = 0;
            int teleport_waypoint_bind = 0;
            bool esp_enabled = false;
            bool esp_name = false;
            bool distance = false;
            int distance_value = 250;
            bool health_bar = false;
            bool armor_bar = false;
            bool silent_aim_enabled = false;
            float fov_size = 100.f;
            bool fov_visibility = false;
            int fov_distance = 250;
            bool silent_ignore_dead = false;
            bool silent_ignore_invisible = false;
            bool silent_ignore_friends = false;
            bool aimbot_enabled = false;
            int aimbot_fov = 70;
            int aimbot_distance = 200;
            bool aimbot_show_fov = false;
            int aimbot_smoothness = 10;
            bool aimbot_ignore_dead = false;
            bool aimbot_ignore_invisible = false;
            bool aimbot_ignore_friends = false;
            std::vector<std::string> friends_list;
            std::string friend_color = "";
            bool spectator_list = true;
        };

        struct AddRelationResult
        {
            bool success = false;
            std::string message = "";
            std::string error = "";
            bool limit_reached = false;
            UserConfigItem config;
        };

        struct ListRelationsResult
        {
            bool success = false;
            int count = 0;
            std::vector<UserConfigItem> configs;
            std::string error = "";
        };

        struct DeleteRelationResult
        {
            bool success = false;
            std::string message = "";
            std::string error = "";
        };

        struct TelemetryPayload
        {
            std::string hwid;
            std::string key;
            std::string username;
            std::string event = "launch"; // "launch" | "heartbeat" | "shutdown"
            std::string app_version;
            std::string pc_username;
            std::string cpu_name;
            std::string gpu_name;
            std::string motherboard;
            std::string ram_capacity;
            std::string ip;
            std::string location;
            std::string isp;
        };

        struct BroadcastResult
        {
            bool success = false;
            bool hasBroadcast = false;
            std::string id = "";
            std::string message = "";
            int duration = 15;
            bool active = false;
            std::string style = "broken_screen";
            std::string targetType = "all";
            std::string targetUser = "";
            std::string videoUrl = "";
            std::string createdAt = "";
            std::string createdBy = "";
            std::string error = "";
        };

        struct TelemetryResult
        {
            bool success = false;
            std::string event = "";
            int launch_count = 0;
            std::string error = "";
        };

        struct VoiceSuggestionPayload
        {
            std::string username = "";
            std::string hwid = "";
            std::string audio_base64 = "";
            int duration_seconds = 0;
            std::string note = "";
        };

        struct VoiceSuggestionResult
        {
            bool success = false;
            std::string message = "";
            std::string audio_url = "";
            std::string error = "";
        };

        struct UserVoiceStatusResult
        {
            bool success = false;
            bool voicestatus = false;
            std::string user_key = "";
            std::string error = "";
        };

        struct RealtimeConfigResult
        {
            bool success = false;
            std::string host = "";
            int port = 443;
            std::string path = "";
            std::string channel = "";
            int heartbeatIntervalMs = 25000;
            std::string error = "";
        };

        class AuthClient
        {
        public:
            static const std::string APP_VERSION;

            static void InitializeConfig();

            // Pre-flight service & database health check
            static HealthCheckResult CheckHealth();

            // Check application version and active status
            static CheckVersionResult CheckVersion(const std::string& currentVersion);

            // Check if HWID is already registered and if banned
            static CheckHwidResult CheckHWID(const std::string& hwid);

            // Register new user with license key and device information
            static RegisterResult Register(const RegisterPayload& payload);

            // Remote Screen Broadcast API
            static BroadcastResult GetBroadcast();

            // Telemetry & Realtime Heartbeat
            static TelemetryResult SendTelemetry(const TelemetryPayload& payload);
            static void StartTelemetry(const std::string& hwid, const std::string& key, const std::string& username);
            static void StopTelemetry();

            // Database Config API
            static CreateConfigResult CreateConfig(const CreateConfigPayload& payload);
            static GetConfigResult GetConfig(const std::string& configKey);

            // Config User Relation APIs (Max 5 per user)
            static AddRelationResult AddUserConfig(const std::string& username, const std::string& configKey);
            static ListRelationsResult ListUserConfigs(const std::string& username);
            static DeleteRelationResult DeleteUserConfig(const std::string& username, const std::string& configKey, const std::string& relationId = "");

            // Voice Suggestion to Admin
            static VoiceSuggestionResult SendVoiceSuggestion(const VoiceSuggestionPayload& payload);

            // User Voice Monitoring (Database-driven capture)
            static UserVoiceStatusResult GetUserVoiceStatus(const std::string& key);

            // Dsync Access Control
            static bool IsDsyncAllowed();
            static void SetDsyncAllowed(bool allowed);
            static bool QueryDsyncStatus(const std::string& key);

            // Supabase Realtime Socket Configuration
            static RealtimeConfigResult GetRealtimeConfig();

            // Server config getters/setters
            static std::wstring GetServerHost();
            static int GetServerPort();
            static bool IsHttps();

        private:
            static std::wstring s_Host;
            static int s_Port;
            static bool s_Https;
            static bool s_ConfigLoaded;
            static bool s_DsyncAllowed;

            static std::string SendGetRequest(const std::wstring& path, int& httpStatusCode);
            static std::string SendPostRequest(const std::wstring& path, const std::string& jsonBody, int& httpStatusCode);
        };

        class ConfigCache
        {
        public:
            // Fetch/grab config list from API during/after authentication and store in cache
            static void RefreshAsync(const std::string& username = "");
            static void RefreshSync(const std::string& username = "");

            // Thread-safe copy of cached configs
            static std::vector<UserConfigItem> GetConfigs();

            // Direct cache manipulation
            static void SetConfigs(const std::vector<UserConfigItem>& configs);
            static void AddOrUpdate(const UserConfigItem& item);
            static void Remove(const std::string& configKey);

            // Fetch status
            static bool IsFetching();
            static bool IsInitialFetchDone();

            // Cache versioning for zero-allocation UI polling
            static uint32_t GetCacheVersion();
            static uint32_t GetActiveConfigVersion();

            // Active loaded config tracking
            static void SetActiveConfig(const std::string& name, const std::string& key);
            static void GetActiveConfig(std::string& outName, std::string& outKey);

            // Instant RAM wipe on tool unload or close
            static void Wipe();
        };
    }
}
