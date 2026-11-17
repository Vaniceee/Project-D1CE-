#include "GameManager.h"
#include "GameplayState.h"
#include "raylib.h"
#include "MainMenuState.h"


GameManager& GameManager::Instance() {
    static GameManager instance;
    return instance;
}

GameManager::GameManager() {}

GameManager::~GameManager() {
    Shutdown();
}

void GameManager::Init() {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "Stealth Horror Game");
    SetExitKey(KEY_NULL);
    
    SetTargetFPS(300);
    isRunning = true;

    ChangeState(new MainMenuState());
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
    while (isRunning && !WindowShouldClose()) {
        if (currentState) {
            currentState->Update();
        }

        if (!isRunning || WindowShouldClose()) {
            break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (currentState) {
            currentState->Draw();
        }

        EndDrawing();
    }
    Shutdown();
}

void GameManager::Shutdown() {

    static bool isShutdown = false;
    if (isShutdown) return;


    if (currentState) {
        currentState->Cleanup();
        delete currentState;
        currentState = nullptr;
    }

    AssetManager::Instance().UnloadAll();

    CloseAudioDevice();
    if (IsWindowReady()) {
        CloseWindow();
    }

    isShutdown = true;
}