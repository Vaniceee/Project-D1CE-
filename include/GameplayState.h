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

    // --- ШЕЙДЕР ОСВІТЛЕННЯ ---
    Shader lightShader = { 0 };
    int playerPosLoc = -1;
    int resolutionLoc = -1;
    int radiusLoc = -1;
    int flickerLoc = -1;
    int ambientLoc = -1;

    float lightRadius = 260.0f;
    float flickerTimer = 0.0f;
    float currentFlicker = 1.0f;

    // Прапорці та стани переходу локації
    bool isPaused = false;
    bool isGameOverProcessed = false;
    bool showDebugOverlay = false;

    enum class TeleportPhase {
        NONE,
        FADING_OUT,
        FADING_IN
    };

    TeleportPhase tpPhase = TeleportPhase::NONE;
    float tpFadeAlpha = 0.0f;
    float tpFadeSpeed = 2.8f; // ~0.35 сек на затемнення і ~0.35 сек на проявлення
    Vector2 pendingTeleportPos = { 0.0f, 0.0f };
    bool waitForKeysRelease = false;
    bool wasInTrigger = false;
};