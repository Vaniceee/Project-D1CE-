#include "GameOverUI.h"
#include "CustomCursor.h"
#include <cmath>

namespace {
    void DrawSoftGlow(Vector2 center, float radius, Color color) {
        const int rings = 14;
        for (int i = rings; i >= 1; --i) {
            float t = (float)i / (float)rings;
            float r = radius * t;
            float falloff = (1.0f - t) * (1.0f - t) + 0.12f;
            float alphaFactor = (falloff / (float)rings) * 2.1f;
            DrawCircleV(center, r, ColorAlpha(color, alphaFactor));
        }
    }
}

void GameOverUI::Init() {
    InitSpotlights();
    InitEmbers();
}

void GameOverUI::Update(float dt, float animTime, Vector2 mousePos) {
    UpdateSpotlights(dt, animTime, mousePos);
    UpdateEmbers(dt, animTime);
}

void GameOverUI::InitSpotlights() {
    spotlights.clear();

    // 1. Лівий верхній кружок
    CrimsonSpotlight s1;
    s1.anchorNorm = { 0.24f, 0.26f };
    s1.driftAmp = { 130.0f, 85.0f };
    s1.driftFreq = { 0.31f, 0.24f };
    s1.radiusFactor = 0.27f;
    s1.color = Color{ 165, 18, 30, 255 };
    s1.baseAlpha = 0.30f;
    s1.pulseScale = 0.12f;
    spotlights.push_back(s1);

    // 2. Правий центрально-верхній кружок
    CrimsonSpotlight s2;
    s2.anchorNorm = { 0.76f, 0.38f };
    s2.driftAmp = { 120.0f, 95.0f };
    s2.driftFreq = { 0.23f, 0.34f };
    s2.radiusFactor = 0.25f;
    s2.color = Color{ 140, 14, 25, 255 };
    s2.baseAlpha = 0.28f;
    s2.pulseScale = 0.11f;
    spotlights.push_back(s2);

    // 3. Нижній центрально-лівий кружок
    CrimsonSpotlight s3;
    s3.anchorNorm = { 0.48f, 0.74f };
    s3.driftAmp = { 145.0f, 70.0f };
    s3.driftFreq = { 0.27f, 0.21f };
    s3.radiusFactor = 0.29f;
    s3.color = Color{ 195, 22, 34, 255 };
    s3.baseAlpha = 0.26f;
    s3.pulseScale = 0.14f;
    spotlights.push_back(s3);
}

void GameOverUI::UpdateSpotlights(float dt, float animTime, Vector2 mousePos) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    const float cursorRepelRadius = 310.0f;
    const float cursorRepelStrength = 2800.0f;
    const float pairMinDist = 340.0f;
    const float pairRepelStrength = 950.0f;
    const float springK = 4.5f;
    const float damping = 3.8f;
    const float maxOffset = 280.0f;

    std::vector<Vector2> basePositions(spotlights.size());
    for (size_t i = 0; i < spotlights.size(); ++i) {
        auto& s = spotlights[i];
        float phaseX = (float)i * 2.35f;
        float phaseY = (float)i * 1.47f + 0.8f;

        basePositions[i] = {
            sw * s.anchorNorm.x + sinf(animTime * s.driftFreq.x + phaseX) * s.driftAmp.x,
            sh * s.anchorNorm.y + cosf(animTime * s.driftFreq.y + phaseY) * s.driftAmp.y
        };
        s.pos = Vector2Add(basePositions[i], s.repelOffset);
    }

    for (size_t i = 0; i < spotlights.size(); ++i) {
        auto& s = spotlights[i];
        Vector2 accel = { 0.0f, 0.0f };

        Vector2 awayFromMouse = Vector2Subtract(s.pos, mousePos);
        float distMouse = Vector2Length(awayFromMouse);
        if (distMouse < cursorRepelRadius) {
            Vector2 dir = (distMouse > 0.001f)
                ? Vector2Scale(awayFromMouse, 1.0f / distMouse)
                : Vector2{ cosf((float)i * 2.1f), sinf((float)i * 2.1f) };

            float falloff = 1.0f - (distMouse / cursorRepelRadius);
            accel = Vector2Add(accel, Vector2Scale(dir, cursorRepelStrength * falloff * falloff));
        }

        for (size_t j = 0; j < spotlights.size(); ++j) {
            if (i == j) continue;
            Vector2 awayFromOther = Vector2Subtract(s.pos, spotlights[j].pos);
            float distOther = Vector2Length(awayFromOther);
            if (distOther < pairMinDist && distOther > 0.001f) {
                Vector2 dir = Vector2Scale(awayFromOther, 1.0f / distOther);
                float overlap = 1.0f - (distOther / pairMinDist);
                accel = Vector2Add(accel, Vector2Scale(dir, pairRepelStrength * overlap));
            }
        }

        accel = Vector2Subtract(accel, Vector2Scale(s.repelOffset, springK));
        accel = Vector2Subtract(accel, Vector2Scale(s.repelVel, damping));

        s.repelVel = Vector2Add(s.repelVel, Vector2Scale(accel, dt));
        s.repelOffset = Vector2Add(s.repelOffset, Vector2Scale(s.repelVel, dt));

        float offsetLen = Vector2Length(s.repelOffset);
        if (offsetLen > maxOffset) {
            s.repelOffset = Vector2Scale(s.repelOffset, maxOffset / offsetLen);
        }

        s.pos = Vector2Add(basePositions[i], s.repelOffset);
    }
}

void GameOverUI::InitEmbers() {
    embers.clear();
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    const int emberCount = 46;
    embers.reserve(emberCount);

    for (int i = 0; i < emberCount; ++i) {
        GameOverEmber e;
        e.pos = {
            (float)GetRandomValue(0, (int)sw),
            (float)GetRandomValue(0, (int)sh)
        };
        e.speedY = (float)GetRandomValue(16, 48);
        e.driftFreq = (float)GetRandomValue(8, 22) / 10.0f;
        e.driftAmp = (float)GetRandomValue(12, 36);
        e.phase = (float)GetRandomValue(0, 628) / 100.0f;
        e.size = (float)GetRandomValue(15, 38) / 10.0f;
        e.alpha = (float)GetRandomValue(22, 65) / 100.0f;
        e.isGold = (GetRandomValue(0, 100) < 22);
        embers.push_back(e);
    }
}

void GameOverUI::UpdateEmbers(float dt, float animTime) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    for (auto& e : embers) {
        e.pos.y -= e.speedY * dt;
        e.pos.x += cosf(animTime * e.driftFreq + e.phase) * e.driftAmp * dt;

        if (e.pos.y < -12.0f) {
            e.pos.y = sh + 12.0f;
            e.pos.x = (float)GetRandomValue(0, (int)sw);
        }
        if (e.pos.x < -12.0f) e.pos.x = sw + 12.0f;
        if (e.pos.x > sw + 12.0f) e.pos.x = -12.0f;
    }
}

void GameOverUI::DrawAnimatedCrimsonBackdrop(float animTime) const {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    // Базовий глибокий багряно-чорний градієнт
    DrawRectangleGradientV(0, 0, (int)sw, (int)sh, Color{ 32, 7, 11, 255 }, Color{ 9, 4, 6, 255 });

    // Пульсуючий ритм згасаючого серцебиття
    float beatCycle = fmodf(animTime, 1.9f);
    float beat1 = expf(-powf((beatCycle - 0.18f) * 9.0f, 2.0f));
    float beat2 = expf(-powf((beatCycle - 0.42f) * 11.0f, 2.0f)) * 0.65f;
    float heartPulse = Clamp(beat1 + beat2, 0.0f, 1.0f);

    // 1. Три інтерактивні багряні кружки
    for (const auto& s : spotlights) {
        DrawSoftGlow(s.pos, sw * s.radiusFactor, ColorAlpha(s.color, s.baseAlpha + s.pulseScale * heartPulse));
    }

    // 2. Великі фонові ромби Ар-Деко
    Vector2 bgDiamondCenter = { sw * 0.5f, sh * 0.31f };
    float rotSlow = sinf(animTime * 0.35f) * 4.0f;
    DrawPolyLinesEx(bgDiamondCenter, 4, 210.0f + heartPulse * 8.0f, rotSlow, 1.5f, ColorAlpha(Color{ 225, 48, 56, 255 }, 0.10f + 0.06f * heartPulse));
    DrawPolyLinesEx(bgDiamondCenter, 4, 255.0f, -rotSlow * 0.7f, 1.0f, ColorAlpha(GOLD, 0.06f));

    // 3. Попіл та багряно-золоті іскри
    for (const auto& e : embers) {
        float flicker = 0.65f + 0.35f * sinf(animTime * 4.2f + e.phase);
        Color col = e.isGold
            ? ColorAlpha(GOLD, e.alpha * flicker * 0.75f)
            : ColorAlpha(Color{ 245, 68, 76, 255 }, e.alpha * flicker);

        DrawCircleV(e.pos, e.size * 2.2f, ColorAlpha(col, 0.18f));
        DrawRectangle((int)(e.pos.x - e.size * 0.5f), (int)(e.pos.y - e.size * 0.5f), (int)ceilf(e.size), (int)ceilf(e.size), col);
    }

    // 4. Глибока віньєтка по краях екрана
    int vigDepth = (int)(sh * 0.24f);
    DrawRectangleGradientV(0, 0, (int)sw, vigDepth, Fade(BLACK, 0.75f), BLANK);
    DrawRectangleGradientV(0, (int)sh - vigDepth, (int)sw, vigDepth, BLANK, Fade(BLACK, 0.85f));

    int sideVig = (int)(sw * 0.18f);
    DrawRectangleGradientH(0, 0, sideVig, (int)sh, Fade(BLACK, 0.68f), BLANK);
    DrawRectangleGradientH((int)sw - sideVig, 0, sideVig, (int)sh, BLANK, Fade(BLACK, 0.68f));

    // 5. Сканлайни та рідкісні перешкоди сигналу CCTV
    for (int y = 0; y < (int)sh; y += 4) {
        DrawRectangle(0, y, (int)sw, 1, Fade(WHITE, 0.024f));
    }

    if (fmodf(animTime, 4.1f) > 3.92f) {
        for (int i = 0; i < 4; ++i) {
            int barY = GetRandomValue(0, (int)sh);
            int barH = GetRandomValue(2, 6);
            DrawRectangle(0, barY, (int)sw, barH, ColorAlpha(Color{ 235, 52, 60, 255 }, 0.16f));
        }
    }
}

void GameOverUI::Draw(
    float animTime,
    float fadeInAlpha,
    Texture2D cardTex,
    bool hasCardTex,
    const std::string& selectedQuote,
    const UiButton& tryAgainButton,
    const UiButton& mainMenuButton
) const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // 1. Живий анімований багряний фон
    DrawAnimatedCrimsonBackdrop(animTime);

    // 2. Кутові візири CCTV
    float margin = 26.0f;
    float arm = 28.0f;
    Color hudCol = ColorAlpha(Color{ 225, 48, 56, 255 }, 0.42f);
    DrawLineEx({ margin, margin }, { margin + arm, margin }, 1.5f, hudCol);
    DrawLineEx({ margin, margin }, { margin, margin + arm }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin - arm, margin }, { screenW - margin, margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + arm }, 1.5f, hudCol);
    DrawLineEx({ margin, screenH - margin }, { margin + arm, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ margin, screenH - margin - arm }, { margin, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin - arm, screenH - margin }, { screenW - margin, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin, screenH - margin - arm }, { screenW - margin, screenH - margin }, 1.5f, hudCol);

    if (fmodf(animTime, 1.3f) < 0.8f) {
        DrawCircle((int)(margin + 16.0f), (int)(margin + 18.0f), 4.5f, ColorAlpha(RED, 0.88f));
    }
    DrawText("SIGNAL LOST // SUBJECT ELIMINATED", (int)(margin + 30.0f), (int)(margin + 11.0f), 14, ColorAlpha(LIGHTGRAY, 0.65f));

    // 3. Розірваний Туз у центрі композиції
    Vector2 cardCenter = { screenW * 0.5f, screenH * 0.23f };
    float pulse = 0.65f + 0.35f * sinf(animTime * 3.5f);

    DrawCircleV(cardCenter, 82.0f, ColorAlpha(MAROON, 0.28f * pulse));
    DrawPolyLinesEx(cardCenter, 4, 76.0f, 0.0f, 1.5f, ColorAlpha(RED, 0.42f));

    if (hasCardTex) {
        const float frameW = 32.0f;
        const float frameH = 32.0f;
        Rectangle srcRec = { 3.0f * frameW, 0.0f, frameW, frameH };
        float cardScale = 4.5f;
        float floatY = sinf(animTime * 2.1f) * 3.2f;
        float tilt = sinf(animTime * 1.4f) * 3.2f;

        Rectangle dstRec = {
            cardCenter.x,
            cardCenter.y + floatY,
            frameW * cardScale,
            frameH * cardScale
        };
        DrawTexturePro(cardTex, srcRec, dstRec, { dstRec.width * 0.5f, dstRec.height * 0.5f }, tilt, WHITE);
    }

    // 4. Багряний неоновий заголовок GAME OVER
    const char* titleText = "GAME OVER";
    int titleSize = 62;
    int titleW = MeasureText(titleText, titleSize);
    int titleX = (int)((screenW - titleW) * 0.5f);
    int titleY = (int)(screenH * 0.38f);

    float glitchX = (fmodf(animTime, 3.8f) > 3.65f) ? (float)GetRandomValue(-3, 3) : 0.0f;

    DrawText(titleText, (int)(titleX - 2 + glitchX), titleY + 2, titleSize, ColorAlpha(MAROON, 0.62f * pulse));
    DrawText(titleText, (int)(titleX + 2 + glitchX), titleY - 1, titleSize, ColorAlpha(GOLD, 0.24f * pulse));
    DrawText(titleText, (int)(titleX + glitchX), titleY, titleSize, Color{ 242, 56, 64, 255 });

    // Філігранний роздільник Ар-Деко (--- ? ---)
    float lineY = (float)(titleY + titleSize + 14);
    float centerX = screenW * 0.5f;
    DrawLineEx({ centerX - 180.0f, lineY }, { centerX - 14.0f, lineY }, 1.5f, ColorAlpha(GOLD, 0.48f));
    DrawLineEx({ centerX + 14.0f, lineY }, { centerX + 180.0f, lineY }, 1.5f, ColorAlpha(GOLD, 0.48f));
    DrawPoly({ centerX, lineY }, 4, 5.5f, 0.0f, GOLD);

    // 5. Випадкова фраза в стилі казино
    int quoteSize = 18;
    int quoteW = MeasureText(selectedQuote.c_str(), quoteSize);
    DrawText(
        selectedQuote.c_str(),
        (int)((screenW - quoteW) * 0.5f),
        (int)(lineY + 18.0f),
        quoteSize,
        ColorAlpha(Color{ 238, 214, 150, 255 }, 0.90f)
    );

    // 6. Кнопки TRY AGAIN та MAIN MENU
    GameplayUI::DrawArtDecoButton(tryAgainButton, animTime);
    GameplayUI::DrawArtDecoButton(mainMenuButton, animTime);

    CustomCursor::Draw(mainMenuButton.isHovered);

    if (fadeInAlpha > 0.0f) {
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, fadeInAlpha));
    }
}