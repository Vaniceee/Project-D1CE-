#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>

class GameCamera {
public:
    void Init(Vector2 startPos, int virtualWidth, int virtualHeight, float zoom = 1.8f);
    void Update(Vector2 targetPos, bool isMoving, bool isSprinting, bool isExhausted, float staminaRatio, float dt);

    // Допоміжний метод для зовнішніх подій (наприклад, удар охоронця)
    void AddTrauma(float amount);

    const Camera2D& GetCamera() const { return camera; }
    Vector2 GetScreenPosition(Vector2 worldPos) const;

private:
    Camera2D camera = { 0 };
    Vector2 currentTarget = { 0.0f, 0.0f };
    float lerpSpeed = 7.0f;

    // Параметри процедурного похитування (фігура Ліссажу)
    float breathTimer = 0.0f;
    float currentBreathSpeed = 2.0f;
    float currentAmpX = 1.0f;
    float currentAmpY = 1.5f;

    // Параметри тряски (Trauma Shake)
    float trauma = 0.0f;
    float maxShakeOffset = 6.0f;
};