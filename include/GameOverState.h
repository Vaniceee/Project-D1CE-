#pragma once

#include "GameState.h"
#include "GameOverUI.h"
#include "GameplayUI.h"
#include "AssetManager.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include <string>

class GameOverState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    GameOverUI gameOverUI;

    Texture2D cardTex = { 0 };
    bool hasCardTex = false;

    Music gameOverMusic = { 0 };
    bool hasMusic = false;

    Sound hoverSound = { 0 };
    bool hasHoverSound = false;

    float animTime = 0.0f;
    float fadeInAlpha = 1.0f;

    std::string selectedQuote;

    UiButton tryAgainButton;
    UiButton mainMenuButton;
};