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

    // Інвентар ключів (3 слоти справа знизу)
    bool hasRoomKey = false;
    bool hasSafeKeyCard = false;
    bool hasMasterKey = false;
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
    RESTART,
    MAIN_MENU
};

class GameplayUI {
public:
    void Init();
    void Cleanup();

    // HUD та екранні оверлеї
    void DrawHud(const GameplayHudData& data) const;
    void DrawInteractPrompt(float animTime) const;
    void DrawStatusNotification(const std::string& text, Color accentColor, float timer, float maxDuration, float animTime) const;
    void DrawDiceRollWorld(Vector2 playerHeadWorldPos, int d1, int d2, int sum, float timer, float duration) const;

    void DrawTeleportOverlay(float alpha, int camNumber) const;
    void DrawDeathOverlay(float deathTimer, float deathDuration) const;

    // Меню паузи
    void ResetPauseMenu();
    PauseMenuAction UpdatePauseMenu(float dt);
    void DrawPauseMenu() const;

    // Спільна кнопка в стилі Art Deco
    static void DrawArtDecoButton(const UiButton& btn, float animTime);

private:
    float pauseAnimTime = 0.0f;
    Sound hoverSound = { 0 };
    bool hasHoverSound = false;

    UiButton resumeButton;
    UiButton restartButton;
    UiButton exitToMenuButton;

    void DrawInventorySlots(const GameplayHudData& data, float screenW, float screenH) const;
};