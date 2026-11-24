#include "CutsceneUI.h"
#include "CustomCursor.h"
#include <algorithm>
#include <cmath>

namespace {
    const Color NOIR_BG = { 10, 9, 13, 255 };
    const Color PANEL_BG = { 14, 12, 18, 240 };
    const Color GOLD_ACCENT = { 212, 175, 55, 255 };
    const Color GOLD_DIM = { 130, 106, 36, 180 };
    const Color CRIMSON_ACCENT = { 185, 38, 48, 255 };
    const Color IVORY_TEXT = { 242, 236, 224, 255 };
}

void CutsceneUI::DrawComicPage(
    const std::vector<CutsceneVisualLayer>& layers,
    const std::string& visibleSubtitle,
    bool showContinueBtn,
    const UiButton& actionButton,
    int currentPageNum,
    int totalPages,
    float animTime,
    float screenFadeAlpha)
{
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();

    ClearBackground(NOIR_BG);

    // Вписуємо віртуальне полотно 1920x1080 у доступну область над субтитрами
    const float topMargin = 28.0f;
    const float bottomReserved = 155.0f;
    const float availH = sh - topMargin - bottomReserved;
    const float availW = sw - 80.0f;

    const float designAspect = DESIGN_WIDTH / DESIGN_HEIGHT;
    const float boxAspect = availW / availH;

    float drawW = availW;
    float drawH = availH;
    if (designAspect > boxAspect) {
        drawH = availW / designAspect;
    }
    else {
        drawW = availH * designAspect;
    }

    Rectangle frameRect = {
        (sw - drawW) * 0.5f,
        topMargin + (availH - drawH) * 0.5f,
        drawW,
        drawH
    };

    // Тінь і підкладка під сторінкою коміксу
    DrawRectangleRec({ frameRect.x + 6.0f, frameRect.y + 6.0f, frameRect.width, frameRect.height }, Color{ 0, 0, 0, 160 });
    DrawRectangleRec(frameRect, Color{ 18, 16, 22, 255 });

    // Послідовне накладання шарів сторінки у координатах 1920x1080
    for (const auto& layer : layers) {
        if (layer.texture.id > 0 && layer.alpha > 0.001f) {
            DrawLayeredFrame(layer, frameRect);
        }
    }

    // Подвійна Art Deco рамка навколо коміксу
    DrawRectangleLinesEx(frameRect, 2.0f, GOLD_ACCENT);
    DrawRectangleLinesEx({ frameRect.x - 4.0f, frameRect.y - 4.0f, frameRect.width + 8.0f, frameRect.height + 8.0f }, 1.0f, GOLD_DIM);

    // Плашка субтитрів з ефектом друкарської машинки
    DrawSubtitleBox(visibleSubtitle, animTime);

    // Верхній індикатор сторінки та підказка скіпу
    const char* headerTag = TextFormat("DOSSIER // SCENE %d OF %d", currentPageNum, totalPages);
    DrawText(headerTag, (int)frameRect.x, 8, 14, GOLD_DIM);

    const char* skipHint = showContinueBtn ? "[CLICK CONTINUE OR PRESS ENTER]" : "[CLICK / SPACE] SKIP ANIMATION";
    int hintW = MeasureText(skipHint, 14);
    DrawText(skipHint, (int)(frameRect.x + frameRect.width - hintW), 8, 14, Color{ 170, 165, 155, 180 });

    // Кнопка CONTINUE
    if (showContinueBtn) {
        GameplayUI::DrawArtDecoButton(actionButton, animTime);
    }

    DrawNoirBorder(animTime);

    if (screenFadeAlpha > 0.001f) {
        DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(BLACK, screenFadeAlpha));
    }
    CustomCursor::Draw(showContinueBtn && actionButton.isHovered);
}

void CutsceneUI::DrawPlaceholderVideo(
    Texture2D gifTexture,
    float progressRatio,
    const std::string& visibleSubtitle,
    const UiButton& skipButton,
    int rangeStartPage,
    int rangeEndPage,
    float animTime,
    float screenFadeAlpha)
{
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();

    ClearBackground(NOIR_BG);

    const float topMargin = 28.0f;
    const float bottomReserved = 155.0f;
    const float availH = sh - topMargin - bottomReserved;
    const float availW = sw - 80.0f;

    Rectangle videoRect = { (sw - availW) * 0.5f, topMargin, availW, availH };
    if (gifTexture.id > 0) {
        float texAspect = (float)gifTexture.width / (float)gifTexture.height;
        float drawW = availW;
        float drawH = availH;
        if (texAspect > (availW / availH)) {
            drawH = availW / texAspect;
        }
        else {
            drawW = availH * texAspect;
        }
        videoRect = { (sw - drawW) * 0.5f, topMargin + (availH - drawH) * 0.5f, drawW, drawH };
        DrawFullViewport(gifTexture, 1.0f, videoRect);
    }

    DrawRectangleLinesEx(videoRect, 2.0f, CRIMSON_ACCENT);
    DrawRectangleLinesEx({ videoRect.x - 4.0f, videoRect.y - 4.0f, videoRect.width + 8.0f, videoRect.height + 8.0f }, 1.0f, GOLD_DIM);

    const char* headerTag = (rangeStartPage == rangeEndPage)
        ? TextFormat("DOSSIER // SCENE %d [PLACEHOLDER]", rangeStartPage)
        : TextFormat("DOSSIER // SCENES %d-%d [PLACEHOLDER]", rangeStartPage, rangeEndPage);
    DrawText(headerTag, (int)videoRect.x, 8, 14, CRIMSON_ACCENT);

    const char* skipHint = "[CLICK SKIP OR PRESS ENTER] ENTER CASINO";
    int hintW = MeasureText(skipHint, 14);
    DrawText(skipHint, (int)(videoRect.x + videoRect.width - hintW), 8, 14, Color{ 170, 165, 155, 180 });

    DrawSubtitleBox(visibleSubtitle, animTime);

    if (progressRatio > 0.001f) {
        float barY = videoRect.y + videoRect.height + 8.0f;
        DrawRectangle((int)videoRect.x, (int)barY, (int)videoRect.width, 4, Color{ 40, 35, 45, 255 });
        DrawRectangle((int)videoRect.x, (int)barY, (int)(videoRect.width * std::clamp(progressRatio, 0.0f, 1.0f)), 4, CRIMSON_ACCENT);
    }

    GameplayUI::DrawArtDecoButton(skipButton, animTime);

    DrawNoirBorder(animTime);

    if (screenFadeAlpha > 0.001f) {
        DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(BLACK, screenFadeAlpha));
    }

    CustomCursor::Draw(skipButton.isHovered);
}

void CutsceneUI::DrawLayeredFrame(const CutsceneVisualLayer& layer, Rectangle canvasRect)
{
    const float scaleX = canvasRect.width / DESIGN_WIDTH;
    const float scaleY = canvasRect.height / DESIGN_HEIGHT;

    Rectangle src = { 0.0f, 0.0f, (float)layer.texture.width, (float)layer.texture.height };
    Rectangle dst = {
        canvasRect.x + layer.designPos.x * scaleX,
        canvasRect.y + layer.designPos.y * scaleY,
        (float)layer.texture.width * scaleX,
        (float)layer.texture.height * scaleY
    };

    DrawTexturePro(layer.texture, src, dst, { 0.0f, 0.0f }, 0.0f, ColorAlpha(WHITE, layer.alpha));
}

void CutsceneUI::DrawFullViewport(Texture2D tex, float alpha, Rectangle targetRect)
{
    Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    DrawTexturePro(tex, src, targetRect, { 0.0f, 0.0f }, 0.0f, ColorAlpha(WHITE, alpha));
}

void CutsceneUI::DrawSubtitleBox(const std::string& text, float animTime)
{
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();

    const float boxW = sw - 260.0f;
    const float boxH = 68.0f;
    const float boxX = 40.0f;
    const float boxY = sh - boxH - 28.0f;

    Rectangle box = { boxX, boxY, boxW, boxH };

    DrawRectangleRec({ box.x + 4.0f, box.y + 4.0f, box.width, box.height }, Color{ 0, 0, 0, 170 });
    DrawRectangleRec(box, PANEL_BG);
    DrawRectangleLinesEx(box, 1.5f, GOLD_ACCENT);
    DrawRectangleLinesEx({ box.x + 3.0f, box.y + 3.0f, box.width - 6.0f, box.height - 6.0f }, 1.0f, GOLD_DIM);

    float midY = box.y + box.height * 0.5f;
    DrawPoly({ box.x + 14.0f, midY }, 4, 4.5f, 0.0f, GOLD_ACCENT);

    int fontSize = 20;
    float textX = box.x + 30.0f;
    float textY = box.y + (box.height - fontSize) * 0.5f;

    DrawText(text.c_str(), (int)textX + 2, (int)textY + 2, fontSize, BLACK);
    DrawText(text.c_str(), (int)textX, (int)textY, fontSize, IVORY_TEXT);

    if (((int)(animTime * 3.0f) % 2) == 0 && !text.empty()) {
        int w = MeasureText(text.c_str(), fontSize);
        DrawRectangle((int)(textX + w + 5.0f), (int)textY + 2, 8, fontSize - 4, GOLD_ACCENT);
    }
}

void CutsceneUI::DrawNoirBorder(float animTime)
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    for (int y = 0; y < sh; y += 4) {
        DrawRectangle(0, y, sw, 1, Color{ 0, 0, 0, 22 });
    }

    const int len = 18;
    const Color c = ColorAlpha(GOLD_ACCENT, 0.55f + 0.15f * std::sin(animTime * 2.0f));
    DrawRectangle(10, 10, len, 2, c);
    DrawRectangle(10, 10, 2, len, c);

    DrawRectangle(sw - 10 - len, 10, len, 2, c);
    DrawRectangle(sw - 10, 10, 2, len, c);

    DrawRectangle(10, sh - 10, len, 2, c);
    DrawRectangle(10, sh - 10 - len, 2, len, c);

    DrawRectangle(sw - 10 - len, sh - 10, len, 2, c);
    DrawRectangle(sw - 10, sh - 10 - len, 2, len, c);
}