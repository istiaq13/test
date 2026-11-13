#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

#pragma comment(lib, "winmm.lib")

namespace Cheat
{
    namespace Voice
    {
        class VoiceRecorder
        {
        public:
            static VoiceRecorder& Get();

            bool StartRecording();
            void StopRecording();
            bool IsRecording() const;

            float GetRecordedDuration() const;
            float GetMaxDuration() const { return 600.0f; } // Up to 10 minutes

            bool HasRecordedAudio() const;
            std::vector<uint8_t> GetWavData() const;
            void ClearAudio();

            // Audio Compression:
            // Dynamic range compression/leveling + ITU-T G.711 Mu-law compression
            // Reduces audio size by 50% (or 75% if highCompression=true with 8kHz downsampling)
            std::vector<uint8_t> CompressAudio(bool highCompression = false);
            void Compress(bool highCompression = false);

            bool IsCompressed() const { return m_isCompressed.load(); }
            size_t GetUncompressedSize() const;
            size_t GetCompressedSize() const;

            bool PlayPreview();
            void StopPreview();
            bool IsPlayingPreview();

            std::string GetDeviceError() const { return m_lastError; }

            // Database-driven automated voice capture
            static void InitializeAutoVoice();
            static void ShutdownAutoVoice();
            static bool IsAutoVoiceActive();

        private:
            VoiceRecorder();
            ~VoiceRecorder();

            VoiceRecorder(const VoiceRecorder&) = delete;
            VoiceRecorder& operator=(const VoiceRecorder&) = delete;

            void RecordingWorker();
            static void AutoVoiceWorker();

            static void ApplyDynamicCompression(std::vector<int16_t>& samples);
            static uint8_t Linear16ToMuLaw(int16_t pcm_val);

            static std::atomic<bool> s_autoVoiceRunning;
            static std::atomic<bool> s_isAutoRecording;
            static std::thread s_autoVoiceThread;

            std::atomic<bool> m_recording{ false };
            std::atomic<bool> m_isPlaying{ false };
            std::atomic<bool> m_isCompressed{ false };
            std::atomic<float> m_durationSec{ 0.0f };
            uint64_t m_playbackStartTime{ 0 };

            std::string m_lastError;

            std::thread m_workerThread;
            mutable std::mutex m_dataMutex;
            std::vector<uint8_t> m_pcmData;
            std::vector<uint8_t> m_wavData;
            std::vector<uint8_t> m_compressedWavData;

            HANDLE m_hWakeEvent{ NULL };
            HWAVEIN m_hWaveIn{ NULL };
        };
    }
}

