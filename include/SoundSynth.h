#pragma once

#include "raylib.h"

class SoundSynth {
public:
    void Init();
    void Cleanup();

    void PlayDiceRoll() const;
    void PlayLockedDoor() const;
    void PlayLockFail() const;
    void PlayLockOpen() const;
    void PlayKeyPickup() const;
    void PlayCardPickup() const;
    void PlayCamSwitch() const;
    void PlayGuardAlert() const;

private:
    Sound diceRoll = { 0 };
    Sound lockedDoor = { 0 };
    Sound lockFail = { 0 };
    Sound lockOpen = { 0 };
    Sound keyPickup = { 0 };
    Sound cardPickup = { 0 };
    Sound camSwitch = { 0 };
    Sound guardAlert = { 0 };

    static void SafePlay(const Sound& snd, float pitchMin = 0.96f, float pitchMax = 1.04f);
    static void SafeUnload(Sound& snd);
};