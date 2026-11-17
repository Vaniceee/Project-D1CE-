#pragma once

#include "GameplayUI.h"
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

struct GameOverEmber {
    Vector2 pos = { 0.0f, 0.0f };
    float speedY = 0.0f;
    float driftFreq = 0.0f;
    float driftAmp = 0.0f;
    float phase = 0.0f;
    float size = 0.0f;
    float alpha = 0.0f;
    bool isGold = false;
};

struct CrimsonSpotlight {
    Vector2 anchorNorm = { 0.0f, 0.0f };
    Vector2 driftAmp = { 0.0f, 0.0f };
    Vector2 driftFreq = { 0.0f, 0.0f };
    float radiusFactor = 0.25f;
    Color color = MAROON;
    float baseAlpha = 0.28f;
    float pulseScale = 0.12f;

    Vector2 pos = { 0.0f, 0.0f };
    Vector2 repelOffset = { 0.0f, 0.0f };
    Vector2 repelVel = { 0.0f, 0.0f };
};

class GameOverUI {
public:
    void Init();
    void Update(float dt, float animTime, Vector2 mousePos);
    void Draw(
        float animTime,
        float fadeInAlpha,
        Texture2D cardTex,
        bool hasCardTex,
        const std::string& selectedQuote,
        const UiButton& tryAgainButton,
        const UiButton& mainMenuButton
    ) const;

private:
    std::vector<GameOverEmber> embers;
    std::vector<CrimsonSpotlight> spotlights;

    void InitSpotlights();
    void UpdateSpotlights(float dt, float animTime, Vector2 mousePos);
    void InitEmbers();
    void UpdateEmbers(float dt, float animTime);
    void DrawAnimatedCrimsonBackdrop(float animTime) const;
};
