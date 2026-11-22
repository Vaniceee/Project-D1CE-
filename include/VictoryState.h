#pragma once

#include "GameState.h"
#include "GameplayUI.h"
#include "raylib.h"
#include <string>

class VictoryState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    Texture2D cardTex = { 0 };
    bool hasCardTex = false;

    Sound hoverSound = { 0 };
    bool hasHoverSound = false;

    float animTime = 0.0f;
    float fadeInAlpha = 1.0f;

    std::string selectedQuote;

    UiButton playAgainButton;
    UiButton mainMenuButton;
};
