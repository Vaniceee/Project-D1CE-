#include "GameplayState.h"
#include "MainMenuState.h"
#include "GameOverState.h"
#include "GameManager.h"
#include "InputManager.h"

void GameplayState::Init() {
    targetTexture = LoadRenderTexture(virtualWidth, virtualHeight);

    // === 1. ІНІЦІАЛІЗАЦІЯ ШЕЙДЕРА ОСВІТЛЕННЯ ТА СТАНУ ===
    lightShader = LoadShader(0, Config::PATH_SHADER_LIGHTING);

    playerPosLoc = GetShaderLocation(lightShader, "playerScreenPos");
    resolutionLoc = GetShaderLocation(lightShader, "resolution");
    radiusLoc = GetShaderLocation(lightShader, "lightRadius");
    flickerLoc = GetShaderLocation(lightShader, "flicker");
    ambientLoc = GetShaderLocation(lightShader, "ambientColor");
    dangerLoc = GetShaderLocation(lightShader, "dangerFactor");
    exhaustionLoc = GetShaderLocation(lightShader, "exhaustionFactor");
    panicLoc = GetShaderLocation(lightShader, "panicFactor");
    timeLoc = GetShaderLocation(lightShader, "time");

    float res[2] = { (float)virtualWidth, (float)virtualHeight };
    SetShaderValue(lightShader, resolutionLoc, res, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightShader, radiusLoc, &lightRadius, SHADER_UNIFORM_FLOAT);

    float ambient[4] = { 0.004f, 0.004f, 0.006f, 1.0f };
    SetShaderValue(lightShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

    float zeroVal = 0.0f;
    SetShaderValue(lightShader, dangerLoc, &zeroVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, exhaustionLoc, &zeroVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, panicLoc, &zeroVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, timeLoc, &zeroVal, SHADER_UNIFORM_FLOAT);
    // ====================================================

    AssetManager& am = AssetManager::Instance();

    am.LoadTexture("player_spritesheet", Config::PATH_PNG_PLAYER_SPRITESHEET);
    am.LoadTexture("floor", Config::PATH_PNG_FLOOR);
    am.LoadTexture("rugs", Config::PATH_PNG_RUGS);
    am.LoadTexture("walls", Config::PATH_PNG_WALLS_1);
    am.LoadTexture("walls1", Config::PATH_PNG_WALLS_2);

    for (int i = 0; i <= 10; ++i) {
        std::string name = "stamina_" + std::to_string(i);
        std::string path = "assets/stamina/bar" + std::to_string(i) + ".png";
        am.LoadTexture(name, path);
    }

    am.LoadSound("step_wood_1", Config::PATH_STEP_WOOD_1);
    am.LoadSound("step_wood_2", Config::PATH_STEP_WOOD_2);
    am.LoadSound("step_wood_3", Config::PATH_STEP_WOOD_3);
    am.LoadSound("step_wood_4", Config::PATH_STEP_WOOD_4);
    am.LoadSound("Breath", "assets/player/Breath.wav");
    am.LoadSound("card_rip", Config::PATH_SFX_CARD_RIP);

    am.LoadTexture("card_lives", Config::PATH_PNG_CARD_LIVES);

    am.LoadMusic("guard_chase", Config::PATH_AUDIO_GUARD_CHASE);

    gameMap.Load(Config::PATH_MAP);

    // 1. Спавн гравця з мапи
    Vector2 startPos = gameMap.GetPlayerSpawn();
    player = std::make_unique<Player>(startPos, 112.0f, 32.0f, 64.0f, am.GetTexture("player_spritesheet"), 1.45f);

    Rectangle startFoot = player->GetFootCollider();
    lightWorldPos = {
        startFoot.x + startFoot.width * 0.5f,
        startFoot.y + startFoot.height * 0.5f
    };
    lightVelocity = { 0.0f, 0.0f };
    isLightPosInitialized = true;

    // 2. Спавн охоронця з мапи
    const auto& guardSpawns = gameMap.GetGuardSpawns();
    Vector2 guardPos = !guardSpawns.empty() ? guardSpawns[0] : Vector2{ 2200.0f, 2464.0f };
    guard = std::make_unique<Guard>(guardPos, 70.0f);
    guard->InitPathfinding(gameMap);

    // 3. Ініціалізація камери
    gameCamera.Init(startPos, virtualWidth, virtualHeight, 1.8f);

    // 4. Ініціалізація кнопок меню паузи
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float btnWidth = 300.0f;
    float btnHeight = 60.0f;
    float btnX = (screenW - btnWidth) * 0.5f;

    resumeButton.bounds = { btnX, (screenH * 0.5f) - 40.0f, btnWidth, btnHeight };
    resumeButton.text = "RESUME";

    exitToMenuButton.bounds = { btnX, (screenH * 0.5f) + 40.0f, btnWidth, btnHeight };
    exitToMenuButton.text = "MAIN MENU";

    // Скидання станів
    isPaused = false;
    isDying = false;
    deathFadeAlpha = 0.0f;
    tpPhase = TeleportPhase::NONE;
    tpFadeAlpha = 0.0f;
    waitForKeysRelease = false;
    wasInTrigger = false;
    smoothedNoise = 0.0f;
    smoothedDanger = 0.0f;
    smoothedExhaustion = 0.0f;
    smoothedPanic = 0.0f;
    damageImpulse = 0.0f;
    shaderTime = 0.0f;
    lightDipTimer = 0.0f;
    lightDipDepth = 0.0f;

    //======================MUSIC======================
    AssetManager::Instance().LoadMusic("ambient", Config::PATH_AUDIO_AMBIENT);
    bgMusic = AssetManager::Instance().GetMusic("ambient");

    SetMusicVolume(bgMusic, 0.046f);
    bgMusic.looping = true;

    PlayMusicStream(bgMusic);
    //=================================================
}

void GameplayState::Cleanup() {
    StopMusicStream(bgMusic);
    if (guard) {
        guard->StopAudio();
    }
    gameMap.Unload();

    if (lightShader.id != 0) {
        UnloadShader(lightShader);
    }

    if (targetTexture.id != 0) {
        UnloadRenderTexture(targetTexture);
    }

    player.reset();
    guard.reset();

    AssetManager::Instance().UnloadAll();
}

void GameplayState::UpdatePauseMenu() {
    Vector2 mousePos = GetMousePosition();

    resumeButton.isHovered = CheckCollisionPointRec(mousePos, resumeButton.bounds);
    exitToMenuButton.isHovered = CheckCollisionPointRec(mousePos, exitToMenuButton.bounds);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (resumeButton.isHovered) {
            isPaused = false;
            ResumeMusicStream(bgMusic);
            if (guard) guard->SetAudioPaused(false);
        }
        else if (exitToMenuButton.isHovered) {
            GameManager::Instance().ChangeState(new MainMenuState());
            return;
        }
    }
}

void GameplayState::Update() {
    float dt = GetFrameTime();

    if (isDying) {
        UpdateMusicStream(bgMusic);
        deathFadeAlpha += deathFadeSpeed * dt;
        SetMusicVolume(bgMusic, 0.046f * (1.0f - (deathFadeAlpha > 1.0f ? 1.0f : deathFadeAlpha)));

        if (deathFadeAlpha >= 1.0f) {
            GameManager::Instance().ChangeState(new GameOverState());
            return;
        }
        return;
    }

    if (InputManager::IsPausePressed() && tpPhase == TeleportPhase::NONE) {
        isPaused = !isPaused;
        if (isPaused) {
            PauseMusicStream(bgMusic);
            if (guard) guard->SetAudioPaused(true);
        }
        else {
            ResumeMusicStream(bgMusic);
            if (guard) guard->SetAudioPaused(false);
        }
    }

    if (isPaused) {
        UpdatePauseMenu();
        return;
    }

    if (InputManager::IsDebugTogglePressed()) {
        showDebugOverlay = !showDebugOverlay;
    }

    if (player) {
        UpdateMusicStream(bgMusic);
        shaderTime += dt;

        Vector2 currentInputDir = InputManager::GetMovementDirection();
        bool anyMoveKeyPressed = (currentInputDir.x != 0.0f || currentInputDir.y != 0.0f);

        if (waitForKeysRelease && !anyMoveKeyPressed) {
            waitForKeysRelease = false;
        }

        if (tpPhase == TeleportPhase::FADING_OUT) {
            tpFadeAlpha += tpFadeSpeed * dt;
            if (tpFadeAlpha >= 1.0f) {
                tpFadeAlpha = 1.0f;
                player->SetPosition(pendingTeleportPos);

                Rectangle newFoot = player->GetFootCollider();
                lightWorldPos = {
                    newFoot.x + newFoot.width * 0.5f,
                    newFoot.y + newFoot.height * 0.5f
                };
                lightVelocity = { 0.0f, 0.0f };

                tpPhase = TeleportPhase::FADING_IN;
            }
        }
        else if (tpPhase == TeleportPhase::FADING_IN) {
            tpFadeAlpha -= tpFadeSpeed * dt;
            if (tpFadeAlpha <= 0.0f) {
                tpFadeAlpha = 0.0f;
                tpPhase = TeleportPhase::NONE;
                waitForKeysRelease = true;
            }
        }

        bool isMovementBlocked = (tpPhase != TeleportPhase::NONE) || waitForKeysRelease;
        if (!isMovementBlocked) {
            player->Update(gameMap);
        }

        int currentLives = player->GetLives();
        if (currentLives < lastPlayerLives) {
            cardFlickerTimer = cardFlickerDuration;
            lastPlayerLives = currentLives;
            damageImpulse = 1.0f;
            gameCamera.AddTrauma(0.55f);
        }

        if (cardFlickerTimer > 0.0f) {
            cardFlickerTimer -= dt;
            if (cardFlickerTimer < 0.0f) cardFlickerTimer = 0.0f;
        }

        if (damageImpulse > 0.0f) {
            damageImpulse -= dt * 1.4f;
            if (damageImpulse < 0.0f) damageImpulse = 0.0f;
        }

        if (guard) {
            guard->Update(*player, gameMap, dt);
        }

        if (player->GetLives() <= 0 && !isDying) {
            TraceLog(LOG_WARNING, "GAME OVER! Player died.");
            isDying = true;
            deathFadeAlpha = 0.0f;
            if (guard) guard->StopAudio();
            gameCamera.AddTrauma(0.9f);
            return;
        }

        Rectangle footCollider = player->GetFootCollider();

        gameMap.UpdateTriggers(footCollider);
        bool isInTriggerNow = !gameMap.GetCurrentTriggerMessage().empty();

        if (tpPhase == TeleportPhase::NONE) {
            if (isInTriggerNow && !wasInTrigger) {
                Vector2 newPlayerPos;
                if (gameMap.TryGetTeleportDestination(footCollider, player->GetPosition(), newPlayerPos)) {
                    Vector2 enteredTriggerCenter;
                    if (guard && gameMap.TryGetActiveTriggerCenter(footCollider, enteredTriggerCenter)) {
                        guard->OnPlayerTeleported(enteredTriggerCenter);
                    }

                    pendingTeleportPos = newPlayerPos;
                    tpPhase = TeleportPhase::FADING_OUT;
                    wasInTrigger = true;
                }
            }
            else if (!isInTriggerNow) {
                wasInTrigger = false;
            }
        }

        Vector2 desiredTarget = {
            footCollider.x + footCollider.width / 2.0f,
            footCollider.y + footCollider.height / 2.0f
        };

        // === МІКРО-ІНЕРЦІЯ ДЖЕРЕЛА СВІТЛА ===
        if (!isLightPosInitialized) {
            lightWorldPos = desiredTarget;
            isLightPosInitialized = true;
        }
        else {
            const float stiffness = 175.0f;
            const float damping = 18.0f;

            Vector2 displacement = Vector2Subtract(desiredTarget, lightWorldPos);
            Vector2 accel = Vector2Subtract(
                Vector2Scale(displacement, stiffness),
                Vector2Scale(lightVelocity, damping)
            );

            lightVelocity = Vector2Add(lightVelocity, Vector2Scale(accel, dt));
            lightWorldPos = Vector2Add(lightWorldPos, Vector2Scale(lightVelocity, dt));

            Vector2 offsetFromPlayer = Vector2Subtract(lightWorldPos, desiredTarget);
            float offsetLen = Vector2Length(offsetFromPlayer);
            const float maxLagDist = 7.0f;
            if (offsetLen > maxLagDist) {
                lightWorldPos = Vector2Add(desiredTarget, Vector2Scale(offsetFromPlayer, maxLagDist / offsetLen));
            }
        }
        // ===================================

        float proximity = 0.0f;
        bool isChasing = false;
        if (guard) {
            Rectangle guardFoot = guard->GetFootCollider();
            Vector2 guardCenter = {
                guardFoot.x + guardFoot.width * 0.5f,
                guardFoot.y + guardFoot.height * 0.5f
            };
            float distToGuard = Vector2Distance(desiredTarget, guardCenter);

            const float minDist = 42.0f;
            const float maxDist = 340.0f;
            float normalizedDist = Clamp((distToGuard - minDist) / (maxDist - minDist), 0.0f, 1.0f);
            float linearProx = 1.0f - normalizedDist;

            proximity = linearProx * linearProx;
            isChasing = (guard->GetCurrentState() == GuardState::CHASE);

            if (isChasing) {
                proximity = Clamp(proximity * 1.15f, 0.0f, 1.0f);
            }
        }

        if (proximity > 0.05f) {
            float proximityTrauma = Lerp(0.10f, 0.36f, proximity);
            gameCamera.SetMinTrauma(proximityTrauma);
        }

        float staminaRatio = player->GetCurrentStamina() / player->GetMaxStamina();
        bool effectiveMoving = !isMovementBlocked && player->IsMoving();
        gameCamera.Update(desiredTarget, effectiveMoving, player->IsSprinting(), player->IsExhausted(), staminaRatio, dt);

        float flickerFreq = Lerp(8.5f, 22.0f, proximity);
        float waveAmp = Lerp(0.010f, 0.065f, proximity);
        float noiseAmp = Lerp(0.004f, 0.052f, proximity);

        flickerTimer += dt * flickerFreq;
        float rawNoise = ((float)GetRandomValue(-100, 100) / 100.0f) * noiseAmp;
        float noiseBlend = 1.0f - expf(-25.0f * dt);
        smoothedNoise = Lerp(smoothedNoise, rawNoise, noiseBlend);

        if (lightDipTimer > 0.0f) {
            lightDipTimer -= dt;
            if (lightDipTimer < 0.0f) lightDipTimer = 0.0f;
        }
        else if (proximity > 0.25f) {
            float triggerChancePerSec = proximity * 3.2f;
            if (((float)GetRandomValue(0, 10000) / 10000.0f) < triggerChancePerSec * dt) {
                lightDipDuration = (float)GetRandomValue(13, 19) / 100.0f;
                lightDipTimer = lightDipDuration;
                lightDipDepth = ((float)GetRandomValue(7, 15) / 100.0f) * proximity;
            }
        }

        float activeDip = 0.0f;
        if (lightDipTimer > 0.0f && lightDipDuration > 0.0f) {
            float dipProgress = 1.0f - (lightDipTimer / lightDipDuration);
            activeDip = sinf(dipProgress * PI) * lightDipDepth;
        }

        currentFlicker = 1.0f + (sinf(flickerTimer) * waveAmp) + smoothedNoise - activeDip;
        currentFlicker = Clamp(currentFlicker, 0.65f, 1.08f);
        SetShaderValue(lightShader, flickerLoc, &currentFlicker, SHADER_UNIFORM_FLOAT);

        float dangerBlend = 1.0f - expf(-6.0f * dt);
        smoothedDanger = Lerp(smoothedDanger, proximity, dangerBlend);
        SetShaderValue(lightShader, dangerLoc, &smoothedDanger, SHADER_UNIFORM_FLOAT);

        // === РОЗРАХУНОК ВИСНАЖЕННЯ ТА ПАНІКИ ДЛЯ ПОСТ-ПРОЦЕСУ ===
        float targetExhaustion = 0.0f;
        if (staminaRatio < 0.40f) {
            targetExhaustion = Clamp((0.40f - staminaRatio) / 0.40f, 0.0f, 1.0f);
        }
        if (player->IsExhausted()) {
            targetExhaustion = fmaxf(targetExhaustion, 0.78f);
        }
        float exhaustBlend = 1.0f - expf(-4.5f * dt);
        smoothedExhaustion = Lerp(smoothedExhaustion, targetExhaustion, exhaustBlend);

        float targetPanic = Clamp(smoothedDanger * (isChasing ? 1.15f : 0.75f) + damageImpulse * 0.85f, 0.0f, 1.0f);
        float panicBlend = 1.0f - expf(-5.5f * dt);
        smoothedPanic = Lerp(smoothedPanic, targetPanic, panicBlend);

        SetShaderValue(lightShader, exhaustionLoc, &smoothedExhaustion, SHADER_UNIFORM_FLOAT);
        SetShaderValue(lightShader, panicLoc, &smoothedPanic, SHADER_UNIFORM_FLOAT);
        SetShaderValue(lightShader, timeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);
        // ==========================================================

        Vector2 lightScreenPos = gameCamera.GetScreenPosition(lightWorldPos);

        float pos[2] = {
            lightScreenPos.x,
            (float)virtualHeight - lightScreenPos.y
        };

        SetShaderValue(lightShader, playerPosLoc, pos, SHADER_UNIFORM_VEC2);
    }
}

void GameplayState::Draw() {
    BeginTextureMode(targetTexture);
    ClearBackground(BLACK);

    BeginMode2D(gameCamera.GetCamera());
    renderSystem.RenderWorld(gameMap, player.get(), guard.get());
    if (showDebugOverlay) {
        gameMap.DrawDebug();
        if (player) player->DrawDebug();
        if (guard) guard->DrawDebug();
    }
    EndMode2D();

    EndTextureMode();

    float windowWidth = (float)GetScreenWidth();
    float windowHeight = (float)GetScreenHeight();

    float scaleX = windowWidth / (float)virtualWidth;
    float scaleY = windowHeight / (float)virtualHeight;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float drawWidth = (float)virtualWidth * scale;
    float drawHeight = (float)virtualHeight * scale;

    float destX = (windowWidth - drawWidth) * 0.5f;
    float destY = (windowHeight - drawHeight) * 0.5f;

    ClearBackground(BLACK);

    Rectangle sourceRec = { 0.0f, 0.0f, (float)targetTexture.texture.width, (float)-targetTexture.texture.height };
    Rectangle destRec = { destX, destY, drawWidth, drawHeight };
    Vector2 origin = { 0.0f, 0.0f };

    BeginShaderMode(lightShader);
    DrawTexturePro(targetTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
    EndShaderMode();

    if (player) {
        float staminaRatio = player->GetCurrentStamina() / player->GetMaxStamina();
        if (staminaRatio < 0.0f) staminaRatio = 0.0f;
        if (staminaRatio > 1.0f) staminaRatio = 1.0f;

        int staminaFrame = (int)std::round(staminaRatio * 10.0f);
        std::string staminaTexName = "stamina_" + std::to_string(staminaFrame);
        Texture2D staminaTex = AssetManager::Instance().GetTexture(staminaTexName);

        if (staminaTex.id > 0) {
            Vector2 position = { 20.0f, 400.0f };
            DrawTextureEx(staminaTex, position, 270.0f, 2.5f, WHITE);
        }

        Texture2D cardTex = AssetManager::Instance().GetTexture("card_lives");
        if (cardTex.id > 0) {
            const float frameWidth = 32.0f;
            const float frameHeight = 32.0f;

            int frameIndex = 0;
            if (player->GetLives() == 2) frameIndex = 1;
            else if (player->GetLives() == 1) frameIndex = 2;
            else if (player->GetLives() <= 0) frameIndex = 3;

            Rectangle cardSourceRec = { (float)frameIndex * frameWidth, 0.0f, frameWidth, frameHeight };
            Vector2 cardPos = { 55.0f, 105.0f };
            float cardScale = 4.5f;

            Color cardColor = WHITE;
            Vector2 shakeOffset = { 0.0f, 0.0f };

            if (cardFlickerTimer > 0.0f) {
                float alpha = (sinf(cardFlickerTimer * 40.0f) > 0.0f) ? 0.3f : 1.0f;
                cardColor = ColorAlpha(RED, alpha);
                shakeOffset.x = (float)GetRandomValue(-3, 3);
                shakeOffset.y = (float)GetRandomValue(-3, 3);
            }

            Rectangle cardDestRec = {
                cardPos.x + shakeOffset.x,
                cardPos.y + shakeOffset.y,
                frameWidth * cardScale,
                frameHeight * cardScale
            };

            DrawTexturePro(cardTex, cardSourceRec, cardDestRec, Vector2{ 0.0f, 0.0f }, 0.0f, cardColor);
        }

        if (showDebugOverlay) {
            Rectangle foot = player->GetFootCollider();
            DrawRectangle(10, 10, 350, 80, Fade(BLACK, 0.8f));
            DrawRectangleLines(10, 10, 350, 80, GREEN);

            DrawFPS(20, 20);
            DrawText(TextFormat("Player Pos: X: %.1f | Y: %.1f", foot.x, foot.y), 20, 45, 18, GREEN);
            DrawText(TextFormat("Tile Coords: X: %d | Y: %d", (int)(foot.x / 32.0f), (int)(foot.y / 32.0f)), 20, 65, 18, YELLOW);
        }
    }

    if (tpFadeAlpha > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, tpFadeAlpha));
    }

    if (isDying) {
        float clampedAlpha = (deathFadeAlpha > 1.0f) ? 1.0f : deathFadeAlpha;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(MAROON, clampedAlpha * 0.45f));
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, clampedAlpha));
    }

    if (isPaused) {
        DrawPauseMenu();
    }
}

void GameplayState::DrawPauseMenu() {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 0.72f));

    const char* pauseTitle = "PAUSED";
    int fontSize = 54;
    int textWidth = MeasureText(pauseTitle, fontSize);
    DrawText(pauseTitle, (int)((screenW - textWidth) * 0.5f), (int)(screenH * 0.28f), fontSize, GOLD);

    DrawPauseButton(resumeButton);
    DrawPauseButton(exitToMenuButton);
}

void GameplayState::DrawPauseButton(const PauseButton& btn) {
    Color rectColor = btn.isHovered ? Fade(GOLD, 0.3f) : Fade(DARKGRAY, 0.6f);
    Color borderColor = btn.isHovered ? GOLD : GRAY;
    Color textColor = btn.isHovered ? WHITE : LIGHTGRAY;

    DrawRectangleRec(btn.bounds, rectColor);
    DrawRectangleLinesEx(btn.bounds, 2.0f, borderColor);

    int fontSize = 24;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = btn.bounds.x + (btn.bounds.width - textWidth) * 0.5f;
    float textY = btn.bounds.y + (btn.bounds.height - fontSize) * 0.5f;

    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);
}