#pragma once

#include "GameState.h"
#include "Player.h"
#include "Guard.h"
#include "Map.h"
#include "RenderSystem.h"
#include "GameCamera.h"
#include "GameplayUI.h"
#include "AssetManager.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>
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
    GameplayUI hudUI;

    // --- ВЗАЄМОДІЯ З ОБ'ЄКТАМИ ---
    bool showInteractHint = false;

    // --- ВІДОБРАЖЕННЯ РЕЗУЛЬТАТУ КУБИКІВ ---
    int lastDice1 = 0;
    int lastDice2 = 0;
    int lastDiceSum = 0;
    float sumDisplayTimer = 0.0f;
    const float sumDisplayDuration = 1.0f;

    std::unique_ptr<Player> player;
    std::unique_ptr<Guard> guard;

    FloorTutorialHint floorHint;
    bool hasMovedOnce = false;
    bool hasSprintedOnce = false;

    Music bgMusic = { 0 };

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
    int cameraTargetLoc = -1;
    int cameraOffsetLoc = -1;
    int cameraZoomLoc = -1;

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

    Vector2 lightWorldPos = { 0.0f, 0.0f };
    Vector2 lightVelocity = { 0.0f, 0.0f };
    bool isLightPosInitialized = false;

    // --- ПУЛ ЧАСТИНОК ПИЛУ ПІД НОГАМИ ПРИ СПРИНТІ ---
    struct SprintDustParticle {
        Vector2 pos;
        Vector2 vel;
        float life;
        float maxLife;
        float size;
    };
    std::vector<SprintDustParticle> sprintDust;
    float sprintDustTimer = 0.0f;

    // --- ЕФЕКТИ ВИЯВЛЕННЯ ТА ПЕРЕМИКАННЯ КАМЕР ---
    GuardState lastGuardState = GuardState::PATROL;
    float detectionFlashTimer = 0.0f;
    float baseCameraZoom = 1.8f;
    int currentCamNumber = 1;

    // --- СТАН ПАУЗИ ТА СМЕРТІ ---
    bool isPaused = false;
    bool isDying = false;
    float deathTimer = 0.0f;
    float deathDuration = 2.35f;
    Vector2 deathFocusPos = { 0.0f, 0.0f };
    void UpdateDeathSequence(float dt);
    void SyncShaderCameraUniforms();
    void OnInteract(InteractableObject& obj);

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