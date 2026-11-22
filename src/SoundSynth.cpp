#include "SoundSynth.h"
#include "raymath.h"
#include <cmath>

namespace {
    float PseudoNoise(unsigned int n) {
        n = (n << 13U) ^ n;
        unsigned int m = (n * (n * n * 15731U + 789221U) + 1376312589U) & 0x7fffffffU;
        return 1.0f - (float)m / 1073741824.0f;
    }

    Sound BuildSoundFromSamples(const short* samples, unsigned int frameCount, float volume) {
        Wave w = { 0 };
        w.sampleRate = 44100;
        w.sampleSize = 16;
        w.channels = 1;
        w.frameCount = frameCount;
        w.data = (void*)samples;

        Sound snd = LoadSoundFromWave(w);
        UnloadWave(w);

        if (snd.stream.buffer != nullptr) {
            SetSoundVolume(snd, volume);
        }
        return snd;
    }
}

void SoundSynth::SafePlay(const Sound& snd, float pitchMin, float pitchMax) {
    if (snd.stream.buffer == nullptr) return;
    float pitch = pitchMin + ((float)GetRandomValue(0, 100) / 100.0f) * (pitchMax - pitchMin);
    SetSoundPitch(snd, pitch);
    PlaySound(snd);
}

void SoundSynth::SafeUnload(Sound& snd) {
    if (snd.stream.buffer != nullptr) {
        UnloadSound(snd);
        snd = { 0 };
    }
}

void SoundSynth::Init() {
    const unsigned int sr = 44100;

    // 1. КИДОК КУБИКІВ (5 сухих ударів зі спадаючим інтервалом)
    {
        unsigned int frames = (unsigned int)(sr * 0.42f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            const float hits[5] = { 0.00f, 0.085f, 0.155f, 0.215f, 0.265f };
            const float amps[5] = { 1.00f, 0.78f,  0.58f,  0.40f,  0.24f };
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float mix = 0.0f;
                for (int h = 0; h < 5; ++h) {
                    float lt = t - hits[h];
                    if (lt >= 0.0f && lt < 0.045f) {
                        float env = expf(-lt * 135.0f);
                        float click = sinf(2.0f * PI * (1450.0f + h * 110.0f) * lt) * 0.55f
                            + sinf(2.0f * PI * 3100.0f * lt) * 0.30f
                            + PseudoNoise(i + h * 97U) * 0.35f;
                        mix += click * env * amps[h];
                    }
                }
                data[i] = (short)Clamp(mix * 15000.0f, -32767.0f, 32767.0f);
            }
            diceRoll = BuildSoundFromSamples(data, frames, 0.38f);
        }
    }

    // 2. ЗАЧИНЕНІ ДВЕРІ / СЕЙФ (глухий подвійний стукіт ручки)
    {
        unsigned int frames = (unsigned int)(sr * 0.28f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            const float hits[2] = { 0.00f, 0.095f };
            const float amps[2] = { 1.00f, 0.72f };
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float mix = 0.0f;
                for (int h = 0; h < 2; ++h) {
                    float lt = t - hits[h];
                    if (lt >= 0.0f) {
                        float env = expf(-lt * 55.0f);
                        float thud = sinf(2.0f * PI * 145.0f * lt) * 0.70f
                            + sinf(2.0f * PI * 310.0f * lt) * 0.30f
                            + PseudoNoise(i + h * 31U) * 0.25f * expf(-lt * 120.0f);
                        mix += thud * env * amps[h];
                    }
                }
                data[i] = (short)Clamp(mix * 16500.0f, -32767.0f, 32767.0f);
            }
            lockedDoor = BuildSoundFromSamples(data, frames, 0.42f);
        }
    }

    // 3. НЕВДАЛИЙ КИДОК НА ЗАМОК (зрив пружини вниз)
    {
        unsigned int frames = (unsigned int)(sr * 0.24f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float env = expf(-t * 24.0f);
                float freq = Lerp(420.0f, 155.0f, Clamp(t / 0.20f, 0.0f, 1.0f));
                float sig = sinf(2.0f * PI * freq * t) * 0.65f
                    + sinf(2.0f * PI * (freq * 1.48f) * t) * 0.30f;
                data[i] = (short)Clamp(sig * env * 13000.0f, -32767.0f, 32767.0f);
            }
            lockFail = BuildSoundFromSamples(data, frames, 0.32f);
        }
    }

    // 4. УСПІШНЕ ВІДКРИТТЯ ЗАМКА / СЕЙФА (клац ригеля + золотий дзвін)
    {
        unsigned int frames = (unsigned int)(sr * 0.36f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float clickEnv = expf(-t * 140.0f);
                float click = (sinf(2.0f * PI * 1950.0f * t) + PseudoNoise(i) * 0.5f) * clickEnv;

                float chime = 0.0f;
                if (t > 0.055f) {
                    float ct = t - 0.055f;
                    float cEnv = expf(-ct * 16.0f);
                    chime = (sinf(2.0f * PI * 1318.5f * ct) * 0.6f
                        + sinf(2.0f * PI * 1975.5f * ct) * 0.4f) * cEnv;
                }

                float mix = click * 0.75f + chime * 0.85f;
                data[i] = (short)Clamp(mix * 15000.0f, -32767.0f, 32767.0f);
            }
            lockOpen = BuildSoundFromSamples(data, frames, 0.38f);
        }
    }

    // 5. ПІДБІР МЕТАЛЕВОГО КЛЮЧА (RoomKey / MasterKey)
    {
        unsigned int frames = (unsigned int)(sr * 0.45f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            const float notes[3] = { 1567.98f, 2093.00f, 2637.02f };
            const float starts[3] = { 0.00f, 0.045f, 0.095f };
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float mix = 0.0f;
                for (int n = 0; n < 3; ++n) {
                    float lt = t - starts[n];
                    if (lt >= 0.0f) {
                        float env = expf(-lt * 14.0f);
                        mix += sinf(2.0f * PI * notes[n] * lt) * env * 0.5f;
                    }
                }
                data[i] = (short)Clamp(mix * 14000.0f, -32767.0f, 32767.0f);
            }
            keyPickup = BuildSoundFromSamples(data, frames, 0.34f);
        }
    }

    // 6. ПІДБІР КЛЮЧ-КАРТКИ СЕЙФА (електронний подвійний біп)
    {
        unsigned int frames = (unsigned int)(sr * 0.30f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float sig = 0.0f;
                if (t < 0.08f) {
                    float env = sinf((t / 0.08f) * PI);
                    sig = sinf(2.0f * PI * 1174.66f * t) * env;
                }
                else if (t >= 0.11f && t < 0.22f) {
                    float lt = t - 0.11f;
                    float env = sinf((lt / 0.11f) * PI);
                    sig = sinf(2.0f * PI * 1567.98f * lt) * env;
                }
                data[i] = (short)Clamp(sig * 12500.0f, -32767.0f, 32767.0f);
            }
            cardPickup = BuildSoundFromSamples(data, frames, 0.32f);
        }
    }

    // 7. ПЕРЕМИКАННЯ КАМЕРИ / ТЕЛЕПОРТ (реле + короткий сплеск статичного шуму CCTV)
    {
        unsigned int frames = (unsigned int)(sr * 0.22f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float env = expf(-t * 22.0f);
                float relay = sinf(2.0f * PI * 240.0f * t) * expf(-t * 95.0f) * 0.8f;
                float staticNoise = PseudoNoise(i * 3U) * env * 0.45f;
                float mix = relay + staticNoise;
                data[i] = (short)Clamp(mix * 13500.0f, -32767.0f, 32767.0f);
            }
            camSwitch = BuildSoundFromSamples(data, frames, 0.28f);
        }
    }

    // 8. ТРИВОГА ОХОРОНЦЯ (різкий саб-бас удар + дисонанс при виявленні гравця)
    {
        unsigned int frames = (unsigned int)(sr * 0.55f);
        short* data = (short*)RL_MALLOC(frames * sizeof(short));
        if (data != nullptr) {
            for (unsigned int i = 0; i < frames; ++i) {
                float t = (float)i / (float)sr;
                float attack = Clamp(t / 0.012f, 0.0f, 1.0f);
                float env = attack * expf(-t * 5.2f);
                float subFreq = Lerp(115.0f, 52.0f, Clamp(t / 0.45f, 0.0f, 1.0f));
                float sub = sinf(2.0f * PI * subFreq * t) * 0.75f;
                float tension = sinf(2.0f * PI * 277.18f * t) * 0.25f
                    + sinf(2.0f * PI * 293.66f * t) * 0.25f;
                float mix = (sub + tension * expf(-t * 8.0f)) * env;
                data[i] = (short)Clamp(mix * 18500.0f, -32767.0f, 32767.0f);
            }
            guardAlert = BuildSoundFromSamples(data, frames, 0.45f);
        }
    }
}

void SoundSynth::Cleanup() {
    SafeUnload(diceRoll);
    SafeUnload(lockedDoor);
    SafeUnload(lockFail);
    SafeUnload(lockOpen);
    SafeUnload(keyPickup);
    SafeUnload(cardPickup);
    SafeUnload(camSwitch);
    SafeUnload(guardAlert);
}

void SoundSynth::PlayDiceRoll() const { SafePlay(diceRoll, 0.94f, 1.06f); }
void SoundSynth::PlayLockedDoor() const { SafePlay(lockedDoor, 0.95f, 1.03f); }
void SoundSynth::PlayLockFail() const { SafePlay(lockFail, 0.96f, 1.04f); }
void SoundSynth::PlayLockOpen() const { SafePlay(lockOpen, 0.98f, 1.04f); }
void SoundSynth::PlayKeyPickup() const { SafePlay(keyPickup, 0.98f, 1.03f); }
void SoundSynth::PlayCardPickup() const { SafePlay(cardPickup, 1.00f, 1.00f); }
void SoundSynth::PlayCamSwitch() const { SafePlay(camSwitch, 0.95f, 1.05f); }
void SoundSynth::PlayGuardAlert() const { SafePlay(guardAlert, 0.97f, 1.02f); }