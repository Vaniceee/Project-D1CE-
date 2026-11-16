#include "GameOverState.h"
#include "MainMenuState.h"
#include "GameplayState.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "config.h"
#include "Logger.h"
#include "raymath.h"
#include <cmath>

void GameOverState::Init() {
    AssetManager& am = AssetManager::Instance();

    // 1. Завантаження розірваного туза (останній кадр спрайтшиту життів)
    am.LoadTexture("card_lives", Config::PATH_PNG_CARD_LIVES);
    tornCardTexture = am.GetTexture("card_lives");
    hasTornCard = (tornCardTexture.id > 0);

    // 2. ШАБЛОН: фон (якщо файл з'явиться — шейдер автоматично накладе перешкоди поверх нього)
    // am.LoadTexture("GameOverBG", "assets/menu/GameOverBG.png");
    bgTexture = am.GetTexture("GameOverBG");
    hasBackground = (bgTexture.id > 0);

    // 3. Ініціалізація шейдера втраченого сигналу
    gameOverShader = LoadShader(0, Config::PATH_SHADER_GAMEOVER);
    if (gameOverShader.id != 0) {
        resLoc = GetShaderLocation(gameOverShader, "resolution");
        timeLoc = GetShaderLocation(gameOverShader, "time");
        fadeInLoc = GetShaderLocation(gameOverShader, "fadeIn");
        hasBgLoc = GetShaderLocation(gameOverShader, "hasBgTexture");

        float res[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(gameOverShader, resLoc, res, SHADER_UNIFORM_VEC2);

        float zero = 0.0f;
        float bgFlag = hasBackground ? 1.0f : 0.0f;
        SetShaderValue(gameOverShader, timeLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gameOverShader, fadeInLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gameOverShader, hasBgLoc, &bgFlag, SHADER_UNIFORM_FLOAT);
    }

    // 4. Звук розриву карти при старті екрана поразки
    am.LoadSound("card_rip", Config::PATH_SFX_CARD_RIP);
    gameOverSound = am.GetSound("card_rip");
    hasSound = (gameOverSound.stream.buffer != nullptr);
    if (hasSound) {
        SetSoundPitch(gameOverSound, 0.85f);
        SetSoundVolume(gameOverSound, 0.55f);
        PlaySound(gameOverSound);
    }

    // 5. Генерація тактильного звуку наведення на кнопки
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
            SetSoundVolume(hoverSound, 0.26f);
        }
    }

    // 6. ШАБЛОН: ембієнт поразки
    // am.LoadMusic("GameOverTheme", "assets/music/gameover_theme.wav");
    gameOverMusic = am.GetMusic("GameOverTheme");
    hasMusic = (gameOverMusic.stream.buffer != nullptr);
    if (hasMusic) {
        SetMusicVolume(gameOverMusic, 0.25f);
        gameOverMusic.looping = true;
        PlayMusicStream(gameOverMusic);
    }

    sceneTimer = 0.0f;
    exitTarget = ExitTarget::NONE;
    exitFadeTimer = 0.0f;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float btnWidth = 310.0f;
    float btnHeight = 58.0f;
    float btnX = (screenW - btnWidth) * 0.5f;

    retryButton.bounds = { btnX, (screenH * 0.62f), btnWidth, btnHeight };
    retryButton.text = "TRY AGAIN";
    retryButton.isHovered = false;
    retryButton.wasHovered = false;
    retryButton.hoverProgress = 0.0f;
    retryButton.isPrimary = true;

    menuButton.bounds = { btnX, (screenH * 0.62f) + 76.0f, btnWidth, btnHeight };
    menuButton.text = "MAIN MENU";
    menuButton.isHovered = false;
    menuButton.wasHovered = false;
    menuButton.hoverProgress = 0.0f;
    menuButton.isPrimary = false;

    GAME_INFO("STATE", "Entered GameOverState");
}

void GameOverState::Cleanup() {
    if (hasMusic) {
        StopMusicStream(gameOverMusic);
    }
    if (hasHoverSound) {
        UnloadSound(hoverSound);
        hoverSound = { 0 };
        hasHoverSound = false;
    }
    if (gameOverShader.id != 0) {
        UnloadShader(gameOverShader);
        gameOverShader = { 0 };
    }
    AssetManager::Instance().UnloadAll();
}

void GameOverState::Update() {
    float dt = GetFrameTime();
    sceneTimer += dt;

    if (hasMusic) {
        UpdateMusicStream(gameOverMusic);
    }

    float fadeProgress = Clamp(sceneTimer / fadeInDuration, 0.0f, 1.0f);
    if (gameOverShader.id != 0) {
        float res[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(gameOverShader, resLoc, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(gameOverShader, timeLoc, &sceneTimer, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gameOverShader, fadeInLoc, &fadeProgress, SHADER_UNIFORM_FLOAT);
    }

    if (exitTarget != ExitTarget::NONE) {
        exitFadeTimer += dt;
        if (hasMusic) {
            float progress = Clamp(exitFadeTimer / exitFadeDuration, 0.0f, 1.0f);
            SetMusicVolume(gameOverMusic, 0.25f * (1.0f - progress));
        }

        if (exitFadeTimer >= exitFadeDuration) {
            if (exitTarget == ExitTarget::RETRY_GAME) {
                GameManager::Instance().ChangeState(new GameplayState());
            }
            else {
                GameManager::Instance().ChangeState(new MainMenuState());
            }
            return;
        }
        return;
    }

    if (sceneTimer >= buttonAppearDelay) {
        Vector2 mousePos = GetMousePosition();

        retryButton.wasHovered = retryButton.isHovered;
        menuButton.wasHovered = menuButton.isHovered;

        retryButton.isHovered = CheckCollisionPointRec(mousePos, retryButton.bounds);
        menuButton.isHovered = CheckCollisionPointRec(mousePos, menuButton.bounds);

        if (hasHoverSound) {
            if (retryButton.isHovered && !retryButton.wasHovered) {
                SetSoundPitch(hoverSound, 1.08f);
                PlaySound(hoverSound);
            }
            else if (menuButton.isHovered && !menuButton.wasHovered) {
                SetSoundPitch(hoverSound, 0.86f);
                PlaySound(hoverSound);
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (retryButton.isHovered) {
                exitTarget = ExitTarget::RETRY_GAME;
                exitFadeTimer = 0.0f;
            }
            else if (menuButton.isHovered) {
                exitTarget = ExitTarget::MAIN_MENU;
                exitFadeTimer = 0.0f;
            }
        }
    }

    float animSpeed = 1.0f - expf(-12.0f * dt);
    retryButton.hoverProgress = Lerp(retryButton.hoverProgress, retryButton.isHovered ? 1.0f : 0.0f, animSpeed);
    menuButton.hoverProgress = Lerp(menuButton.hoverProgress, menuButton.isHovered ? 1.0f : 0.0f, animSpeed);
}

void GameOverState::Draw() {
    ClearBackground(BLACK);

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float fadeProgress = Clamp(sceneTimer / fadeInDuration, 0.0f, 1.0f);

    // 1. Фон із шейдером втраченого сигналу (NO SIGNAL)
    if (gameOverShader.id != 0) {
        BeginShaderMode(gameOverShader);
    }

    if (hasBackground) {
        DrawTexturePro(
            bgTexture,
            Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
            Rectangle{ 0, 0, screenW, screenH },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );
    }
    else {
        DrawRectangle(0, 0, (int)screenW, (int)screenH, BLACK);
    }

    if (gameOverShader.id != 0) {
        EndShaderMode();
    }

    // 2. Анімований розірваний туз над заголовком
    DrawTornAceCard(screenW, screenH, fadeProgress);

    // 3. Багряний неоновий заголовок "GAME OVER"
    DrawNeonGameOverTitle(screenW, screenH, fadeProgress);

    // 4. Кнопки TRY AGAIN та MAIN MENU у стилі Art Deco
    if (sceneTimer >= buttonAppearDelay) {
        float btnAlpha = Clamp((sceneTimer - buttonAppearDelay) / 0.55f, 0.0f, 1.0f);
        DrawButton(retryButton, btnAlpha);
        DrawButton(menuButton, btnAlpha);
    }

    // 5. Кутові візири CCTV та статус втрати сигналу
    DrawNoSignalHud(screenW, screenH, fadeProgress);

    // 6. Плавне затемнення при переході
    if (exitTarget != ExitTarget::NONE) {
        float outAlpha = Clamp(exitFadeTimer / exitFadeDuration, 0.0f, 1.0f);
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, outAlpha));
    }
}

void GameOverState::DrawTornAceCard(float screenW, float screenH, float fadeProgress) {
    if (!hasTornCard) return;

    // Останній (4-й) кадр спрайтшиту LifesAce.png — повністю розірвана карта
    const float frameWidth = 32.0f;
    const float frameHeight = 32.0f;
    Rectangle sourceRec = { 3.0f * frameWidth, 0.0f, frameWidth, frameHeight };

    // Ефект падіння/удару в перші 0.45 с + легке погойдування
    float dropT = Clamp(sceneTimer / 0.45f, 0.0f, 1.0f);
    float easeOut = 1.0f - powf(1.0f - dropT, 3.0f);
    float scale = Lerp(6.8f, 4.6f, easeOut);
    float rotation = sinf(sceneTimer * 1.8f) * 4.5f + (1.0f - easeOut) * -14.0f;

    Vector2 centerPos = {
        screenW * 0.5f,
        screenH * 0.17f + sinf(sceneTimer * 2.2f) * 4.0f
    };

    Rectangle destRec = {
        centerPos.x,
        centerPos.y,
        frameWidth * scale,
        frameHeight * scale
    };
    Vector2 origin = { destRec.width * 0.5f, destRec.height * 0.5f };

    // Багряний ореол позаду карти
    DrawCircleV(centerPos, 78.0f, ColorAlpha(MAROON, 0.10f * fadeProgress));
    DrawCircleV(centerPos, 54.0f, ColorAlpha(MAROON, 0.16f * fadeProgress));
    DrawCircleV(centerPos, 32.0f, ColorAlpha(MAROON, 0.22f * fadeProgress));
    DrawTexturePro(tornCardTexture, sourceRec, destRec, origin, rotation, ColorAlpha(WHITE, fadeProgress));
}

void GameOverState::DrawNeonGameOverTitle(float screenW, float screenH, float fadeProgress) {
    const char* titleText = "GAME OVER";
    int titleSize = 74;
    int titleWidth = MeasureText(titleText, titleSize);

    float jitterX = 0.0f;
    float jitterY = 0.0f;
    if (sceneTimer < 0.75f) {
        float decay = 1.0f - (sceneTimer / 0.75f);
        jitterX = (float)GetRandomValue(-5, 5) * decay;
        jitterY = (float)GetRandomValue(-3, 3) * decay;
    }

    // Мерехтіння розбитої червоної неонової вивіски
    float cycle = fmodf(sceneTimer, 3.6f);
    float tubeFlicker = 0.90f + 0.10f * sinf(sceneTimer * 7.5f);
    if (cycle > 2.95f && cycle < 3.12f) {
        tubeFlicker = 0.42f;
    }
    float alpha = fadeProgress * tubeFlicker;

    int titleX = (int)((screenW - titleWidth) * 0.5f + jitterX);
    int titleY = (int)(screenH * 0.31f + jitterY);

    // Темна підкладка
    DrawRectangleGradientH(titleX - 70, titleY - 14, titleWidth / 2 + 70, titleSize + 28, Fade(BLACK, 0.0f), Fade(BLACK, 0.58f * fadeProgress));
    DrawRectangleGradientH(titleX + titleWidth / 2, titleY - 14, titleWidth / 2 + 70, titleSize + 28, Fade(BLACK, 0.58f * fadeProgress), Fade(BLACK, 0.0f));

    // Хроматична тінь
    DrawText(titleText, titleX - 3, titleY + 2, titleSize, ColorAlpha(SKYBLUE, 0.22f * alpha));
    DrawText(titleText, titleX + 3, titleY - 2, titleSize, ColorAlpha(MAROON, 0.55f * alpha));

    // Багряний неоновий ореол
    Color outerGlow = ColorAlpha(MAROON, 0.35f * alpha);
    Color innerGlow = ColorAlpha(RED, 0.48f * alpha);

    DrawText(titleText, titleX - 2, titleY, titleSize, outerGlow);
    DrawText(titleText, titleX + 2, titleY, titleSize, outerGlow);
    DrawText(titleText, titleX, titleY - 2, titleSize, outerGlow);
    DrawText(titleText, titleX, titleY + 2, titleSize, outerGlow);

    DrawText(titleText, titleX - 1, titleY - 1, titleSize, innerGlow);
    DrawText(titleText, titleX + 1, titleY + 1, titleSize, innerGlow);

    // Серцевина тексту
    DrawText(titleText, titleX, titleY, titleSize, ColorAlpha(Color{ 255, 68, 76, 255 }, alpha));

    // Декоративна лінія з багряним ромбом
    float centerX = screenW * 0.5f;
    float lineY = (float)(titleY + titleSize + 12);
    float halfSpan = titleWidth * 0.45f;

    DrawLineEx({ centerX - halfSpan, lineY }, { centerX - 14.0f, lineY }, 1.5f, ColorAlpha(RED, 0.45f * alpha));
    DrawLineEx({ centerX + 14.0f, lineY }, { centerX + halfSpan, lineY }, 1.5f, ColorAlpha(RED, 0.45f * alpha));
    DrawPoly({ centerX, lineY }, 4, 5.5f, 0.0f, ColorAlpha(RED, 0.85f * alpha));

    // Підзаголовок
    const char* subText = "The house always wins...";
    int subSize = 23;
    int subWidth = MeasureText(subText, subSize);
    DrawText(
        subText,
        (int)((screenW - subWidth) * 0.5f),
        (int)(lineY + 16.0f),
        subSize,
        ColorAlpha(Color{ 210, 195, 185, 255 }, fadeProgress * 0.82f)
    );
}

void GameOverState::DrawButton(const GameOverButton& btn, float alpha) {
    float p = btn.hoverProgress;
    float expandX = p * 12.0f;
    float expandY = p * 2.0f;

    float jitterX = (!btn.isPrimary && p > 0.05f) ? sinf(sceneTimer * 56.0f) * 1.2f * p : 0.0f;
    float jitterY = (!btn.isPrimary && p > 0.05f) ? cosf(sceneTimer * 45.0f) * 0.8f * p : 0.0f;

    Rectangle rect = {
        btn.bounds.x - expandX + jitterX,
        btn.bounds.y - expandY + jitterY,
        btn.bounds.width + expandX * 2.0f,
        btn.bounds.height + expandY * 2.0f
    };

    Color accentColor = btn.isPrimary ? GOLD : Color{ 225, 48, 56, 255 };
    Color darkFillBase = btn.isPrimary ? Color{ 12, 13, 16, 225 } : Color{ 16, 10, 12, 225 };
    Color hoverTint = btn.isPrimary ? ColorAlpha(GOLD, 0.16f * p * alpha) : ColorAlpha(MAROON, 0.36f * p * alpha);

    DrawRectangle((int)rect.x + 4, (int)rect.y + 5, (int)rect.width, (int)rect.height, ColorAlpha(BLACK, 0.7f * alpha));
    if (p > 0.01f) {
        Rectangle glowRect = { rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
        DrawRectangleLinesEx(glowRect, 2.0f, ColorAlpha(accentColor, 0.25f * p * alpha));
    }

    DrawRectangleRec(rect, ColorAlpha(darkFillBase, alpha));
    if (p > 0.01f) {
        DrawRectangleRec(rect, hoverTint);
    }

    if (p > 0.02f) {
        BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
        float sweepPhase = fmodf(sceneTimer * 1.35f, 1.8f) - 0.4f;
        float sweepX = rect.x + sweepPhase * rect.width;
        DrawLineEx(
            { sweepX - 18.0f, rect.y + rect.height },
            { sweepX + 18.0f, rect.y },
            14.0f,
            ColorAlpha(accentColor, 0.14f * p * alpha)
        );
        EndScissorMode();
    }

    Color outerBorder = ColorAlpha(ColorLerp(Color{ 95, 95, 102, 200 }, accentColor, p), alpha);
    Color innerBorder = ColorAlpha(accentColor, (0.18f + 0.45f * p) * alpha);

    DrawRectangleLinesEx(rect, 2.0f, outerBorder);
    Rectangle innerRect = { rect.x + 5.0f, rect.y + 5.0f, rect.width - 10.0f, rect.height - 10.0f };
    DrawRectangleLinesEx(innerRect, 1.0f, innerBorder);

    float cornerR = 3.0f + p * 1.0f;
    Color cornerCol = ColorAlpha(ColorLerp(GRAY, accentColor, p), alpha);
    DrawPoly({ innerRect.x, innerRect.y }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x + innerRect.width, innerRect.y }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x, innerRect.y + innerRect.height }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x + innerRect.width, innerRect.y + innerRect.height }, 4, cornerR, 0.0f, cornerCol);

    int fontSize = 24;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = rect.x + (rect.width - textWidth) * 0.5f;
    float textY = rect.y + (rect.height - fontSize) * 0.5f;

    Color textColor = ColorAlpha(ColorLerp(LIGHTGRAY, WHITE, p), alpha);
    DrawText(btn.text.c_str(), (int)textX + 1, (int)textY + 2, fontSize, ColorAlpha(BLACK, 0.8f * alpha));
    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);

    if (p > 0.05f) {
        float diamondOffset = 18.0f + p * 10.0f;
        float diamondRadius = 4.5f * p;
        float midY = rect.y + rect.height * 0.5f;
        Color sideDiamondCol = ColorAlpha(accentColor, p * alpha);

        DrawPoly({ textX - diamondOffset, midY }, 4, diamondRadius, 0.0f, sideDiamondCol);
        DrawPoly({ textX + textWidth + diamondOffset, midY }, 4, diamondRadius, 0.0f, sideDiamondCol);
    }
}

void GameOverState::DrawNoSignalHud(float screenW, float screenH, float alpha) {
    float margin = 24.0f;
    float armLen = 28.0f;
    float thick = 1.5f;
    Color hudColor = ColorAlpha(Color{ 210, 85, 92, 255 }, 0.34f * alpha);

    DrawLineEx({ margin, margin }, { margin + armLen, margin }, thick, hudColor);
    DrawLineEx({ margin, margin }, { margin, margin + armLen }, thick, hudColor);

    DrawLineEx({ screenW - margin - armLen, margin }, { screenW - margin, margin }, thick, hudColor);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + armLen }, thick, hudColor);

    DrawLineEx({ margin, screenH - margin }, { margin + armLen, screenH - margin }, thick, hudColor);
    DrawLineEx({ margin, screenH - margin - armLen }, { margin, screenH - margin }, thick, hudColor);

    DrawLineEx({ screenW - margin - armLen, screenH - margin }, { screenW - margin, screenH - margin }, thick, hudColor);
    DrawLineEx({ screenW - margin, screenH - margin - armLen }, { screenW - margin, screenH - margin }, thick, hudColor);

    if (alpha > 0.05f) {
        bool blink = (fmodf(sceneTimer, 1.1f) < 0.65f);
        if (blink) {
            DrawCircle((int)(margin + 18.0f), (int)(margin + 18.0f), 4.5f, ColorAlpha(RED, 0.85f * alpha));
        }
        DrawText("NO SIGNAL", (int)(margin + 30.0f), (int)(margin + 11.0f), 15, ColorAlpha(RED, 0.78f * alpha));
        DrawText("CAM-04 // CONNECTION LOST", (int)(margin + 12.0f), (int)(margin + 32.0f), 13, ColorAlpha(LIGHTGRAY, 0.42f * alpha));

        const char* statusText = "STATUS: TERMINATED";
        int statusW = MeasureText(statusText, 13);
        DrawText(statusText, (int)(screenW - margin - statusW - 12.0f), (int)(screenH - margin - 24.0f), 13, ColorAlpha(RED, 0.55f * alpha));
    }
}