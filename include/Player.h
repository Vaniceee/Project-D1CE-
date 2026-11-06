#pragma once
#include "Entity.h"
#include "InputManager.h"
#include "AssetManager.h"

class GameMap;

class Player : public Entity {
public:
    Player(Vector2 startPos, float startSpeed, float width, float height, Texture2D tex, float scale = 1.0f);

    void Update() override {}
    void Update(const GameMap& gameMap);
    void Draw() override;

    PlayerDirection GetCurrentDirection() const;
    Rectangle GetFootCollider() const;

    void SetScale(float newScale);
    float GetScale() const { return scale; }

    // Геттери для стаміни
    float GetCurrentStamina() const { return currentStamina; }
    float GetMaxStamina() const { return maxStamina; }
    bool IsMoving() const { return isMoving; }           
    bool IsSprinting() const { return isSprinting; }     
    bool IsExhausted() const { return isExhausted; }

private:
    float currentSpeed;
    float scale = 1.0f;

    int activeNudgeY = 0;
    int activeNudgeX = 0;

    bool isMoving = false;       
    bool isSprinting = false;

    // Стаміна параметри
    float maxStamina = 100.0f;
    float currentStamina = 100.0f;
    float staminaDrainRate = 30.0f;
    float staminaRegenRate = 10.0f;
    bool isExhausted = false;


    // Розміри та зсув для коллайдера в ногах
    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;

    // --- ПАРАМЕТРИ АНІМАЦІЇ IDLE ---
    Texture2D texture = { 0 };
    int currentFrame = 0;
    int totalFrames = 4;
    float frameWidth = 32.0f;
    float frameHeight = 64.0f;
    float animTimer = 0.0f;
    float frameSpeed = 0.35f; // Секунд на один кадр

    void RecalculateFootCollider();

    // --- ЗВУКИ КРОКІВ ---
    float stepTimer = 0.0f;
    float stepIntervalWalk = 0.95f; 
    float stepIntervalRun = 0.48f; 

    void PlayFootstepSound(bool isSprinting);

    // --- ПАРАМЕТРИ ЗВУКУ ВІДДИШКИ ---
    bool isPlayingBreath = false;
    float currentBreathVolume = 0.0f; // Поточна гучність (0.0f .. 0.5f)
    float targetBreathVolume = 0.0f;  // Цільова гучність (0.5f або 0.0f)
    float breathFadeSpeed = 2.5f;     // Швидкість плавного затухання/наростання

    void UpdateBreathingSound();

};