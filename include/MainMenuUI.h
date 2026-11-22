#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>

struct MenuButton {
    Rectangle bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::string text;
    bool isHovered = false;
    bool wasHovered = false;
    float hoverProgress = 0.0f;
    bool isDanger = false; 
};

class MainMenuUI {
public:
    void DrawNeonTitle(float screenW, float screenH, float shaderTime, Vector2 smoothedMouseOffset) const;
    void DrawButton(const MenuButton& btn, float shaderTime) const;
    void DrawCctvOverlay(float screenW, float screenH, float shaderTime, float transitionProgress, bool isTransitioning) const;
    void DrawTransitionDoors(float screenW, float screenH, float progress) const;
    void DrawRulesModal(float screenW, float screenH, float openAnim, float shaderTime, const MenuButton& closeBtn) const;
};