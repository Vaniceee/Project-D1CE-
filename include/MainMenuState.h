#pragma once

#include "GameState.h"
#include "raylib.h"
#include "AssetManager.h"
#include "config.h"
#include <string>

struct MenuButton {
    Rectangle bounds;
    std::string text;
    bool isHovered = false;
    bool wasHovered = false;
    float hoverProgress = 0.0f;
    bool isDanger = false; // true äëÿ EXIT (áàãğÿíèé ñòèëü ³ç òğåìò³ííÿì)
};

class MainMenuState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    Texture2D bgTexture = { 0 };
    Music bgMusic = { 0 };
    bool hasBackground = false;
    bool hasMusic = false;

    // --- ØÅÉÄÅĞ ÌÅÍŞ ÒÀ ÏÀĞÀËÀÊÑ ---
    Shader menuShader = { 0 };
    int menuResLoc = -1;
    int menuTimeLoc = -1;
    int menuTransitionLoc = -1;
    int menuMouseLoc = -1;
    float shaderTime = 0.0f;
    Vector2 smoothedMouseOffset = { 0.0f, 0.0f };

    // --- ÏÅĞÅÕ²Ä ² ÇÂÓÊ ---
    bool isTransitioning = false;
    float transitionTimer = 0.0f;
    const float transitionDuration = 4.0f;

    Sound startSound = { 0 };
    bool hasStartSound = false;

    Sound hoverSound = { 0 };
    bool hasHoverSound = false;

    // Êíîïêè
    MenuButton playButton;
    MenuButton exitButton;

    void DrawNeonTitle(float screenW, float screenH);
    void DrawButton(const MenuButton& btn);
    void DrawCctvOverlay(float screenW, float screenH, float transitionProgress);
};