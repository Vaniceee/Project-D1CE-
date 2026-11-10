#pragma once

#include "GameState.h"
#include "Player.h"
#include "Guard.h"
#include "raylib.h"
#include "raymath.h"
#include "AssetManager.h"
#include "config.h"
#include "Map.h"


class GameplayState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    GameMap gameMap;
    Player* player = nullptr;
    Guard* guard = nullptr;

    Music bgMusic = { 0 };
    Camera2D camera = { 0 };
    float cameraLerpSpeed = 7.0f;
    Vector2 cameraCurrentTarget = { 0.0f, 0.0f };


    // --- UI КАРТИ ЖИТТІВ ---
    int lastPlayerLives = 3;         
    float cardFlickerTimer = 0.0f;   
    float cardFlickerDuration = 0.45f; 


    // --- ПАРАМЕТРИ ПРОЦЕДУРНОГО ПОХИТУВАННЯ (BOBBING / BREATHING) ---
    float breathTimer = 0.0f;
    float currentBreathSpeed = 2.0f;
    float currentAmpX = 1.0f;
    float currentAmpY = 1.5f;
    
    //--------STRESS(TRAUMA SHAKE)----------
    float trauma = 0.0f;
    float maxShakeOffset = 6.0f;

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

    // Прапорці
    bool isPaused = false;
    bool isGameOverProcessed = false;
};