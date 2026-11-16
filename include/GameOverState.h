#pragma once

#include "GameState.h"
#include "raylib.h"
#include <string>

struct GameOverButton {
    Rectangle bounds;
    std::string text;
    bool isHovered = false;
    bool wasHovered = false;
    float hoverProgress = 0.0f;
    bool isPrimary = false; // true ‰Îˇ TRY AGAIN (ÁÓÎÓÚÓ), false ‰Îˇ MAIN MENU (·‡„ˇÌËÈ)
};

class GameOverState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    // --- –≈—”–—» ---
    Texture2D bgTexture = { 0 };
    Texture2D tornCardTexture = { 0 };
    Sound gameOverSound = { 0 };
    Sound hoverSound = { 0 };
    Music gameOverMusic = { 0 };

    bool hasBackground = false;
    bool hasTornCard = false;
    bool hasSound = false;
    bool hasHoverSound = false;
    bool hasMusic = false;

    // --- ÿ≈…ƒ≈– ---
    Shader gameOverShader = { 0 };
    int resLoc = -1;
    int timeLoc = -1;
    int fadeInLoc = -1;
    int hasBgLoc = -1;

    // --- ¿Õ≤Ã¿÷≤ﬂ —÷≈Õ» GAME OVER ---
    float sceneTimer = 0.0f;
    float fadeInDuration = 1.2f;
    float buttonAppearDelay = 1.0f;

    // --- œ≈–≈’≤ƒ ¬ ≤Õÿ»… —“¿Õ ---
    enum class ExitTarget {
        NONE,
        MAIN_MENU,
        RETRY_GAME
    };

    ExitTarget exitTarget = ExitTarget::NONE;
    float exitFadeTimer = 0.0f;
    float exitFadeDuration = 0.85f;

    GameOverButton retryButton;
    GameOverButton menuButton;

    void DrawTornAceCard(float screenW, float screenH, float fadeProgress);
    void DrawNeonGameOverTitle(float screenW, float screenH, float fadeProgress);
    void DrawButton(const GameOverButton& btn, float alpha);
    void DrawNoSignalHud(float screenW, float screenH, float alpha);
};