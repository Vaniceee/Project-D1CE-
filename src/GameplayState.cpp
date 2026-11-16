#include "GameplayState.h"
#include "MainMenuState.h"
#include "GameOverState.h"
#include "GameManager.h"
#include "InputManager.h"

void GameplayState::Init() {
    targetTexture = LoadRenderTexture(virtualWidth, virtualHeight);

    // === 1. ІНІЦІАЛІЗАЦІЯ ШЕЙДЕРА ОСВІТЛЕННЯ ===
    lightShader = LoadShader(0, Config::PATH_SHADER_LIGHTING);

    playerPosLoc = GetShaderLocation(lightShader, "playerScreenPos");
    resolutionLoc = GetShaderLocation(lightShader, "resolution");
    radiusLoc = GetShaderLocation(lightShader, "lightRadius");
    flickerLoc = GetShaderLocation(lightShader, "flicker");
    ambientLoc = GetShaderLocation(lightShader, "ambientColor");

    float res[2] = { (float)virtualWidth, (float)virtualHeight };
    SetShaderValue(lightShader, resolutionLoc, res, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightShader, radiusLoc, &lightRadius, SHADER_UNIFORM_FLOAT);

    float ambient[4] = { 0.04f, 0.04f, 0.08f, 1.0f };
    SetShaderValue(lightShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    // ============================================

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

    am.LoadTexture("card_lives", Config::PATH_PNG_CARD_LIVES);

    gameMap.Load(Config::PATH_MAP);

    // 1. Спавн гравця з мапи
    Vector2 startPos = gameMap.GetPlayerSpawn();
    player = std::make_unique<Player>(startPos, 112.0f, 32.0f, 64.0f, am.GetTexture("player_spritesheet"), 1.45f);

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
        }
        else if (exitToMenuButton.isHovered) {
            GameManager::Instance().ChangeState(new MainMenuState());
            return;
        }
    }
}

void GameplayState::Update() {
    float dt = GetFrameTime();

    // 1. Обробка смерті гравця та перехід у GameOverState
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

    // 2. Перемикання паузи по клавіші ESC (якщо не триває телепорт)
    if (InputManager::IsPausePressed() && tpPhase == TeleportPhase::NONE) {
        isPaused = !isPaused;
        if (isPaused) {
            PauseMusicStream(bgMusic);
        }
        else {
            ResumeMusicStream(bgMusic);
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

        Vector2 currentInputDir = InputManager::GetMovementDirection();
        bool anyMoveKeyPressed = (currentInputDir.x != 0.0f || currentInputDir.y != 0.0f);

        if (waitForKeysRelease && !anyMoveKeyPressed) {
            waitForKeysRelease = false;
        }

        // Обробка фаз переходу між локаціями
        if (tpPhase == TeleportPhase::FADING_OUT) {
            tpFadeAlpha += tpFadeSpeed * dt;
            if (tpFadeAlpha >= 1.0f) {
                tpFadeAlpha = 1.0f;
                player->SetPosition(pendingTeleportPos);
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

        // === ЛОГІКА АНІМАЦІЇ UI КАРТИ ЖИТТІВ ===
        int currentLives = player->GetLives();
        if (currentLives < lastPlayerLives) {
            cardFlickerTimer = cardFlickerDuration;
            lastPlayerLives = currentLives;
            gameCamera.AddTrauma(0.55f);
        }

        if (cardFlickerTimer > 0.0f) {
            cardFlickerTimer -= dt;
            if (cardFlickerTimer < 0.0f) cardFlickerTimer = 0.0f;
        }
        // ========================================

        if (guard) {
            guard->Update(*player, gameMap, dt);
        }

        // Перевірка смерті гравця (коли ХП <= 0)
        if (player->GetLives() <= 0 && !isDying) {
            TraceLog(LOG_WARNING, "GAME OVER! Player died.");
            isDying = true;
            deathFadeAlpha = 0.0f;
            gameCamera.AddTrauma(0.9f);
            return;
        }

        Rectangle footCollider = player->GetFootCollider();

        // Оновлення тригерів та запуск переходу при вході в двері
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

        // Оновлення камери
        Vector2 desiredTarget = {
            footCollider.x + footCollider.width / 2.0f,
            footCollider.y + footCollider.height / 2.0f
        };
        float staminaRatio = player->GetCurrentStamina() / player->GetMaxStamina();
        bool effectiveMoving = !isMovementBlocked && player->IsMoving();
        gameCamera.Update(desiredTarget, effectiveMoving, player->IsSprinting(), player->IsExhausted(), staminaRatio, dt);

        // Мерехтіння та передача позиції гравця в шейдер
        flickerTimer += dt;
        float noise = (float)GetRandomValue(-10, 10) / 900.0f;
        currentFlicker = 1.0f + (sinf(flickerTimer * 10.0f) * 0.025f) + noise;
        SetShaderValue(lightShader, flickerLoc, &currentFlicker, SHADER_UNIFORM_FLOAT);

        Vector2 playerScreenPos = gameCamera.GetScreenPosition(desiredTarget);

        float pos[2] = {
            playerScreenPos.x,
            (float)virtualHeight - playerScreenPos.y
        };

        SetShaderValue(lightShader, playerPosLoc, pos, SHADER_UNIFORM_VEC2);
    }
}

void GameplayState::Draw() {
    // 1. РЕНДЕРИНГ ІГРОВОГО СВІТУ
    BeginTextureMode(targetTexture);
    ClearBackground(BLACK);

    BeginMode2D(gameCamera.GetCamera());
    renderSystem.RenderWorld(gameMap, player.get(), guard.get());
    if (showDebugOverlay) {
        gameMap.DrawDebug();
    }
    EndMode2D();

    EndTextureMode();

    // 2. МАСШТАБУВАННЯ НА ЕКРАН
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

    // 3. ІНТЕРФЕЙС ГРАВЦЯ
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

        Rectangle foot = player->GetFootCollider();
        DrawRectangle(10, 10, 350, 80, Fade(BLACK, 0.8f));
        DrawRectangleLines(10, 10, 350, 80, GREEN);

        DrawFPS(20, 20);
        DrawText(TextFormat("Player Pos: X: %.1f | Y: %.1f", foot.x, foot.y), 20, 45, 18, GREEN);
        DrawText(TextFormat("Tile Coords: X: %d | Y: %d", (int)(foot.x / 32.0f), (int)(foot.y / 32.0f)), 20, 65, 18, YELLOW);
    }

    // 4. ЗАТЕМНЕННЯ ПРИ ТЕЛЕПОРТАЦІЇ
    if (tpFadeAlpha > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, tpFadeAlpha));
    }

    // 5. ЕФЕКТ СМЕРТІ ПЕРЕД GAME OVER
    if (isDying) {
        float clampedAlpha = (deathFadeAlpha > 1.0f) ? 1.0f : deathFadeAlpha;
        // Спочатку спалах червоного, який переходить у чорний
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(MAROON, clampedAlpha * 0.45f));
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, clampedAlpha));
    }

    // 6. ОВЕРЛЕЙ ПАУЗИ
    if (isPaused) {
        DrawPauseMenu();
    }
}

void GameplayState::DrawPauseMenu() {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // Напівпрозоре затемнення ігрового світу
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