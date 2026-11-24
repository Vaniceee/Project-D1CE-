#include "MainMenuState.h"
#include "GameplayState.h"
#include "CutsceneState.h"
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

    // 5. Скидання стану переходу, правил та кнопок
    isTransitioning = false;
    transitionTimer = 0.0f;
    isRulesOpen = false;
    rulesOpenAnim = 0.0f;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float btnWidth = 310.0f;
    float btnHeight = 56.0f;
    float btnX = (screenW - btnWidth) * 0.5f;
    float startY = (screenH * 0.5f) - 62.0f;
    float gapY = 74.0f;

    playButton.bounds = { btnX, startY, btnWidth, btnHeight };
    playButton.text = "PLAY";
    playButton.isHovered = false;
    playButton.wasHovered = false;
    playButton.hoverProgress = 0.0f;
    playButton.isDanger = false;

    rulesButton.bounds = { btnX, startY + gapY, btnWidth, btnHeight };
    rulesButton.text = "HOUSE RULES";
    rulesButton.isHovered = false;
    rulesButton.wasHovered = false;
    rulesButton.hoverProgress = 0.0f;
    rulesButton.isDanger = false;

    exitButton.bounds = { btnX, startY + gapY * 2.0f, btnWidth, btnHeight };
    exitButton.text = "EXIT";
    exitButton.isHovered = false;
    exitButton.wasHovered = false;
    exitButton.hoverProgress = 0.0f;
    exitButton.isDanger = true;

    float closeW = 240.0f;
    float closeH = 48.0f;
    float modalH = 540.0f;
    float modalY = (screenH - modalH) * 0.5f;
    closeRulesButton.bounds = { (screenW - closeW) * 0.5f, modalY + modalH - 72.0f, closeW, closeH };
    closeRulesButton.text = "CLOSE";
    closeRulesButton.isHovered = false;
    closeRulesButton.wasHovered = false;
    closeRulesButton.hoverProgress = 0.0f;
    closeRulesButton.isDanger = false;
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
    if (!isTransitioning && !isRulesOpen && screenW > 1.0f && screenH > 1.0f) {
        targetMouseOffset.x = Clamp((mousePos.x / screenW) * 2.0f - 1.0f, -1.0f, 1.0f);
        targetMouseOffset.y = Clamp((mousePos.y / screenH) * 2.0f - 1.0f, -1.0f, 1.0f);
    }
    float mouseBlend = 1.0f - expf(-6.5f * deltaTime);
    smoothedMouseOffset.x = Lerp(smoothedMouseOffset.x, targetMouseOffset.x, mouseBlend);
    smoothedMouseOffset.y = Lerp(smoothedMouseOffset.y, targetMouseOffset.y, mouseBlend);

    float rulesBlend = 1.0f - expf(-14.0f * deltaTime);
    rulesOpenAnim = Lerp(rulesOpenAnim, isRulesOpen ? 1.0f : 0.0f, rulesBlend);

    float progress = 0.0f;
    float hoverAnimSpeed = 1.0f - expf(-12.0f * deltaTime);

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
            GameManager::Instance().ChangeState(new CutsceneState());
            return;
        }
    }
    else if (isRulesOpen) {
        playButton.isHovered = false;
        rulesButton.isHovered = false;
        exitButton.isHovered = false;

        closeRulesButton.wasHovered = closeRulesButton.isHovered;
        closeRulesButton.isHovered = CheckCollisionPointRec(mousePos, closeRulesButton.bounds);

        if (hasHoverSound && closeRulesButton.isHovered && !closeRulesButton.wasHovered) {
            SetSoundPitch(hoverSound, 1.08f);
            PlaySound(hoverSound);
        }

        Rectangle modalBounds = { (screenW - 920.0f) * 0.5f, (screenH - 540.0f) * 0.5f, 920.0f, 540.0f };
        bool clickedOutside = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(mousePos, modalBounds);

        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_H) || clickedOutside ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && closeRulesButton.isHovered)) {
            isRulesOpen = false;
            closeRulesButton.isHovered = false;
            if (hasHoverSound) {
                SetSoundPitch(hoverSound, 0.92f);
                PlaySound(hoverSound);
            }
        }
    }
    else {
        closeRulesButton.isHovered = false;

        playButton.wasHovered = playButton.isHovered;
        rulesButton.wasHovered = rulesButton.isHovered;
        exitButton.wasHovered = exitButton.isHovered;

        playButton.isHovered = CheckCollisionPointRec(mousePos, playButton.bounds);
        rulesButton.isHovered = CheckCollisionPointRec(mousePos, rulesButton.bounds);
        exitButton.isHovered = CheckCollisionPointRec(mousePos, exitButton.bounds);

        if (hasHoverSound) {
            if ((playButton.isHovered && !playButton.wasHovered) ||
                (rulesButton.isHovered && !rulesButton.wasHovered)) {
                SetSoundPitch(hoverSound, 1.08f);
                PlaySound(hoverSound);
            }
            else if (exitButton.isHovered && !exitButton.wasHovered) {
                SetSoundPitch(hoverSound, 0.86f);
                PlaySound(hoverSound);
            }
        }

        if (IsKeyPressed(KEY_H)) {
            isRulesOpen = true;
            if (hasHoverSound) {
                SetSoundPitch(hoverSound, 1.12f);
                PlaySound(hoverSound);
            }
        }
        else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (playButton.isHovered) {
                isTransitioning = true;
                transitionTimer = 0.0f;

                if (hasStartSound) {
                    PlaySound(startSound);
                }
            }
            else if (rulesButton.isHovered) {
                isRulesOpen = true;
                if (hasHoverSound) {
                    SetSoundPitch(hoverSound, 1.12f);
                    PlaySound(hoverSound);
                }
            }
            else if (exitButton.isHovered) {
                GameManager::Instance().Quit();
                return;
            }
        }
    }

    playButton.hoverProgress = Lerp(playButton.hoverProgress, (playButton.isHovered && !isTransitioning && !isRulesOpen) ? 1.0f : 0.0f, hoverAnimSpeed);
    rulesButton.hoverProgress = Lerp(rulesButton.hoverProgress, (rulesButton.isHovered && !isTransitioning && !isRulesOpen) ? 1.0f : 0.0f, hoverAnimSpeed);
    exitButton.hoverProgress = Lerp(exitButton.hoverProgress, (exitButton.isHovered && !isTransitioning && !isRulesOpen) ? 1.0f : 0.0f, hoverAnimSpeed);
    closeRulesButton.hoverProgress = Lerp(closeRulesButton.hoverProgress, (closeRulesButton.isHovered && isRulesOpen) ? 1.0f : 0.0f, hoverAnimSpeed);

    bool anyHovered = !isTransitioning && (isRulesOpen
        ? closeRulesButton.isHovered
        : (playButton.isHovered || rulesButton.isHovered || exitButton.isHovered));
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

    menuUI.DrawNeonTitle(screenW, screenH, shaderTime, smoothedMouseOffset);
    menuUI.DrawButton(playButton, shaderTime);
    menuUI.DrawButton(rulesButton, shaderTime);
    menuUI.DrawButton(exitButton, shaderTime);

    float progress = isTransitioning ? Clamp(transitionTimer / transitionDuration, 0.0f, 1.0f) : 0.0f;
    menuUI.DrawCctvOverlay(screenW, screenH, shaderTime, progress, isTransitioning);

    if (rulesOpenAnim > 0.01f) {
        menuUI.DrawRulesModal(screenW, screenH, rulesOpenAnim, shaderTime, closeRulesButton);
    }

    if (isTransitioning) {
        menuUI.DrawTransitionDoors(screenW, screenH, progress);
    }
    else {
        CustomCursor::Draw(!isRulesOpen && exitButton.isHovered);
    }
}