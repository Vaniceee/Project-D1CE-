#pragma once

#include "GameplayUI.h"
#include "raylib.h"
#include "raymath.h"
#include <string>
#include <vector>

enum class VictoryAction {
    NONE,
    PLAY_AGAIN,
    MAIN_MENU
};

class VictoryUI {
public:
    void Init();
    void Cleanup();

    VictoryAction Update(float dt);
    void Draw() const;

private:
    struct GoldenParticle {
        Vector2 pos;
        Vector2 vel;
        float size;
        float alpha;
        float phase;
        bool isEmerald;
    };

    struct CasinoSpotlight {
        Vector2 basePos;
        Vector2 currentPos;
        float radius;
        float speedX;
        float speedY;
        float phase;
        float mouseInfluence;
        bool isGold;
    };

    float animTime = 0.0f;
    float entranceAlpha = 0.0f;
    std::string chosenQuote;
    std::vector<GoldenParticle> particles;
    std::vector<CasinoSpotlight> spotlights;

    UiButton playAgainButton;
    UiButton mainMenuButton;

    Sound hoverSound = { 0 };
    bool hasHoverSound = false;
    Sound victoryChimeSound = { 0 };
    bool hasVictoryChimeSound = false;

    void InitAudio();
    void InitSceneElements();
    void DrawAnimatedLuxuryBackdrop() const;
};
