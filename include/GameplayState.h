#pragma once

#include "GameState.h"
#include "Player.h"
#include "raylib.h"
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

    Camera2D camera = { 0 };

    const int virtualWidth = 1280;
    const int virtualHeight = 720;

    RenderTexture2D targetTexture = { 0 };

    // Прапорець для майбутньої паузи (зараз ні на що не впливає)
    bool isPaused = false;
};