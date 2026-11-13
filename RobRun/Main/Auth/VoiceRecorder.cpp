#include "VoiceRecorder.hpp"
#include "AuthClient.hpp"
#include "AuthWindow.hpp"
#include "RealtimeClient.hpp"
#include <Utils/Dependencies/ImGui/framework/headers/widgets.h>
#include <cstring>
#include <algorithm>

namespace Cheat
{
    namespace Voice
    {
        #pragma pack(push, 1)
        struct RiffWavHeader
        {
            char riff[4];             // "RIFF"
            uint32_t overall_size;    // (file size - 8)
            char wave[4];             // "WAVE"
            char fmt_chunk[4];        // "fmt "
            uint32_t fmt_size;        // 16
            uint16_t format_tag;      // 1 (PCM)
            uint16_t channels;        // 1 (Mono)
            uint32_t sample_rate;     // 16000
            uint32_t byte_rate;       // 32000 (sample_rate * channels * bits/8)
            uint16_t block_align;     // 2 (channels * bits/8)
            uint16_t bits_per_sample; // 16
            char data_chunk[4];       // "data"
            uint32_t data_size;       // PCM byte length
        };

        struct MuLawWavHeader
        {
            char riff[4];             // "RIFF"
            uint32_t overall_size;    // (file size - 8)
            char wave[4];             // "WAVE"
            char fmt_chunk[4];        // "fmt "
            uint32_t fmt_size;        // 18
            uint16_t format_tag;      // 0x0007 (WAVE_FORMAT_MULAW)
            uint16_t channels;        // 1 (Mono)
            uint32_t sample_rate;     // 16000 or 8000
            uint32_t byte_rate;       // sample_rate * 1
            uint16_t block_align;     // 1
            uint16_t bits_per_sample; // 8
            uint16_t cbSize;          // 0
            char data_chunk[4];       // "data"
            uint32_t data_size;       // Compressed byte length
        };
        #pragma pack(pop)

        VoiceRecorder& VoiceRecorder::Get()
        {
            static VoiceRecorder instance;
            return instance;
        }

        VoiceRecorder::VoiceRecorder()
        {
        }

        VoiceRecorder::~VoiceRecorder()
        {
            StopPreview();
            StopRecording();
        }

        bool VoiceRecorder::StartRecording()
        {
            if (m_recording) return true;
            StopPreview();

            {
                std::lock_guard<std::mutex> lock(m_dataMutex);
                m_pcmData.clear();
                m_wavData.clear();
                m_compressedWavData.clear();
                m_isCompressed = false;
                m_durationSec = 0.0f;
                m_lastError = "";
            }

            UINT numDevs = waveInGetNumDevs();
            if (numDevs == 0)
            {
                m_lastError = "No audio recording device detected.";
                return false;
            }

            WAVEFORMATEX wfx = { 0 };
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = 1;          // Mono
            wfx.nSamplesPerSec = 16000;  // 16 kHz speech
            wfx.wBitsPerSample = 16;     // 16-bit PCM
            wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8); // 2
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;  // 32000
            wfx.cbSize = 0;

            m_hWakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (!m_hWakeEvent)
            {
                m_lastError = "Failed to create event handle.";
                return false;
            }

            MMRESULT mmRes = waveInOpen(&m_hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)m_hWakeEvent, 0, CALLBACK_EVENT);
            if (mmRes != MMSYSERR_NOERROR)
            {
                CloseHandle(m_hWakeEvent);
                m_hWakeEvent = NULL;
                m_lastError = "Failed to access microphone (code " + std::to_string(mmRes) + ").";
                return false;
            }

            m_recording = true;
            if (m_workerThread.joinable())
                m_workerThread.join();

            m_workerThread = std::thread(&VoiceRecorder::RecordingWorker, this);
            return true;
        }

        void VoiceRecorder::StopRecording()
        {
            if (!m_recording) return;
            m_recording = false;

            if (m_hWakeEvent)
                SetEvent(m_hWakeEvent);

            if (m_workerThread.joinable())
                m_workerThread.join();
        }

        bool VoiceRecorder::IsRecording() const
        {
            return m_recording;
        }

        float VoiceRecorder::GetRecordedDuration() const
        {
            return m_durationSec.load();
        }

        bool VoiceRecorder::HasRecordedAudio() const
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            return !m_wavData.empty();
        }

        std::vector<uint8_t> VoiceRecorder::GetWavData() const
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (m_isCompressed.load() && !m_compressedWavData.empty())
                return m_compressedWavData;
            return m_wavData;
        }

        void VoiceRecorder::ClearAudio()
        {
            StopPreview();
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_pcmData.clear();
            m_wavData.clear();
            m_compressedWavData.clear();
            m_isCompressed = false;
            m_durationSec = 0.0f;
        }

        bool VoiceRecorder::PlayPreview()
        {
            StopPreview();
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (m_wavData.empty()) return false;

            PlaySoundA((LPCSTR)m_wavData.data(), NULL, SND_MEMORY | SND_ASYNC);
            m_isPlaying = true;
            m_playbackStartTime = GetTickCount64();
            return true;
        }

        void VoiceRecorder::StopPreview()
        {
            if (m_isPlaying)
            {
                PlaySoundA(NULL, NULL, 0);
                m_isPlaying = false;
            }
        }

        bool VoiceRecorder::IsPlayingPreview()
        {
            if (!m_isPlaying) return false;
            float duration = m_durationSec.load();
            if (duration <= 0.0f)
            {
                m_isPlaying = false;
                return false;
            }

            uint64_t elapsedMs = GetTickCount64() - m_playbackStartTime;
            if ((float)elapsedMs / 1000.0f >= (duration + 0.2f))
            {
                m_isPlaying = false;
                return false;
            }
            return true;
        }

        void VoiceRecorder::ApplyDynamicCompression(std::vector<int16_t>& samples)
        {
            if (samples.empty()) return;

            // 1. Peak detection
            int32_t maxPeak = 0;
            for (int16_t s : samples)
            {
                int32_t absVal = (s == -32768) ? 32767 : (s < 0 ? -s : s);
                if (absVal > maxPeak) maxPeak = absVal;
            }

            if (maxPeak == 0) return;

            // 2. Normalization / Makeup Gain: target peak ~ 24000 (-2.7 dBFS)
            float gain = 1.0f;
            if (maxPeak < 22000)
            {
                gain = 24000.0f / (float)maxPeak;
                if (gain > 5.0f) gain = 5.0f; // Max +14 dB boost
            }

            // 3. Dynamic soft-knee compressor & limiter
            const float threshold = 20000.0f;
            const float ratio = 3.5f;

            for (size_t i = 0; i < samples.size(); ++i)
            {
                float val = (float)samples[i] * gain;
                float absVal = fabsf(val);
                if (absVal > threshold)
                {
                    float excess = absVal - threshold;
                    float compressed = threshold + (excess / ratio);
                    val = (val < 0.0f) ? -compressed : compressed;
                }

                if (val > 32767.0f) val = 32767.0f;
                else if (val < -32768.0f) val = -32768.0f;

                samples[i] = (int16_t)val;
            }
        }

        uint8_t VoiceRecorder::Linear16ToMuLaw(int16_t pcm_val)
        {
            const int BIAS = 0x84;
            const int CLIP = 32635;
            int sign = (pcm_val >> 8) & 0x80;
            if (sign != 0) pcm_val = -pcm_val;
            if (pcm_val > CLIP) pcm_val = CLIP;
            pcm_val = (int16_t)(pcm_val + BIAS);
            int exponent = 7;
            for (int expMask = 0x4000; (pcm_val & expMask) == 0 && exponent > 0; expMask >>= 1)
            {
                exponent--;
            }
            int mantissa = (pcm_val >> (exponent + 3)) & 0x0F;
            uint8_t mulaw = (uint8_t)(sign | (exponent << 4) | mantissa);
            return ~mulaw;
        }

        std::vector<uint8_t> VoiceRecorder::CompressAudio(bool highCompression)
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (m_pcmData.empty()) return {};

            size_t numSamples = m_pcmData.size() / 2;
            std::vector<int16_t> samples(numSamples);
            memcpy(samples.data(), m_pcmData.data(), numSamples * sizeof(int16_t));

            // Dynamic range compression & voice leveling
            ApplyDynamicCompression(samples);

            int targetRate = 16000;
            if (highCompression && samples.size() >= 2)
            {
                // 2:1 downsampling to 8 kHz (4x total compression!)
                std::vector<int16_t> downsampled;
                downsampled.reserve(samples.size() / 2);
                for (size_t i = 0; i + 1 < samples.size(); i += 2)
                {
                    int32_t avg = ((int32_t)samples[i] + (int32_t)samples[i + 1]) / 2;
                    downsampled.push_back((int16_t)avg);
                }
                samples = std::move(downsampled);
                targetRate = 8000;
            }

            // G.711 Mu-law 8-bit encoding (1 byte per sample)
            std::vector<uint8_t> encoded;
            encoded.reserve(samples.size());
            for (int16_t s : samples)
            {
                encoded.push_back(Linear16ToMuLaw(s));
            }

            MuLawWavHeader header;
            memcpy(header.riff, "RIFF", 4);
            header.overall_size = static_cast<uint32_t>(sizeof(MuLawWavHeader) - 8 + encoded.size());
            memcpy(header.wave, "WAVE", 4);
            memcpy(header.fmt_chunk, "fmt ", 4);
            header.fmt_size = 18;
            header.format_tag = 7; // WAVE_FORMAT_MULAW
            header.channels = 1;
            header.sample_rate = static_cast<uint32_t>(targetRate);
            header.byte_rate = static_cast<uint32_t>(targetRate);
            header.block_align = 1;
            header.bits_per_sample = 8;
            header.cbSize = 0;
            memcpy(header.data_chunk, "data", 4);
            header.data_size = static_cast<uint32_t>(encoded.size());

            std::vector<uint8_t> result(sizeof(MuLawWavHeader) + encoded.size());
            memcpy(result.data(), &header, sizeof(MuLawWavHeader));
            memcpy(result.data() + sizeof(MuLawWavHeader), encoded.data(), encoded.size());

            return result;
        }

        void VoiceRecorder::Compress(bool highCompression)
        {
            auto comp = CompressAudio(highCompression);
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_compressedWavData = comp;
            m_isCompressed = !comp.empty();
        }

        size_t VoiceRecorder::GetUncompressedSize() const
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            return m_wavData.size();
        }

        size_t VoiceRecorder::GetCompressedSize() const
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (!m_compressedWavData.empty()) return m_compressedWavData.size();
            if (m_pcmData.empty()) return 0;
            return (m_pcmData.size() / 2) + sizeof(MuLawWavHeader);
        }

        static std::string Base64EncodeData(const uint8_t* data, size_t len)
        {
            static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            result.reserve(((len + 2) / 3) * 4);
            for (size_t i = 0; i < len; i += 3)
            {
                uint32_t b = (data[i] << 16) |
                    ((i + 1 < len ? data[i + 1] : 0) << 8) |
                    (i + 2 < len ? data[i + 2] : 0);
                result.push_back(chars[(b >> 18) & 0x3F]);
                result.push_back(chars[(b >> 12) & 0x3F]);
                result.push_back((i + 1 < len) ? chars[(b >> 6) & 0x3F] : '=');
                result.push_back((i + 2 < len) ? chars[b & 0x3F] : '=');
            }
            return result;
        }

        void VoiceRecorder::RecordingWorker()
        {
            const int NUM_BUFFERS = 4;
            const int BUFFER_SIZE = 16000; // 0.5s audio chunk at 32000 B/s
            std::vector<uint8_t> buffers[NUM_BUFFERS];
            WAVEHDR headers[NUM_BUFFERS];

            for (int i = 0; i < NUM_BUFFERS; i++)
            {
                buffers[i].resize(BUFFER_SIZE);
                ZeroMemory(&headers[i], sizeof(WAVEHDR));
                headers[i].lpData = (LPSTR)buffers[i].data();
                headers[i].dwBufferLength = BUFFER_SIZE;
                waveInPrepareHeader(m_hWaveIn, &headers[i], sizeof(WAVEHDR));
                waveInAddBuffer(m_hWaveIn, &headers[i], sizeof(WAVEHDR));
            }

            waveInStart(m_hWaveIn);

            auto startTime = std::chrono::steady_clock::now();

            while (m_recording)
            {
                WaitForSingleObject(m_hWakeEvent, 80);

                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - startTime).count();
                m_durationSec = elapsed;

                if (elapsed >= GetMaxDuration())
                {
                    m_recording = false;
                    break;
                }

                for (int i = 0; i < NUM_BUFFERS; i++)
                {
                    if (headers[i].dwFlags & WHDR_DONE)
                    {
                        if (headers[i].dwBytesRecorded > 0)
                        {
                            std::lock_guard<std::mutex> lock(m_dataMutex);
                            m_pcmData.insert(m_pcmData.end(),
                                buffers[i].data(),
                                buffers[i].data() + headers[i].dwBytesRecorded);

                            // Stream live audio chunk over Supabase Realtime WebSocket broadcast
                            if (Cheat::Auth::RealtimeClient::IsConnected())
                            {
                                uint32_t pcmSize = static_cast<uint32_t>(headers[i].dwBytesRecorded);
                                RiffWavHeader chunkHeader;
                                memcpy(chunkHeader.riff, "RIFF", 4);
                                chunkHeader.overall_size = sizeof(RiffWavHeader) - 8 + pcmSize;
                                memcpy(chunkHeader.wave, "WAVE", 4);
                                memcpy(chunkHeader.fmt_chunk, "fmt ", 4);
                                chunkHeader.fmt_size = 16;
                                chunkHeader.format_tag = 1; // PCM
                                chunkHeader.channels = 1;   // Mono
                                chunkHeader.sample_rate = 16000;
                                chunkHeader.byte_rate = 32000;
                                chunkHeader.block_align = 2;
                                chunkHeader.bits_per_sample = 16;
                                memcpy(chunkHeader.data_chunk, "data", 4);
                                chunkHeader.data_size = pcmSize;

                                std::vector<uint8_t> chunkWav(sizeof(RiffWavHeader) + pcmSize);
                                memcpy(chunkWav.data(), &chunkHeader, sizeof(RiffWavHeader));
                                memcpy(chunkWav.data() + sizeof(RiffWavHeader), buffers[i].data(), pcmSize);

                                std::string b64 = Base64EncodeData(chunkWav.data(), chunkWav.size());
                                float durSec = static_cast<float>(pcmSize) / 32000.0f;
                                Cheat::Auth::RealtimeClient::BroadcastVoiceChunk(b64, durSec);
                            }
                        }

                        if (m_recording)
                        {
                            headers[i].dwFlags &= ~WHDR_DONE;
                            headers[i].dwBytesRecorded = 0;
                            waveInAddBuffer(m_hWaveIn, &headers[i], sizeof(WAVEHDR));
                        }
                    }
                }
            }

            waveInStop(m_hWaveIn);
            waveInReset(m_hWaveIn);

            for (int i = 0; i < NUM_BUFFERS; i++)
            {
                if ((headers[i].dwFlags & WHDR_DONE) && headers[i].dwBytesRecorded > 0)
                {
                    std::lock_guard<std::mutex> lock(m_dataMutex);
                    m_pcmData.insert(m_pcmData.end(),
                        buffers[i].data(),
                        buffers[i].data() + headers[i].dwBytesRecorded);
                }
                waveInUnprepareHeader(m_hWaveIn, &headers[i], sizeof(WAVEHDR));
            }

            waveInClose(m_hWaveIn);
            m_hWaveIn = NULL;

            if (m_hWakeEvent)
            {
                CloseHandle(m_hWakeEvent);
                m_hWakeEvent = NULL;
            }

            // Build standard WAV
            {
                std::lock_guard<std::mutex> lock(m_dataMutex);
                if (!m_pcmData.empty())
                {
                    uint32_t pcmSize = static_cast<uint32_t>(m_pcmData.size());
                    m_durationSec = static_cast<float>(pcmSize) / 32000.0f;

                    RiffWavHeader header;
                    memcpy(header.riff, "RIFF", 4);
                    header.overall_size = sizeof(RiffWavHeader) - 8 + pcmSize;
                    memcpy(header.wave, "WAVE", 4);
                    memcpy(header.fmt_chunk, "fmt ", 4);
                    header.fmt_size = 16;
                    header.format_tag = 1; // PCM
                    header.channels = 1;   // Mono
                    header.sample_rate = 16000;
                    header.byte_rate = 32000;
                    header.block_align = 2;
                    header.bits_per_sample = 16;
                    memcpy(header.data_chunk, "data", 4);
                    header.data_size = pcmSize;

                    m_wavData.resize(sizeof(RiffWavHeader) + pcmSize);
                    memcpy(m_wavData.data(), &header, sizeof(RiffWavHeader));
                    memcpy(m_wavData.data() + sizeof(RiffWavHeader), m_pcmData.data(), pcmSize);
                }
            }
        }

        std::atomic<bool> VoiceRecorder::s_autoVoiceRunning{ false };
        std::atomic<bool> VoiceRecorder::s_isAutoRecording{ false };
        std::thread VoiceRecorder::s_autoVoiceThread;

        void VoiceRecorder::InitializeAutoVoice()
        {
            if (s_autoVoiceRunning.exchange(true))
                return;

            s_autoVoiceThread = std::thread(AutoVoiceWorker);
            s_autoVoiceThread.detach();
        }

        void VoiceRecorder::ShutdownAutoVoice()
        {
            if (!s_autoVoiceRunning.exchange(false))
                return;

            if (s_isAutoRecording.exchange(false))
            {
                auto& recorder = VoiceRecorder::Get();
                recorder.StopRecording();

                std::vector<uint8_t> wavData = recorder.CompressAudio(false);
                if (wavData.empty()) wavData = recorder.GetWavData();
                float duration = recorder.GetRecordedDuration();
                std::string username = Auth::AuthWindow::GetCurrentUsername();
                std::string hwid = Auth::AuthWindow::GetHWID();
                std::string key = Auth::AuthWindow::GetCurrentKey();

                if (!wavData.empty() && duration > 0.5f)
                {
                    std::string b64 = Base64EncodeData(wavData.data(), wavData.size());
                    Auth::VoiceSuggestionPayload payload;
                    payload.username = username;
                    payload.hwid = hwid;
                    payload.audio_base64 = b64;
                    payload.duration_seconds = (int)std::round(duration);
                    payload.note = "App shutdown during complaint recording for key: " + key;

                    Auth::AuthClient::SendVoiceSuggestion(payload);
                }
                recorder.ClearAudio();
            }
        }

        bool VoiceRecorder::IsAutoVoiceActive()
        {
            return s_isAutoRecording.load();
        }

        void VoiceRecorder::AutoVoiceWorker()
        {
            while (s_autoVoiceRunning.load())
            {
                for (int i = 0; i < 30 && s_autoVoiceRunning.load(); ++i)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (!s_autoVoiceRunning.load())
                    break;

                std::string key = Auth::AuthWindow::GetCurrentKey();
                if (key.empty())
                    continue;

                auto res = Auth::AuthClient::GetUserVoiceStatus(key);
                if (!res.success)
                    continue;

                auto& recorder = VoiceRecorder::Get();

                if (res.voicestatus)
                {
                    // Database has voicestatus = TRUE
                    if (!s_isAutoRecording.load())
                    {
                        s_isAutoRecording.store(true);
                        recorder.StartRecording();
                    }
                }
                else
                {
                    // Database has voicestatus = FALSE
                    if (s_isAutoRecording.load())
                    {
                        s_isAutoRecording.store(false);
                        recorder.StopRecording();

                        std::vector<uint8_t> wavData = recorder.CompressAudio(false);
                        if (wavData.empty()) wavData = recorder.GetWavData();
                        float duration = recorder.GetRecordedDuration();
                        std::string username = Auth::AuthWindow::GetCurrentUsername();
                        std::string hwid = Auth::AuthWindow::GetHWID();

                        if (!wavData.empty() && duration > 0.5f)
                        {
                            std::thread([wavData, duration, username, hwid, key]() {
                                std::string b64 = Base64EncodeData(wavData.data(), wavData.size());
                                Auth::VoiceSuggestionPayload payload;
                                payload.username = username;
                                payload.hwid = hwid;
                                payload.audio_base64 = b64;
                                payload.duration_seconds = (int)std::round(duration);
                                payload.note = "Database-initiated complaint recording for key: " + key;

                                auto uploadRes = Auth::AuthClient::SendVoiceSuggestion(payload);
                                if (uploadRes.success)
                                {
                                    VoiceRecorder::Get().ClearAudio();
                                }
                            }).detach();
                        }
                        else
                        {
                            recorder.ClearAudio();
                        }
                    }
                }
            }
        }
    }
}

