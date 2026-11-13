#include "GameCamera.h"

void GameCamera::Init(Vector2 startPos, int virtualWidth, int virtualHeight, float zoom) {
    camera.target = startPos;
    currentTarget = startPos;
    camera.offset = { (float)virtualWidth / 2.0f, (float)virtualHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = zoom;

    breathTimer = 0.0f;
    currentBreathSpeed = 2.0f;
    currentAmpX = 1.0f;
    currentAmpY = 1.5f;
    trauma = 0.0f;
}

void GameCamera::AddTrauma(float amount) {
    trauma += amount;
    if (trauma > 1.0f) trauma = 1.0f;
}

void GameCamera::Update(Vector2 targetPos, bool isMoving, bool isSprinting, bool isExhausted, float staminaRatio, float dt) {
    // 1. Розрахунок параметрів похитування (Idle -> Walk -> Sprint)
    float targetSpeed = isMoving ? (isSprinting ? 14.0f : 9.0f) : 2.0f;
    float targetAmpX = isMoving ? (isSprinting ? 1.9f : 1.0f) : 0.5f;
    float targetAmpY = isMoving ? (isSprinting ? 2.5f : 1.5f) : 0.9f;

    float blendAlpha = 1.0f - expf(-8.0f * dt);
    currentBreathSpeed = Lerp(currentBreathSpeed, targetSpeed, blendAlpha);
    currentAmpX = Lerp(currentAmpX, targetAmpX, blendAlpha);
    currentAmpY = Lerp(currentAmpY, targetAmpY, blendAlpha);

    breathTimer += dt * currentBreathSpeed;

    // Фігура Ліссажу
    Vector2 breathOffset = {
        cosf(breathTimer * 0.5f) * currentAmpX,
        sinf(breathTimer) * currentAmpY
    };

    // 2. Накопичення та згасання травми (Trauma Shake)
    if (isExhausted) {
        if (trauma < 0.40f) trauma = 0.40f;
    }
    else if (staminaRatio < 0.35f) {
        float lowStaminaFactor = (0.35f - staminaRatio) / 0.35f;
        float targetTrauma = lowStaminaFactor * 0.22f;
        if (trauma < targetTrauma) trauma = targetTrauma;
    }

    if (trauma > 0.0f) {
        trauma -= dt * 0.7f;
        if (trauma < 0.0f) trauma = 0.0f;
    }

    // Квадратична інтенсивність зсуву (trauma^2)
    float shakeIntensity = trauma * trauma;
    Vector2 shakeOffset = {
        ((float)GetRandomValue(-100, 100) / 100.0f) * maxShakeOffset * shakeIntensity,
        ((float)GetRandomValue(-100, 100) / 100.0f) * maxShakeOffset * shakeIntensity
    };

    // 3. Плавне слідування камери за ціллю
    float cameraAlpha = 1.0f - expf(-lerpSpeed * dt);
    currentTarget = Vector2Lerp(currentTarget, targetPos, cameraAlpha);

    camera.target = Vector2Add(currentTarget, Vector2Add(breathOffset, shakeOffset));
}

Vector2 GameCamera::GetScreenPosition(Vector2 worldPos) const {
    return GetWorldToScreen2D(worldPos, camera);
}