#include "MainMenuState.h"
#include "GameplayState.h"
#include "GameManager.h"
#include "Logger.h"
#include "raymath.h"
#include <cmath>

void MainMenuState::Init() {
    AssetManager& am = AssetManager::Instance();

    // 1. Çàâàíòàæåííÿ ôîíó
    am.LoadTexture("MainMenuBG.jpg", Config::PATH_PNG_MENU_BG);
    bgTexture = am.GetTexture("MainMenuBG.jpg");
    hasBackground = (bgTexture.id > 0);

    // 2. ²í³ö³àë³çàö³ÿ øåéäåðà ìåíþ
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

    // 3. Çàâàíòàæåííÿ ôîíîâî¿ ìóçèêè
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

    // 4. Ñêèäàííÿ ñòàíó ïåðåõîäó
    isTransitioning = false;
    transitionTimer = 0.0f;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float btnWidth = 280.0f;
    float btnHeight = 60.0f;
    float btnX = (screenW - btnWidth) / 2.0f;

    playButton.bounds = { btnX, (screenH / 2.0f) - 40.0f, btnWidth, btnHeight };
    playButton.text = "PLAY";

    exitButton.bounds = { btnX, (screenH / 2.0f) + 40.0f, btnWidth, btnHeight };
    exitButton.text = "EXIT";
}

void MainMenuState::Cleanup() {
    if (hasMusic) {
        StopMusicStream(bgMusic);
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

    // Ïëàâíèé ðîçðàõóíîê ïîçèö³¿ êóðñîðà â³äíîñíî öåíòðó åêðàíà (-1.0 .. 1.0)
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
        playButton.isHovered = CheckCollisionPointRec(mousePos, playButton.bounds);
        exitButton.isHovered = CheckCollisionPointRec(mousePos, exitButton.bounds);

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

    // 1. ÌÀËÞÂÀÍÍß ÔÎÍÓ Ç ØÅÉÄÅÐÎÌ ÄÈÌÓ, ÃË²Ò×²Â ÒÀ ÏÀÐÀËÀÊÑÓ
    if (hasBackground) {
        if (menuShader.id != 0) {
            BeginShaderMode(menuShader);
        }

        DrawTexturePro(
            bgTexture,
            Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
            Rectangle{ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );

        if (menuShader.id != 0) {
            EndShaderMode();
        }
    }

    // 2. ÇÀÃÎËÎÂÎÊ ÃÐÈ (ç ì³êðî-çì³ùåííÿì ó ïðîòèôàç³ äëÿ îá'ºìó)
    const char* titleText = "_PR0JECT_D1CE";
    int fontSize = 60;
    int textWidth = MeasureText(titleText, fontSize);
    int titleX = (GetScreenWidth() - textWidth) / 2 - (int)(smoothedMouseOffset.x * 5.0f);
    int titleY = GetScreenHeight() / 4 - (int)(smoothedMouseOffset.y * 3.5f);
    DrawText(titleText, titleX, titleY, fontSize, GOLD);

    // 3. ÌÀËÞÂÀÍÍß ÊÍÎÏÎÊ
    DrawButton(playButton);
    DrawButton(exitButton);

    // 4. ÏÅÐÅÕ²Ä Ó ÃÐÓ
    if (isTransitioning) {
        float progress = transitionTimer / transitionDuration;

        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();
        Vector2 center = { screenW * 0.5f, screenH * 0.5f };

        float maxRadius = sqrtf(screenW * screenW + screenH * screenH) * 0.5f;
        float innerRadius = maxRadius * (1.0f - progress);
        float outerRadius = maxRadius * 1.5f;
        DrawRing(center, innerRadius, outerRadius, 0.0f, 360.0f, 64, BLACK);
        if (progress > 0.2f && innerRadius > 2.0f) {
            DrawCircleLinesV(center, innerRadius, ColorAlpha(RED, 0.4f * progress));
        }
    }
}

void MainMenuState::DrawButton(const MenuButton& btn) {
    Color rectColor = btn.isHovered ? Fade(GOLD, 0.3f) : Fade(DARKGRAY, 0.6f);
    Color borderColor = btn.isHovered ? GOLD : GRAY;
    Color textColor = btn.isHovered ? WHITE : LIGHTGRAY;

    DrawRectangleRec(btn.bounds, rectColor);
    DrawRectangleLinesEx(btn.bounds, 2.0f, borderColor);

    int fontSize = 24;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = btn.bounds.x + (btn.bounds.width - textWidth) / 2.0f;
    float textY = btn.bounds.y + (btn.bounds.height - fontSize) / 2.0f;

    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);
}