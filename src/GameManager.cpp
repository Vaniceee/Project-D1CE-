#include "GameManager.h"
#include "GameplayState.h"
#include "raylib.h"

GameManager::GameManager() {}

GameManager::~GameManager() {
    Shutdown();
}

void GameManager::Init() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(screenWidth, screenHeight, "Stealth Horror Game");
    InitAudioDevice();
    SetTargetFPS(300);

    // Запускаємо стартовий геймплейний стан
    ChangeState(new GameplayState());
}

void GameManager::ChangeState(GameState* newState) {
    if (currentState) {
        currentState->Cleanup();
        delete currentState;
    }
    currentState = newState;
    if (currentState) {
        currentState->Init();
    }
}

void GameManager::SetResolution(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    SetWindowSize(screenWidth, screenHeight);
}

void GameManager::Run() {
    while (!WindowShouldClose()) {
        if (currentState) {
            currentState->Update();
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (currentState) {
            currentState->Draw();
        }

        EndDrawing();
    }
}

void GameManager::Shutdown() {
    if (currentState) {
        currentState->Cleanup();
        delete currentState;
        currentState = nullptr;
    }
    CloseAudioDevice();
    if (IsWindowReady()) {
        CloseWindow();
    }
    
}