#include "MainMenuState.h"
#include "GameplayState.h"
#include "GameManager.h"
#include "Logger.h"
#include "CustomCursor.h"
#include "raymath.h"
#include <cmath>

void MainMenuState::Init() {
    HideCursor();
    AssetManager& am = AssetManager::Instance();

    // 1. Завантаження фону
    am.LoadTexture("MainMenuBG.jpg", Config::PATH_PNG_MENU_BG);
    bgTexture = am.GetTexture("MainMenuBG.jpg");
    hasBackground = (bgTexture.id > 0);

    // 2. Ініціалізація шейдера меню
    menuShader = LoadShader(0, Config::PATH_SHADER_MENU);
    if (menuShader.id != 0) {
        menuResLoc = GetShaderLocation(menuShader, "resolution");
        menuTimeLoc = GetShaderLocation(menuShader, "time");
        menuTransitionLoc = GetShaderLocation(menuShader, "transitionProgress");
        menuMouseLoc = GetShaderLocation(menuShader, "mouseOffset");

        float res[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
        SetShaderValue(menuShader, menuResLoc, res, SHADER_UNIFORM_VEC2);

        float zero = 0.0f;
        float zeroVec[2] = { 0.0f, 0.0f };
        SetShaderValue(menuShader, menuTimeLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(menuShader, menuTransitionLoc, &zero, SHADER_UNIFORM_FLOAT);
        SetShaderValue(menuShader, menuMouseLoc, zeroVec, SHADER_UNIFORM_VEC2);
    }
    shaderTime = 0.0f;
    smoothedMouseOffset = { 0.0f, 0.0f };

    // 3. Завантаження фонової музики
    am.LoadMusic("MenuTheme.wav", Config::PATH_AUDIO_MENU_MUSIC);
    bgMusic = am.GetMusic("MenuTheme.wav");
    hasMusic = (bgMusic.stream.buffer != nullptr);

    if (hasMusic) {
        SetMusicVolume(bgMusic, 0.24f);
        SetMusicPitch(bgMusic, 1.0f);
        bgMusic.looping = true;
        PlayMusicStream(bgMusic);
    }

    am.LoadSound("btn_start", "assets/menu/PlaySFX.wav");
    startSound = am.GetSound("btn_start");
    hasStartSound = (startSound.stream.buffer != nullptr);

    // 4. Генерація короткого тактильного клацання для наведення на кнопки
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

    // 5. Скидання стану переходу та кнопок
    isTransitioning = false;
    transitionTimer = 0.0f;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float btnWidth = 300.0f;
    float btnHeight = 58.0f;
    float btnX = (screenW - btnWidth) * 0.5f;

    playButton.bounds = { btnX, (screenH * 0.5f) - 38.0f, btnWidth, btnHeight };
    playButton.text = "PLAY";
    playButton.isHovered = false;
    playButton.wasHovered = false;
    playButton.hoverProgress = 0.0f;
    playButton.isDanger = false;

    exitButton.bounds = { btnX, (screenH * 0.5f) + 42.0f, btnWidth, btnHeight };
    exitButton.text = "EXIT";
    exitButton.isHovered = false;
    exitButton.wasHovered = false;
    exitButton.hoverProgress = 0.0f;
    exitButton.isDanger = true;
}

void MainMenuState::Cleanup() {
    if (hasMusic) {
        StopMusicStream(bgMusic);
    }
    if (hasHoverSound) {
        UnloadSound(hoverSound);
        hoverSound = { 0 };
        hasHoverSound = false;
    }
    if (menuShader.id != 0) {
        UnloadShader(menuShader);
        menuShader = { 0 };
    }
}

void MainMenuState::Update() {
    float deltaTime = GetFrameTime();
    shaderTime += deltaTime;

    if (hasMusic) {
        UpdateMusicStream(bgMusic);
    }

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    Vector2 mousePos = GetMousePosition();

    Vector2 targetMouseOffset = { 0.0f, 0.0f };
    if (!isTransitioning && screenW > 1.0f && screenH > 1.0f) {
        targetMouseOffset.x = Clamp((mousePos.x / screenW) * 2.0f - 1.0f, -1.0f, 1.0f);
        targetMouseOffset.y = Clamp((mousePos.y / screenH) * 2.0f - 1.0f, -1.0f, 1.0f);
    }
    float mouseBlend = 1.0f - expf(-6.5f * deltaTime);
    smoothedMouseOffset.x = Lerp(smoothedMouseOffset.x, targetMouseOffset.x, mouseBlend);
    smoothedMouseOffset.y = Lerp(smoothedMouseOffset.y, targetMouseOffset.y, mouseBlend);

    float progress = 0.0f;

    if (isTransitioning) {
        transitionTimer += deltaTime;
        progress = transitionTimer / transitionDuration;
        if (progress > 1.0f) progress = 1.0f;

        if (hasMusic) {
            float currentVolume = 0.24f * (1.0f - progress);
            float currentPitch = 1.0f - (progress * 0.4f);

            SetMusicVolume(bgMusic, currentVolume);
            SetMusicPitch(bgMusic, currentPitch);
        }

        if (hasStartSound) {
            float startSoundBaseVolume = 0.5f;
            float currentStartVolume = startSoundBaseVolume * (1.2f - progress);

            SetSoundVolume(startSound, currentStartVolume);
        }

        if (transitionTimer >= transitionDuration) {
            GameManager::Instance().ChangeState(new GameplayState());
            return;
        }
    }
    else {
        playButton.wasHovered = playButton.isHovered;
        exitButton.wasHovered = exitButton.isHovered;

        playButton.isHovered = CheckCollisionPointRec(mousePos, playButton.bounds);
        exitButton.isHovered = CheckCollisionPointRec(mousePos, exitButton.bounds);

        if (hasHoverSound) {
            if (playButton.isHovered && !playButton.wasHovered) {
                SetSoundPitch(hoverSound, 1.08f);
                PlaySound(hoverSound);
            }
            else if (exitButton.isHovered && !exitButton.wasHovered) {
                SetSoundPitch(hoverSound, 0.86f);
                PlaySound(hoverSound);
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (playButton.isHovered) {
                isTransitioning = true;
                transitionTimer = 0.0f;

                if (hasStartSound) {
                    PlaySound(startSound);
                }
            }
            else if (exitButton.isHovered) {
                GameManager::Instance().Quit();
                return;
            }
        }
    }

    float hoverAnimSpeed = 1.0f - expf(-12.0f * deltaTime);
    playButton.hoverProgress = Lerp(playButton.hoverProgress, (playButton.isHovered && !isTransitioning) ? 1.0f : 0.0f, hoverAnimSpeed);
    exitButton.hoverProgress = Lerp(exitButton.hoverProgress, (exitButton.isHovered && !isTransitioning) ? 1.0f : 0.0f, hoverAnimSpeed);

    bool anyHovered = !isTransitioning && (playButton.isHovered || exitButton.isHovered);
    CustomCursor::Update(deltaTime, anyHovered);

    if (menuShader.id != 0) {
        float res[2] = { screenW, screenH };
        float mouseArr[2] = { smoothedMouseOffset.x, smoothedMouseOffset.y };
        SetShaderValue(menuShader, menuResLoc, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(menuShader, menuTimeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(menuShader, menuTransitionLoc, &progress, SHADER_UNIFORM_FLOAT);
        SetShaderValue(menuShader, menuMouseLoc, mouseArr, SHADER_UNIFORM_VEC2);
    }
}

void MainMenuState::Draw() {
    ClearBackground(BLACK);

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    if (hasBackground) {
        if (menuShader.id != 0) {
            BeginShaderMode(menuShader);
        }

        DrawTexturePro(
            bgTexture,
            Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
            Rectangle{ 0, 0, screenW, screenH },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );

        if (menuShader.id != 0) {
            EndShaderMode();
        }
    }

    DrawNeonTitle(screenW, screenH);

    DrawButton(playButton);
    DrawButton(exitButton);

    float progress = isTransitioning ? Clamp(transitionTimer / transitionDuration, 0.0f, 1.0f) : 0.0f;
    DrawCctvOverlay(screenW, screenH, progress);

    if (isTransitioning) {
        Vector2 center = { screenW * 0.5f, screenH * 0.5f };

        float maxRadius = sqrtf(screenW * screenW + screenH * screenH) * 0.5f;
        float innerRadius = maxRadius * (1.0f - progress);
        float outerRadius = maxRadius * 1.5f;
        DrawRing(center, innerRadius, outerRadius, 0.0f, 360.0f, 64, BLACK);
        if (progress > 0.2f && innerRadius > 2.0f) {
            DrawCircleLinesV(center, innerRadius, ColorAlpha(RED, 0.4f * progress));
        }
    }
    else {
        CustomCursor::Draw(exitButton.isHovered);
    }
}

void MainMenuState::DrawNeonTitle(float screenW, float screenH) {
    std::string titleStr = "_PR0JECT_D1CE";
    float cycle = fmodf(shaderTime, 4.2f);
    float tubeBrightness = 1.0f;

    if (cycle > 3.55f && cycle < 3.72f) {
        tubeBrightness = 0.45f;
        titleStr[3] = (fmodf(shaderTime * 30.0f, 2.0f) > 1.0f) ? '7' : '#';
    }
    else if (cycle > 3.78f && cycle < 3.88f) {
        tubeBrightness = 0.65f;
        titleStr[10] = '7';
    }

    int fontSize = 62;
    int textWidth = MeasureText(titleStr.c_str(), fontSize);
    int titleX = (int)((screenW - textWidth) * 0.5f - smoothedMouseOffset.x * 5.0f);
    int titleY = (int)(screenH * 0.23f - smoothedMouseOffset.y * 3.5f);

    float breathe = 0.88f + 0.12f * sinf(shaderTime * 3.2f);
    float glowAlpha = breathe * tubeBrightness;

    DrawRectangleGradientH(titleX - 60, titleY - 14, textWidth / 2 + 60, fontSize + 28, Fade(BLACK, 0.0f), Fade(BLACK, 0.52f));
    DrawRectangleGradientH(titleX + textWidth / 2, titleY - 14, textWidth / 2 + 60, fontSize + 28, Fade(BLACK, 0.52f), Fade(BLACK, 0.0f));

    DrawText(titleStr.c_str(), titleX - 3, titleY + 2, fontSize, ColorAlpha(RED, 0.35f * glowAlpha));
    DrawText(titleStr.c_str(), titleX + 3, titleY - 1, fontSize, ColorAlpha(SKYBLUE, 0.18f * glowAlpha));

    Color outerGlow = ColorAlpha(ORANGE, 0.22f * glowAlpha);
    Color innerGlow = ColorAlpha(GOLD, 0.42f * glowAlpha);

    DrawText(titleStr.c_str(), titleX - 2, titleY, fontSize, outerGlow);
    DrawText(titleStr.c_str(), titleX + 2, titleY, fontSize, outerGlow);
    DrawText(titleStr.c_str(), titleX, titleY - 2, fontSize, outerGlow);
    DrawText(titleStr.c_str(), titleX, titleY + 2, fontSize, outerGlow);

    DrawText(titleStr.c_str(), titleX - 1, titleY - 1, fontSize, innerGlow);
    DrawText(titleStr.c_str(), titleX + 1, titleY + 1, fontSize, innerGlow);

    Color coreColor = (tubeBrightness < 0.8f)
        ? Color{ 225, 185, 75, 235 }
    : Color{ 255, 226, 110, 255 };
    DrawText(titleStr.c_str(), titleX, titleY, fontSize, coreColor);

    float centerX = titleX + textWidth * 0.5f;
    float lineY = (float)(titleY + fontSize + 14);
    float halfSpan = textWidth * 0.44f;

    Color lineColor = ColorAlpha(GOLD, 0.45f * glowAlpha);
    DrawLineEx({ centerX - halfSpan, lineY }, { centerX - 14.0f, lineY }, 1.5f, lineColor);
    DrawLineEx({ centerX + 14.0f, lineY }, { centerX + halfSpan, lineY }, 1.5f, lineColor);
    DrawPoly({ centerX, lineY }, 4, 5.5f, 0.0f, ColorAlpha(GOLD, 0.85f * glowAlpha));
}

void MainMenuState::DrawButton(const MenuButton& btn) {
    float p = btn.hoverProgress;

    float expandX = p * 14.0f;
    float expandY = p * 2.0f;

    float jitterX = 0.0f;
    float jitterY = 0.0f;
    if (btn.isDanger && p > 0.05f) {
        jitterX = sinf(shaderTime * 58.0f) * 1.4f * p;
        jitterY = cosf(shaderTime * 47.0f) * 0.9f * p;
    }

    Rectangle rect = {
        btn.bounds.x - expandX + jitterX,
        btn.bounds.y - expandY + jitterY,
        btn.bounds.width + expandX * 2.0f,
        btn.bounds.height + expandY * 2.0f
    };

    Color accentColor = btn.isDanger ? Color{ 225, 48, 56, 255 } : GOLD;
    Color darkFillBase = btn.isDanger ? Color{ 16, 10, 12, 215 } : Color{ 12, 13, 16, 215 };
    Color hoverTint = btn.isDanger ? ColorAlpha(MAROON, 0.36f * p) : ColorAlpha(GOLD, 0.16f * p);

    DrawRectangle((int)rect.x + 4, (int)rect.y + 5, (int)rect.width, (int)rect.height, Fade(BLACK, 0.65f));
    if (p > 0.01f) {
        Rectangle glowRect = { rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
        DrawRectangleLinesEx(glowRect, 2.0f, ColorAlpha(accentColor, 0.25f * p));
    }

    DrawRectangleRec(rect, darkFillBase);
    if (p > 0.01f) {
        DrawRectangleRec(rect, hoverTint);
    }

    if (p > 0.02f) {
        BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
        float sweepPhase = fmodf(shaderTime * 1.35f, 1.8f) - 0.4f;
        float sweepX = rect.x + sweepPhase * rect.width;
        DrawLineEx(
            { sweepX - 18.0f, rect.y + rect.height },
            { sweepX + 18.0f, rect.y },
            14.0f,
            ColorAlpha(accentColor, 0.14f * p)
        );
        EndScissorMode();
    }

    Color outerBorder = ColorLerp(Color{ 95, 95, 102, 200 }, accentColor, p);
    Color innerBorder = ColorAlpha(accentColor, 0.18f + 0.45f * p);

    DrawRectangleLinesEx(rect, 2.0f, outerBorder);
    Rectangle innerRect = { rect.x + 5.0f, rect.y + 5.0f, rect.width - 10.0f, rect.height - 10.0f };
    DrawRectangleLinesEx(innerRect, 1.0f, innerBorder);

    float cornerR = 3.0f + p * 1.0f;
    Color cornerCol = ColorLerp(GRAY, accentColor, p);
    DrawPoly({ innerRect.x, innerRect.y }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x + innerRect.width, innerRect.y }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x, innerRect.y + innerRect.height }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x + innerRect.width, innerRect.y + innerRect.height }, 4, cornerR, 0.0f, cornerCol);

    int fontSize = 25;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = rect.x + (rect.width - textWidth) * 0.5f;
    float textY = rect.y + (rect.height - fontSize) * 0.5f;

    Color textColor = ColorLerp(LIGHTGRAY, WHITE, p);
    DrawText(btn.text.c_str(), (int)textX + 1, (int)textY + 2, fontSize, Fade(BLACK, 0.8f));
    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);

    if (p > 0.05f) {
        float diamondOffset = 18.0f + p * 10.0f;
        float diamondRadius = 4.5f * p;
        float midY = rect.y + rect.height * 0.5f;
        Color sideDiamondCol = ColorAlpha(accentColor, p);

        DrawPoly({ textX - diamondOffset, midY }, 4, diamondRadius, 0.0f, sideDiamondCol);
        DrawPoly({ textX + textWidth + diamondOffset, midY }, 4, diamondRadius, 0.0f, sideDiamondCol);
    }
}

void MainMenuState::DrawCctvOverlay(float screenW, float screenH, float transitionProgress) {
    float margin = 24.0f;
    float armLen = 28.0f;
    float thick = 1.5f;
    Color hudColor = ColorAlpha(Color{ 170, 195, 178, 255 }, 0.32f * (1.0f - transitionProgress));

    DrawLineEx({ margin, margin }, { margin + armLen, margin }, thick, hudColor);
    DrawLineEx({ margin, margin }, { margin, margin + armLen }, thick, hudColor);

    DrawLineEx({ screenW - margin - armLen, margin }, { screenW - margin, margin }, thick, hudColor);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + armLen }, thick, hudColor);

    DrawLineEx({ margin, screenH - margin }, { margin + armLen, screenH - margin }, thick, hudColor);
    DrawLineEx({ margin, screenH - margin - armLen }, { margin, screenH - margin }, thick, hudColor);

    DrawLineEx({ screenW - margin - armLen, screenH - margin }, { screenW - margin, screenH - margin }, thick, hudColor);
    DrawLineEx({ screenW - margin, screenH - margin - armLen }, { screenW - margin, screenH - margin }, thick, hudColor);

    float recAlpha = 1.0f - transitionProgress;
    if (recAlpha > 0.05f) {
        bool recBlink = (fmodf(shaderTime, 1.4f) < 0.85f);
        if (recBlink) {
            DrawCircle((int)(margin + 18.0f), (int)(margin + 18.0f), 4.5f, ColorAlpha(RED, 0.85f * recAlpha));
        }
        DrawText("REC", (int)(margin + 30.0f), (int)(margin + 11.0f), 15, ColorAlpha(RAYWHITE, 0.65f * recAlpha));
        DrawText("CAM-04 // CASINO FLOOR", (int)(margin + 12.0f), (int)(margin + 32.0f), 13, ColorAlpha(LIGHTGRAY, 0.42f * recAlpha));

        const char* statusText = isTransitioning ? "SIGNAL: OVERRIDE..." : "SIGNAL: UNSTABLE";
        Color statusColor = isTransitioning ? ColorAlpha(RED, 0.75f * recAlpha) : ColorAlpha(LIGHTGRAY, 0.38f * recAlpha);
        int statusW = MeasureText(statusText, 13);
        DrawText(statusText, (int)(screenW - margin - statusW - 12.0f), (int)(screenH - margin - 24.0f), 13, statusColor);
    }
}