#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>

struct GameplayHudData {
    int lives = 3;
    float currentStamina = 100.0f;
    float maxStamina = 100.0f;
    bool isExhausted = false;
    float shaderTime = 0.0f;
    float cardFlickerTimer = 0.0f;
    int currentCamNumber = 1;
};

struct UiButton {
    Rectangle bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::string text;
    bool isHovered = false;
    bool wasHovered = false;
    float hoverProgress = 0.0f;
    bool isDanger = false;
};

enum class PauseMenuAction {
    NONE,
    RESUME,
    MAIN_MENU
};

class GameplayUI {
public:
    void Init();
    void Cleanup();

    // HUD та екранні оверлеї
    void DrawHud(const GameplayHudData& data) const;
    void DrawTeleportOverlay(float alpha, int camNumber) const;
    void DrawDeathOverlay(float deathTimer, float deathDuration) const;

    // Меню паузи
    void ResetPauseMenu();
    PauseMenuAction UpdatePauseMenu(float dt);
    void DrawPauseMenu() const;

    // Спільна кнопка в стилі Art Deco (можна перевикористовувати і в інших стейтах)
    static void DrawArtDecoButton(const UiButton& btn, float animTime);

private:
    float pauseAnimTime = 0.0f;
    Sound hoverSound = { 0 };
    bool hasHoverSound = false;

    UiButton resumeButton;
    UiButton exitToMenuButton;
};