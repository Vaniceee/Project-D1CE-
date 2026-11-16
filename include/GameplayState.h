#pragma once

#include "GameState.h"
#include "Player.h"
#include "Guard.h"
#include "Map.h"
#include "RenderSystem.h"
#include "GameCamera.h"
#include "AssetManager.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <cmath>
#include <memory>

struct PauseButton {
    Rectangle bounds;
    std::string text;
    bool isHovered = false;
    bool wasHovered = false;
    float hoverProgress = 0.0f;
    bool isDanger = false;
};

class GameplayState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    GameMap gameMap;
    RenderSystem renderSystem;
    GameCamera gameCamera;

    std::unique_ptr<Player> player;
    std::unique_ptr<Guard> guard;

    Music bgMusic = { 0 };

    // --- UI КАРТИ ЖИТТІВ ---
    int lastPlayerLives = 3;
    float cardFlickerTimer = 0.0f;
    float cardFlickerDuration = 0.45f;

    const int virtualWidth = 1280;
    const int virtualHeight = 720;

    RenderTexture2D targetTexture = { 0 };

    // --- ШЕЙДЕР ОСВІТЛЕННЯ ТА СТАНУ ГРАВЦЯ ---
    Shader lightShader = { 0 };
    int playerPosLoc = -1;
    int resolutionLoc = -1;
    int radiusLoc = -1;
    int flickerLoc = -1;
    int ambientLoc = -1;
    int dangerLoc = -1;
    int exhaustionLoc = -1;
    int panicLoc = -1;
    int timeLoc = -1;

    float lightRadius = 280.0f;
    float flickerTimer = 0.0f;
    float currentFlicker = 1.0f;
    float smoothedNoise = 0.0f;
    float smoothedDanger = 0.0f;
    float smoothedExhaustion = 0.0f;
    float smoothedPanic = 0.0f;
    float damageImpulse = 0.0f;
    float shaderTime = 0.0f;
    float lightDipTimer = 0.0f;
    float lightDipDuration = 0.14f;
    float lightDipDepth = 0.0f;

    // Мікро-інерція джерела світла
    Vector2 lightWorldPos = { 0.0f, 0.0f };
    Vector2 lightVelocity = { 0.0f, 0.0f };
    bool isLightPosInitialized = false;

    // --- ПАУЗА ---
    bool isPaused = false;
    float pauseAnimTime = 0.0f;
    Sound pauseHoverSound = { 0 };
    bool hasPauseHoverSound = false;
    PauseButton resumeButton;
    PauseButton exitToMenuButton;
    void UpdatePauseMenu(float dt);
    void DrawPauseMenu();
    void DrawPauseButton(const PauseButton& btn);

    // --- СМЕРТЬ ТА ПЕРЕХІД В GAME OVER ---
    bool isDying = false;
    float deathFadeAlpha = 0.0f;
    float deathFadeSpeed = 1.2f;

    bool showDebugOverlay = false;

    enum class TeleportPhase {
        NONE,
        FADING_OUT,
        FADING_IN
    };

    TeleportPhase tpPhase = TeleportPhase::NONE;
    float tpFadeAlpha = 0.0f;
    float tpFadeSpeed = 2.8f;
    Vector2 pendingTeleportPos = { 0.0f, 0.0f };
    bool waitForKeysRelease = false;
    bool wasInTrigger = false;
};