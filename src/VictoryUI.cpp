#include "VictoryUI.h"
#include "CustomCursor.h"
#include <cmath>

void VictoryUI::Init() {
    animTime = 0.0f;
    entranceAlpha = 0.0f;

    const char* quotes[] = {
        "\"You beat the house at its own rigged game.\"",
        "\"The vault stands open. Fortune favors the bold.\"",
        "\"Midnight chimes, and you walk away with the winning hand.\"",
        "\"Even the sharpest eyes in the casino couldn't keep you locked in.\""
    };
    int quoteCount = (int)(sizeof(quotes) / sizeof(quotes[0]));
    chosenQuote = quotes[GetRandomValue(0, quoteCount - 1)];

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float btnWidth = 310.0f;
    float btnHeight = 58.0f;
    float btnX = (screenW - btnWidth) * 0.5f;

    playAgainButton.bounds = { btnX, screenH * 0.59f, btnWidth, btnHeight };
    playAgainButton.text = "PLAY AGAIN";
    playAgainButton.isHovered = false;
    playAgainButton.wasHovered = false;
    playAgainButton.hoverProgress = 0.0f;
    playAgainButton.isDanger = false;

    mainMenuButton.bounds = { btnX, screenH * 0.59f + 78.0f, btnWidth, btnHeight };
    mainMenuButton.text = "MAIN MENU";
    mainMenuButton.isHovered = false;
    mainMenuButton.wasHovered = false;
    mainMenuButton.hoverProgress = 0.0f;
    mainMenuButton.isDanger = false;

    InitAudio();
    InitSceneElements();

    if (hasVictoryChimeSound) {
        PlaySound(victoryChimeSound);
    }
}

void VictoryUI::Cleanup() {
    if (hasHoverSound) {
        UnloadSound(hoverSound);
        hoverSound = { 0 };
        hasHoverSound = false;
    }
    if (hasVictoryChimeSound) {
        UnloadSound(victoryChimeSound);
        victoryChimeSound = { 0 };
        hasVictoryChimeSound = false;
    }
    particles.clear();
    spotlights.clear();
}

void VictoryUI::InitAudio() {
    // 1. Тактильне золоте клацання при наведенні на кнопки
    Wave clickWave = { 0 };
    clickWave.sampleRate = 44100;
    clickWave.sampleSize = 16;
    clickWave.channels = 1;
    clickWave.frameCount = 1100;
    short* samples = (short*)RL_MALLOC(clickWave.frameCount * sizeof(short));
    if (samples != nullptr) {
        for (unsigned int i = 0; i < clickWave.frameCount; ++i) {
            float t = (float)i / (float)clickWave.sampleRate;
            float env = expf(-t * 140.0f);
            float sig = sinf(2.0f * PI * 1046.5f * t) * 0.65f + sinf(2.0f * PI * 2093.0f * t) * 0.35f;
            samples[i] = (short)(sig * env * 11000.0f);
        }
        clickWave.data = samples;
        hoverSound = LoadSoundFromWave(clickWave);
        UnloadWave(clickWave);
        hasHoverSound = (hoverSound.stream.buffer != nullptr);
        if (hasHoverSound) {
            SetSoundVolume(hoverSound, 0.25f);
        }
    }

    // 2. Урочистий переможний акорд (C5 -> E5 -> G5 -> B5) при вході в екран перемоги
    Wave chimeWave = { 0 };
    chimeWave.sampleRate = 44100;
    chimeWave.sampleSize = 16;
    chimeWave.channels = 1;
    chimeWave.frameCount = (unsigned int)(44100 * 1.45f);
    short* chimeSamples = (short*)RL_MALLOC(chimeWave.frameCount * sizeof(short));
    if (chimeSamples != nullptr) {
        const float notes[4] = { 523.25f, 659.25f, 783.99f, 987.77f };
        const float delays[4] = { 0.00f, 0.09f, 0.18f, 0.28f };

        for (unsigned int i = 0; i < chimeWave.frameCount; ++i) {
            float t = (float)i / (float)chimeWave.sampleRate;
            float mix = 0.0f;

            for (int n = 0; n < 4; ++n) {
                float lt = t - delays[n];
                if (lt > 0.0f) {
                    float attack = Clamp(lt / 0.015f, 0.0f, 1.0f);
                    float decay = expf(-lt * 2.6f);
                    float env = attack * decay;
                    float tone = sinf(2.0f * PI * notes[n] * lt) * 0.65f
                        + sinf(2.0f * PI * notes[n] * 2.0f * lt) * 0.25f
                        + sinf(2.0f * PI * notes[n] * 0.5f * lt) * 0.20f;
                    mix += tone * env;
                }
            }
            chimeSamples[i] = (short)Clamp(mix * 7500.0f, -32767.0f, 32767.0f);
        }

        chimeWave.data = chimeSamples;
        victoryChimeSound = LoadSoundFromWave(chimeWave);
        UnloadWave(chimeWave);
        hasVictoryChimeSound = (victoryChimeSound.stream.buffer != nullptr);
        if (hasVictoryChimeSound) {
            SetSoundVolume(victoryChimeSound, 0.34f);
        }
    }
}

void VictoryUI::InitSceneElements() {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    spotlights.clear();
    spotlights.push_back({ { screenW * 0.22f, screenH * 0.26f }, { screenW * 0.22f, screenH * 0.26f }, 360.0f, 0.55f, 0.38f, 0.2f, 0.08f, true });
    spotlights.push_back({ { screenW * 0.78f, screenH * 0.30f }, { screenW * 0.78f, screenH * 0.30f }, 380.0f, 0.44f, 0.50f, 2.1f, -0.07f, false });
    spotlights.push_back({ { screenW * 0.50f, screenH * 0.20f }, { screenW * 0.50f, screenH * 0.20f }, 320.0f, 0.35f, 0.30f, 4.0f, 0.05f, true });
    spotlights.push_back({ { screenW * 0.30f, screenH * 0.80f }, { screenW * 0.30f, screenH * 0.80f }, 340.0f, 0.48f, 0.42f, 1.3f, 0.06f, false });
    spotlights.push_back({ { screenW * 0.72f, screenH * 0.78f }, { screenW * 0.72f, screenH * 0.78f }, 330.0f, 0.52f, 0.34f, 3.4f, -0.06f, true });

    particles.clear();
    particles.reserve(65);
    for (int i = 0; i < 65; ++i) {
        GoldenParticle p;
        p.pos = { (float)GetRandomValue(0, (int)screenW), (float)GetRandomValue(0, (int)screenH) };
        p.vel = { (float)GetRandomValue(-10, 10), (float)GetRandomValue(-28, -9) };
        p.size = (float)GetRandomValue(18, 42) / 10.0f;
        p.alpha = (float)GetRandomValue(18, 52) / 100.0f;
        p.phase = (float)GetRandomValue(0, 628) / 100.0f;
        p.isEmerald = (i % 4 == 0);
        particles.push_back(p);
    }
}

VictoryAction VictoryUI::Update(float dt) {
    animTime += dt;
    entranceAlpha = Clamp(entranceAlpha + dt * 1.45f, 0.0f, 1.0f);

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    Vector2 mousePos = GetMousePosition();
    Vector2 screenCenter = { screenW * 0.5f, screenH * 0.5f };
    Vector2 mouseOffset = Vector2Subtract(mousePos, screenCenter);

    for (auto& spot : spotlights) {
        Vector2 drift = {
            sinf(animTime * spot.speedX + spot.phase) * 75.0f,
            cosf(animTime * spot.speedY + spot.phase) * 55.0f
        };
        Vector2 target = Vector2Add(spot.basePos, drift);
        target = Vector2Add(target, Vector2Scale(mouseOffset, spot.mouseInfluence));
        spot.currentPos = Vector2Lerp(spot.currentPos, target, 1.0f - expf(-3.5f * dt));
    }

    for (auto& p : particles) {
        p.pos.x += (p.vel.x + sinf(animTime * 1.6f + p.phase) * 9.0f) * dt;
        p.pos.y += p.vel.y * dt;

        if (p.pos.y < -12.0f) {
            p.pos.y = screenH + 12.0f;
            p.pos.x = (float)GetRandomValue(0, (int)screenW);
        }
        if (p.pos.x < -12.0f) p.pos.x = screenW + 12.0f;
        if (p.pos.x > screenW + 12.0f) p.pos.x = -12.0f;
    }

    playAgainButton.wasHovered = playAgainButton.isHovered;
    mainMenuButton.wasHovered = mainMenuButton.isHovered;

    playAgainButton.isHovered = CheckCollisionPointRec(mousePos, playAgainButton.bounds);
    mainMenuButton.isHovered = CheckCollisionPointRec(mousePos, mainMenuButton.bounds);

    if (hasHoverSound) {
        if (playAgainButton.isHovered && !playAgainButton.wasHovered) {
            SetSoundPitch(hoverSound, 1.15f);
            PlaySound(hoverSound);
        }
        else if (mainMenuButton.isHovered && !mainMenuButton.wasHovered) {
            SetSoundPitch(hoverSound, 0.96f);
            PlaySound(hoverSound);
        }
    }

    float animSpeed = 1.0f - expf(-12.0f * dt);
    playAgainButton.hoverProgress = Lerp(playAgainButton.hoverProgress, playAgainButton.isHovered ? 1.0f : 0.0f, animSpeed);
    mainMenuButton.hoverProgress = Lerp(mainMenuButton.hoverProgress, mainMenuButton.isHovered ? 1.0f : 0.0f, animSpeed);

    bool anyHover = playAgainButton.isHovered || mainMenuButton.isHovered;
    CustomCursor::Update(dt, anyHover);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (playAgainButton.isHovered) {
            return VictoryAction::PLAY_AGAIN;
        }
        if (mainMenuButton.isHovered) {
            return VictoryAction::MAIN_MENU;
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        return VictoryAction::PLAY_AGAIN;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        return VictoryAction::MAIN_MENU;
    }

    return VictoryAction::NONE;
}

void VictoryUI::DrawAnimatedLuxuryBackdrop() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    Vector2 center = { screenW * 0.5f, screenH * 0.29f };

    // 1. Глибокий обсидіаново-смарагдовий фон
    DrawRectangleGradientV(
        0, 0, (int)screenW, (int)screenH,
        Color{ 7, 14, 12, 255 },
        Color{ 10, 9, 8, 255 }
    );

    // 2. Дрейфуючі золоті та смарагдові прожектори (м'яке багатошарове сяйво)
    for (const auto& spot : spotlights) {
        Color outerCol = spot.isGold
            ? Color{ 212, 165, 48, 255 }
        : Color{ 22, 130, 88, 255 };
        Color innerCol = spot.isGold
            ? Color{ 255, 224, 125, 255 }
        : Color{ 68, 210, 148, 255 };

        const int layers = 6;
        for (int i = layers; i >= 1; --i) {
            float t = (float)i / (float)layers;
            float r = spot.radius * t;
            Color c = (i <= 2) ? innerCol : outerCol;
            float a = (1.0f - t * 0.85f) * 0.032f;
            DrawCircleV(spot.currentPos, r, ColorAlpha(c, a));
        }
    }

    // 3. Промені світла Art Deco з-за заголовка
    const int rayCount = 12;
    for (int i = 0; i < rayCount; ++i) {
        float baseAngle = ((float)i / (float)rayCount) * PI * 2.0f + animTime * 0.08f;
        float spread = 0.045f;
        float len = screenW * 0.65f;

        Vector2 p1 = { center.x + cosf(baseAngle - spread) * len, center.y + sinf(baseAngle - spread) * len };
        Vector2 p2 = { center.x + cosf(baseAngle + spread) * len, center.y + sinf(baseAngle + spread) * len };

        DrawTriangle(center, p2, p1, ColorAlpha(GOLD, 0.024f));
    }

    // 4. Золоті та смарагдові іскри, що підіймаються вгору
    for (const auto& p : particles) {
        float twinkle = 0.65f + 0.35f * sinf(animTime * 2.8f + p.phase);
        Color col = p.isEmerald
            ? Color{ 85, 225, 160, 255 }
        : Color{ 255, 218, 110, 255 };

        DrawPoly(p.pos, 4, p.size * 1.4f, animTime * 35.0f + p.phase * 20.0f, ColorAlpha(col, p.alpha * twinkle));
        DrawCircleV(p.pos, p.size * 0.45f, ColorAlpha(WHITE, p.alpha * 0.6f * twinkle));
    }

    // 5. Легка текстура сканлайнів
    for (int y = 0; y < (int)screenH; y += 4) {
        DrawRectangle(0, y, (int)screenW, 1, Fade(WHITE, 0.018f));
    }
}

void VictoryUI::Draw() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float centerX = screenW * 0.5f;

    DrawAnimatedLuxuryBackdrop();

    // =========================================================================
    // 1. ВІЗИРИ КАМЕРИ ТА РАМКА ART DECO
    // =========================================================================
    float margin = 28.0f;
    float arm = 34.0f;
    Color frameGold = ColorAlpha(GOLD, 0.55f);

    DrawRectangleLinesEx({ margin, margin, screenW - margin * 2.0f, screenH - margin * 2.0f }, 1.0f, ColorAlpha(GOLD, 0.16f));

    // Кутові акценти
    DrawLineEx({ margin, margin }, { margin + arm, margin }, 2.0f, frameGold);
    DrawLineEx({ margin, margin }, { margin, margin + arm }, 2.0f, frameGold);
    DrawPoly({ margin, margin }, 4, 5.0f, 0.0f, GOLD);

    DrawLineEx({ screenW - margin - arm, margin }, { screenW - margin, margin }, 2.0f, frameGold);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + arm }, 2.0f, frameGold);
    DrawPoly({ screenW - margin, margin }, 4, 5.0f, 0.0f, GOLD);

    DrawLineEx({ margin, screenH - margin }, { margin + arm, screenH - margin }, 2.0f, frameGold);
    DrawLineEx({ margin, screenH - margin - arm }, { margin, screenH - margin }, 2.0f, frameGold);
    DrawPoly({ margin, screenH - margin }, 4, 5.0f, 0.0f, GOLD);

    DrawLineEx({ screenW - margin - arm, screenH - margin }, { screenW - margin, screenH - margin }, 2.0f, frameGold);
    DrawLineEx({ screenW - margin, screenH - margin - arm }, { screenW - margin, screenH - margin }, 2.0f, frameGold);
    DrawPoly({ screenW - margin, screenH - margin }, 4, 5.0f, 0.0f, GOLD);

    // Статус у верхньому лівому куті
    Color emerald = Color{ 72, 225, 155, 255 };
    if (fmodf(animTime, 1.4f) < 0.95f) {
        DrawCircle((int)(margin + 18.0f), (int)(margin + 18.0f), 4.5f, emerald);
    }
    DrawText("EXIT GATE // CLEARANCE GRANTED", (int)(margin + 32.0f), (int)(margin + 11.0f), 15, ColorAlpha(emerald, 0.88f));

    // =========================================================================
    // 2. КОРОНА / РОМБ ПЕРЕМОЖЦЯ НАД ЗАГОЛОВКОМ
    // =========================================================================
    Vector2 emblemCenter = { centerX, screenH * 0.145f };
    float pulse = 0.85f + 0.15f * sinf(animTime * 3.0f);

    DrawLineEx({ centerX - 180.0f, emblemCenter.y }, { centerX - 38.0f, emblemCenter.y }, 1.2f, ColorAlpha(GOLD, 0.45f));
    DrawLineEx({ centerX + 38.0f, emblemCenter.y }, { centerX + 180.0f, emblemCenter.y }, 1.2f, ColorAlpha(GOLD, 0.45f));

    DrawPoly(emblemCenter, 4, 24.0f, 0.0f, Color{ 12, 18, 15, 235 });
    DrawPolyLinesEx(emblemCenter, 4, 24.0f, 0.0f, 1.8f, ColorAlpha(GOLD, 0.92f * pulse));
    DrawPolyLinesEx(emblemCenter, 4, 17.0f, 0.0f, 1.0f, ColorAlpha(emerald, 0.65f));
    DrawPoly(emblemCenter, 4, 7.5f, animTime * 25.0f, GOLD);

    // =========================================================================
    // 3. ГОЛОВНИЙ ЗАГОЛОВОК "CASINO ESCAPED"
    // =========================================================================
    const char* title = "CASINO ESCAPED";
    int titleSize = 62;
    int titleW = MeasureText(title, titleSize);
    int titleX = (int)(centerX - titleW * 0.5f);
    int titleY = (int)(screenH * 0.215f);

    DrawText(title, titleX + 3, titleY + 4, titleSize, Fade(BLACK, 0.85f));
    DrawText(title, titleX - 1, titleY, titleSize, ColorAlpha(emerald, 0.35f * pulse));
    DrawText(title, titleX + 1, titleY, titleSize, ColorAlpha(GOLD, 0.45f * pulse));
    DrawText(title, titleX, titleY, titleSize, Color{ 255, 228, 120, 255 });

    // Підзаголовок
    const char* subtitle = "THE HOUSE LOSES TONIGHT";
    int subSize = 18;
    int subW = MeasureText(subtitle, subSize);
    int subY = titleY + titleSize + 10;
    DrawText(subtitle, (int)(centerX - subW * 0.5f), subY, subSize, ColorAlpha(emerald, 0.90f));

    // Роздільна лінія Art Deco під підзаголовком
    float divY = (float)(subY + subSize + 20);
    DrawLineEx({ centerX - 240.0f, divY }, { centerX - 16.0f, divY }, 1.5f, ColorAlpha(GOLD, 0.55f));
    DrawLineEx({ centerX + 16.0f, divY }, { centerX + 240.0f, divY }, 1.5f, ColorAlpha(GOLD, 0.55f));
    DrawPoly({ centerX - 240.0f, divY }, 4, 4.0f, 0.0f, GOLD);
    DrawPoly({ centerX + 240.0f, divY }, 4, 4.0f, 0.0f, GOLD);
    DrawPoly({ centerX, divY }, 4, 6.0f, 0.0f, GOLD);

    // =========================================================================
    // 4. АТМОСФЕРНА ЦИТАТА ПЕРЕМОЖЦЯ
    // =========================================================================
    int quoteSize = 20;
    int quoteW = MeasureText(chosenQuote.c_str(), quoteSize);
    int quoteX = (int)(centerX - quoteW * 0.5f);
    int quoteY = (int)(divY + 22.0f);

    DrawText(chosenQuote.c_str(), quoteX + 1, quoteY + 2, quoteSize, Fade(BLACK, 0.85f));
    DrawText(chosenQuote.c_str(), quoteX, quoteY, quoteSize, Color{ 232, 226, 205, 240 });

    // =========================================================================
    // 5. КНОПКИ ДІЇ ТА КАСТОМНИЙ КУРСОР
    // =========================================================================
    GameplayUI::DrawArtDecoButton(playAgainButton, animTime);
    GameplayUI::DrawArtDecoButton(mainMenuButton, animTime);

    const char* hintText = "[ENTER] PLAY AGAIN   //   [ESC] MAIN MENU";
    int hintSize = 13;
    int hintW = MeasureText(hintText, hintSize);
    DrawText(hintText, (int)(centerX - hintW * 0.5f), (int)(screenH - margin - 26.0f), hintSize, ColorAlpha(LIGHTGRAY, 0.48f));

    // Плавне проявлення екрана при вході
    if (entranceAlpha < 1.0f) {
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 1.0f - entranceAlpha));
    }

    CustomCursor::Draw(playAgainButton.isHovered || mainMenuButton.isHovered);
}