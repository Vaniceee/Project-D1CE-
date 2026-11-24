#pragma once
#include "raylib.h"
#include "GameplayUI.h"
#include <string>
#include <vector>

struct CutsceneVisualLayer {
    Texture2D texture{};
    Vector2 designPos = { 0.0f, 0.0f }; // Координати на базовому полотні 1920x1080
    float alpha = 0.0f;
};

class CutsceneUI {
public:
    static constexpr float DESIGN_WIDTH = 1920.0f;
    static constexpr float DESIGN_HEIGHT = 1080.0f;

    static void DrawComicPage(
        const std::vector<CutsceneVisualLayer>& layers,
        const std::string& visibleSubtitle,
        bool showContinueBtn,
        const UiButton& actionButton,
        int currentPageNum,
        int totalPages,
        float animTime,
        float screenFadeAlpha
    );

    static void DrawPlaceholderVideo(
        Texture2D gifTexture,
        float progressRatio,
        const std::string& visibleSubtitle,
        const UiButton& skipButton,
        int rangeStartPage,
        int rangeEndPage,
        float animTime,
        float screenFadeAlpha
    );

private:
    static void DrawLayeredFrame(const CutsceneVisualLayer& layer, Rectangle canvasRect);
    static void DrawFullViewport(Texture2D tex, float alpha, Rectangle targetRect);
    static void DrawSubtitleBox(const std::string& text, float animTime);
    static void DrawNoirBorder(float animTime);
};