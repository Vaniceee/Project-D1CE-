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

    menuUI.DrawNeonTitle(screenW, screenH, shaderTime, smoothedMouseOffset);
    menuUI.DrawButton(playButton, shaderTime);
    menuUI.DrawButton(exitButton, shaderTime);

    float progress = isTransitioning ? Clamp(transitionTimer / transitionDuration, 0.0f, 1.0f) : 0.0f;
    menuUI.DrawCctvOverlay(screenW, screenH, shaderTime, progress, isTransitioning);

    if (isTransitioning) {
        menuUI.DrawTransitionDoors(screenW, screenH, progress);
    }
    else {
        CustomCursor::Draw(exitButton.isHovered);
    }
}