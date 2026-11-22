#include "VictoryState.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "CustomCursor.h"
#include "config.h"
#include "raymath.h"
#include <cmath>
#include <vector>

void VictoryState::Init() {
    HideCursor();
    AssetManager& am = AssetManager::Instance();

    am.LoadTexture("card_lives", Config::PATH_PNG_CARD_LIVES);
    cardTex = am.GetTexture("card_lives");
    hasCardTex = (cardTex.id > 0);

    Wave clickWave = { 0 };
    clickWave.sampleRate = 44100;
    clickWave.sampleSize = 16;
    clickWave.channels = 1;
    clickWave.frameCount = 1100;
    short* samples = (short*)RL_MALLOC(clickWave.frameCount * sizeof(short));
    if (samples != nullptr) {
        for (unsigned int i = 0; i < clickWave.frameCount; ++i) {
            float t = (float)i / (float)clickWave.sampleRate;
            float env = expf(-t * 145.0f);
            float sig = sinf(2.0f * PI * 940.0f * t) * 0.65f + sinf(2.0f * PI * 1880.0f * t) * 0.35f;
            samples[i] = (short)(sig * env * 11000.0f);
        }
        clickWave.data = samples;
        hoverSound = LoadSoundFromWave(clickWave);
        UnloadWave(clickWave);
        hasHoverSound = (hoverSound.stream.buffer != nullptr);
        if (hasHoverSound) {
            SetSoundVolume(hoverSound, 0.28f);
        }
    }

    const std::vector<std::string> victoryQuotes = {
        "YOU BEAT THE HOUSE AT ITS OWN GAME.",
        "ROYAL FLUSH. AGAINST ALL ODDS, YOU WALKED OUT ALIVE.",
        "THE VAULT IS EMPTY, AND THE NIGHT IS YOURS.",
        "LOADED DICE COULDN'T STOP A WINNING HAND.",
        "CLEAN GETAWAY. THE CASINO NEVER SAW IT COMING."
    };

    int randomIdx = GetRandomValue(0, (int)victoryQuotes.size() - 1);
    selectedQuote = victoryQuotes[randomIdx];

    animTime = 0.0f;
    fadeInAlpha = 1.0f;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float btnWidth = 310.0f;
    float btnHeight = 58.0f;
    float btnX = (screenW - btnWidth) * 0.5f;

    playAgainButton.bounds = { btnX, screenH * 0.65f, btnWidth, btnHeight };
    playAgainButton.text = "PLAY AGAIN";
    playAgainButton.isHovered = false;
    playAgainButton.wasHovered = false;
    playAgainButton.hoverProgress = 0.0f;
    playAgainButton.isDanger = false;

    mainMenuButton.bounds = { btnX, screenH * 0.65f + 78.0f, btnWidth, btnHeight };
    mainMenuButton.text = "MAIN MENU";
    mainMenuButton.isHovered = false;
    mainMenuButton.wasHovered = false;
    mainMenuButton.hoverProgress = 0.0f;
    mainMenuButton.isDanger = true;
}

void VictoryState::Cleanup() {
    if (hasHoverSound) {
        UnloadSound(hoverSound);
        hoverSound = { 0 };
        hasHoverSound = false;
    }
    AssetManager::Instance().UnloadAll();
}

void VictoryState::Update() {
    float dt = GetFrameTime();
    animTime += dt;

    if (fadeInAlpha > 0.0f) {
        fadeInAlpha -= dt * 1.4f;
        if (fadeInAlpha < 0.0f) fadeInAlpha = 0.0f;
    }

    Vector2 mousePos = GetMousePosition();

    playAgainButton.wasHovered = playAgainButton.isHovered;
    mainMenuButton.wasHovered = mainMenuButton.isHovered;

    playAgainButton.isHovered = CheckCollisionPointRec(mousePos, playAgainButton.bounds);
    mainMenuButton.isHovered = CheckCollisionPointRec(mousePos, mainMenuButton.bounds);

    if (hasHoverSound) {
        if (playAgainButton.isHovered && !playAgainButton.wasHovered) {
            SetSoundPitch(hoverSound, 1.12f);
            PlaySound(hoverSound);
        }
        else if (mainMenuButton.isHovered && !mainMenuButton.wasHovered) {
            SetSoundPitch(hoverSound, 0.86f);
            PlaySound(hoverSound);
        }
    }

    float animSpeed = 1.0f - expf(-12.0f * dt);
    playAgainButton.hoverProgress = Lerp(playAgainButton.hoverProgress, playAgainButton.isHovered ? 1.0f : 0.0f, animSpeed);
    mainMenuButton.hoverProgress = Lerp(mainMenuButton.hoverProgress, mainMenuButton.isHovered ? 1.0f : 0.0f, animSpeed);

    CustomCursor::Update(dt, playAgainButton.isHovered || mainMenuButton.isHovered);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (playAgainButton.isHovered) {
            GameManager::Instance().ChangeState(new GameplayState());
            return;
        }
        else if (mainMenuButton.isHovered) {
            GameManager::Instance().ChangeState(new MainMenuState());
            return;
        }
    }
}

void VictoryState::Draw() {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // 1. Глибокий смарагдово-обсидіановий фон казино
    DrawRectangleGradientV(0, 0, (int)screenW, (int)screenH, Color{ 10, 26, 20, 255 }, Color{ 6, 9, 11, 255 });

    // Золоте сяйво по центру
    float pulse = 0.72f + 0.28f * sinf(animTime * 2.6f);
    Vector2 center = { screenW * 0.5f, screenH * 0.26f };
    for (int i = 12; i >= 1; --i) {
        float t = (float)i / 12.0f;
        DrawCircleV(center, 260.0f * t, ColorAlpha(GOLD, (1.0f - t) * 0.045f * pulse));
    }

    // Фонові ромби Ар-Деко
    float rotSlow = sinf(animTime * 0.35f) * 3.5f;
    DrawPolyLinesEx(center, 4, 215.0f + pulse * 6.0f, rotSlow, 1.5f, ColorAlpha(GOLD, 0.16f));
    DrawPolyLinesEx(center, 4, 260.0f, -rotSlow * 0.7f, 1.0f, ColorAlpha(Color{ 65, 215, 130, 255 }, 0.10f));

    // Сканлайни CCTV
    for (int y = 0; y < (int)screenH; y += 4) {
        DrawRectangle(0, y, (int)screenW, 1, Fade(WHITE, 0.02f));
    }

    // 2. Кутові візири CCTV
    float margin = 26.0f;
    float arm = 28.0f;
    Color hudCol = ColorAlpha(GOLD, 0.45f);
    DrawLineEx({ margin, margin }, { margin + arm, margin }, 1.5f, hudCol);
    DrawLineEx({ margin, margin }, { margin, margin + arm }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin - arm, margin }, { screenW - margin, margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + arm }, 1.5f, hudCol);
    DrawLineEx({ margin, screenH - margin }, { margin + arm, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ margin, screenH - margin - arm }, { margin, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin - arm, screenH - margin }, { screenW - margin, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin, screenH - margin - arm }, { screenW - margin, screenH - margin }, 1.5f, hudCol);

    if (fmodf(animTime, 1.3f) < 0.8f) {
        DrawCircle((int)(margin + 16.0f), (int)(margin + 18.0f), 4.5f, ColorAlpha(GREEN, 0.90f));
    }
    DrawText("FEED TERMINATED // SUBJECT ESCAPED", (int)(margin + 30.0f), (int)(margin + 11.0f), 14, ColorAlpha(LIGHTGRAY, 0.75f));

    // 3. Цілий козирний Туз у центрі композиції
    Vector2 cardCenter = { screenW * 0.5f, screenH * 0.23f };
    DrawPolyLinesEx(cardCenter, 4, 78.0f, 0.0f, 1.5f, ColorAlpha(GOLD, 0.55f));

    if (hasCardTex) {
        const float frameW = 32.0f;
        const float frameH = 32.0f;
        Rectangle srcRec = { 0.0f, 0.0f, frameW, frameH }; // 0-й кадр: цілий золотий Туз
        float cardScale = 4.6f;
        float floatY = sinf(animTime * 2.2f) * 3.5f;
        float tilt = sinf(animTime * 1.5f) * 2.5f;

        Rectangle dstRec = {
            cardCenter.x,
            cardCenter.y + floatY,
            frameW * cardScale,
            frameH * cardScale
        };
        DrawTexturePro(cardTex, srcRec, dstRec, { dstRec.width * 0.5f, dstRec.height * 0.5f }, tilt, WHITE);
    }

    // 4. Золотий заголовок YOU ESCAPED
    const char* titleText = "YOU ESCAPED";
    int titleSize = 62;
    int titleW = MeasureText(titleText, titleSize);
    int titleX = (int)((screenW - titleW) * 0.5f);
    int titleY = (int)(screenH * 0.38f);

    DrawText(titleText, titleX - 2, titleY + 2, titleSize, ColorAlpha(ORANGE, 0.35f * pulse));
    DrawText(titleText, titleX + 2, titleY - 1, titleSize, ColorAlpha(GOLD, 0.45f * pulse));
    DrawText(titleText, titleX, titleY, titleSize, Color{ 255, 226, 110, 255 });

    // Філігранний роздільник Ар-Деко
    float lineY = (float)(titleY + titleSize + 14);
    float centerX = screenW * 0.5f;
    DrawLineEx({ centerX - 180.0f, lineY }, { centerX - 14.0f, lineY }, 1.5f, ColorAlpha(GOLD, 0.55f));
    DrawLineEx({ centerX + 14.0f, lineY }, { centerX + 180.0f, lineY }, 1.5f, ColorAlpha(GOLD, 0.55f));
    DrawPoly({ centerX, lineY }, 4, 5.5f, 0.0f, GOLD);

    // 5. Переможна цитата
    int quoteSize = 18;
    int quoteW = MeasureText(selectedQuote.c_str(), quoteSize);
    DrawText(
        selectedQuote.c_str(),
        (int)((screenW - quoteW) * 0.5f),
        (int)(lineY + 18.0f),
        quoteSize,
        ColorAlpha(Color{ 238, 224, 175, 255 }, 0.92f)
    );

    // 6. Кнопки PLAY AGAIN та MAIN MENU
    GameplayUI::DrawArtDecoButton(playAgainButton, animTime);
    GameplayUI::DrawArtDecoButton(mainMenuButton, animTime);

    CustomCursor::Draw(mainMenuButton.isHovered);

    if (fadeInAlpha > 0.0f) {
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, fadeInAlpha));
    }
}