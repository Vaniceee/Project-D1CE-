#include "GameplayState.h"
#include "MainMenuState.h"
#include "GameOverState.h"
#include "GameManager.h"
#include "InputManager.h"
#include "VictoryState.h"
#include "Logger.h"
#include "Dice.h"

void GameplayState::SyncShaderCameraUniforms() {
    if (lightShader.id == 0) return;
    const Camera2D& cam = gameCamera.GetCamera();
    float targetArr[2] = { cam.target.x, cam.target.y };
    float offsetArr[2] = { cam.offset.x, cam.offset.y };
    float zoomVal = cam.zoom;

    SetShaderValue(lightShader, cameraTargetLoc, targetArr, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightShader, cameraOffsetLoc, offsetArr, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightShader, cameraZoomLoc, &zoomVal, SHADER_UNIFORM_FLOAT);
}

void GameplayState::OnInteract(InteractableObject& obj) {
    if (obj.isUsed) {
        GAME_INFO("INTERACT", "Object '%s' is already opened!", obj.fullId.c_str());
        return;
    }

    // --- ФІНАЛЬНІ ВИХІДНІ ДВЕРІ (exit_door_i -> type == "exit") ---
    if (obj.type == "exit") {
        if (!player->HasItem("MasterKey")) {
            soundSynth.PlayLockedDoor();
            ShowStatusNotification("Locked! Master Key required to escape.", RED, 2.5f);
            GAME_INFO("INTERACT", "Exit door '%s' locked: missing MasterKey.", obj.fullId.c_str());
            return;
        }

        soundSynth.PlayLockOpen();
        obj.isUsed = true;
        isVictoryTriggered = true;
        GAME_INFO("INTERACT", "Player escaped through '%s' with MasterKey! Victory triggered.", obj.fullId.c_str());
        return;
    }

    // --- ПЕРЕВІРКА КАРТКИ ДЛЯ СЕЙФА ---
    if (obj.type == "safe" && !player->HasItem("SafeKeyCard")) {
        soundSynth.PlayLockedDoor();
        ShowStatusNotification("Locked! Safe Key Card required.", RED, 2.5f);
        GAME_INFO("INTERACT", "Safe '%s' locked: missing SafeKeyCard.", obj.fullId.c_str());
        return;
    }

    // --- ПЕРЕВІРКА ТИПУ ОБ'ЄКТА ДЛЯ КУБИКІВ ---
    if (obj.type != "lock" && obj.type != "safe") {
        GAME_INFO("INTERACT", "This object cannot be opened with dice.");
        return;
    }

    // Кидаємо два кубики
    soundSynth.PlayDiceRoll();
    lastDice1 = Dice::Roll();
    lastDice2 = Dice::Roll();
    lastDiceSum = lastDice1 + lastDice2;
    sumDisplayTimer = sumDisplayDuration;

    GAME_INFO("DICE", "Dice: %d + %d = %d", lastDice1, lastDice2, lastDiceSum);

    bool opened = false;
    if (obj.type == "lock" && lastDiceSum >= 7) {
        opened = true;
    }
    else if (obj.type == "safe" && lastDiceSum >= 9) {
        opened = true;
    }

    if (opened) {
        obj.isUsed = true;
        soundSynth.PlayLockOpen();

        if (obj.storedItem != nullptr) {
            std::string itemName = obj.storedItem->GetName();
            player->AddItem(std::move(obj.storedItem));

            if (itemName == "RoomKey") {
                soundSynth.PlayKeyPickup();
                ShowStatusNotification("Found: Guard Room Key!", GREEN, 3.0f);
            }
            else if (itemName == "SafeKeyCard") {
                soundSynth.PlayCardPickup();
                ShowStatusNotification("Found: Safe Key Card!", GREEN, 3.0f);
            }
            else if (itemName == "MasterKey") {
                soundSynth.PlayKeyPickup();
                ShowStatusNotification("Found: Master Key!", GOLD, 3.0f);
            }
            else {
                soundSynth.PlayKeyPickup();
                ShowStatusNotification("Found: " + itemName, GREEN, 2.5f);
            }

            GAME_INFO("INTERACT", "Object '%s' opened! Obtained '%s'.",
                obj.fullId.c_str(), itemName.c_str());
        }
        else {
            ShowStatusNotification("Empty...", LIGHTGRAY, 1.5f);
            GAME_INFO("INTERACT", "Object '%s' opened successfully (empty).",
                obj.fullId.c_str());
        }
    }
    else {
        soundSynth.PlayLockFail();
    }
}

void GameplayState::ShowStatusNotification(const std::string& text, Color color, float duration) {
    statusNotificationText = text;
    statusNotificationColor = color;
    statusNotificationTimer = duration;
    statusNotificationDuration = duration;
}

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
    cameraTargetLoc = GetShaderLocation(lightShader, "cameraTarget");
    cameraOffsetLoc = GetShaderLocation(lightShader, "cameraOffset");
    cameraZoomLoc = GetShaderLocation(lightShader, "cameraZoom");

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

    AssetManager& am = AssetManager::Instance();

    am.LoadTexture("player_spritesheet", Config::PATH_PNG_PLAYER_SPRITESHEET);
    am.LoadTexture("floor", Config::PATH_PNG_FLOOR);
    am.LoadTexture("rugs", Config::PATH_PNG_RUGS);
    am.LoadTexture("walls", Config::PATH_PNG_WALLS_1);
    am.LoadTexture("walls1", Config::PATH_PNG_WALLS_2);

    // Іконки ключів для слотів інвентарю
    am.LoadTexture("icon_room_key", Config::PATH_PNG_ICON_ROOM_KEY);
    am.LoadTexture("icon_safe_card", Config::PATH_PNG_ICON_SAFE_CARD);
    am.LoadTexture("icon_master_key", Config::PATH_PNG_ICON_MASTER_KEY);

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

    hudUI.Init();
    soundSynth.Init();
    gameMap.Load(Config::PATH_MAP);
    gameMap.DistributeLoot();
    statusNotificationTimer = 0.0f;

    Vector2 startPos = gameMap.GetPlayerSpawn();
    player = std::make_unique<Player>(startPos, 212.0f, 32.0f, 64.0f, am.GetTexture("player_spritesheet"), 1.45f);

    Rectangle startFoot = player->GetFootCollider();
    lightWorldPos = {
        startFoot.x + startFoot.width * 0.5f,
        startFoot.y + startFoot.height * 0.5f
    };
    lightVelocity = { 0.0f, 0.0f };
    isLightPosInitialized = true;

    floorHint.anchorPos = {
        startFoot.x + startFoot.width * 0.5f,
        startFoot.y + startFoot.height
    };
    floorHint.moveAlpha = 1.0f;
    floorHint.sprintAlpha = 1.0f;
    floorHint.time = 0.0f;
    hasMovedOnce = false;
    hasSprintedOnce = false;

    const auto& guardSpawns = gameMap.GetGuardSpawns();
    Vector2 guardPos = !guardSpawns.empty() ? guardSpawns[0] : Vector2{ 2200.0f, 2464.0f };
    guard = std::make_unique<Guard>(guardPos, 70.0f);
    guard->InitPathfinding(gameMap);
    guard->SetPatrolWaypoints(gameMap.GetGuardPatrolPoints());

    baseCameraZoom = 1.8f;
    gameCamera.Init(startPos, virtualWidth, virtualHeight, baseCameraZoom);
    SyncShaderCameraUniforms();

    isPaused = false;
    isDying = false;
    deathTimer = 0.0f;
    deathFocusPos = startPos;
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

    sprintDust.clear();
    sprintDustTimer = 0.0f;
    lastGuardState = GuardState::PATROL;
    detectionFlashTimer = 0.0f;
    currentCamNumber = 1;
    lastPlayerLives = player ? player->GetLives() : 3;
    cardFlickerTimer = 0.0f;

    AssetManager::Instance().LoadMusic("ambient", Config::PATH_AUDIO_AMBIENT);
    bgMusic = AssetManager::Instance().GetMusic("ambient");
    SetMusicVolume(bgMusic, 0.046f);
    bgMusic.looping = true;
    PlayMusicStream(bgMusic);
}

void GameplayState::Cleanup() {
    StopMusicStream(bgMusic);
    if (guard) {
        guard->StopAudio();
    }
    soundSynth.Cleanup();
    hudUI.Cleanup();
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

void GameplayState::UpdateDeathSequence(float dt) {
    deathTimer += dt;
    shaderTime += dt;

    float progress = Clamp(deathTimer / deathDuration, 0.0f, 1.0f);

    UpdateMusicStream(bgMusic);
    SetMusicVolume(bgMusic, 0.046f * Clamp(1.0f - progress * 1.35f, 0.0f, 1.0f));
    SetMusicPitch(bgMusic, Lerp(1.0f, 0.55f, progress));

    if (progress < 0.14f) {
        gameCamera.SetMinTrauma(0.65f);
    }
    else if (progress > 0.72f && progress < 0.82f) {
        gameCamera.SetMinTrauma(0.45f);
    }

    float targetDeathZoom = Lerp(baseCameraZoom, 2.55f, 1.0f - powf(1.0f - progress, 2.2f));
    gameCamera.SetZoom(Lerp(gameCamera.GetZoom(), targetDeathZoom, 1.0f - expf(-8.0f * dt)));
    gameCamera.Update(deathFocusPos, false, false, true, 0.0f, dt);
    SyncShaderCameraUniforms();

    float lightDecay = Clamp(1.0f - (progress / 0.48f), 0.0f, 1.0f);
    float dyingFlicker = (lightDecay > 0.02f)
        ? (lightDecay * (0.65f + 0.35f * sinf(deathTimer * 54.0f)))
        : 0.0f;

    float maxVal = 1.0f;
    float maxExhaustion = Clamp(0.5f + progress * 0.5f, 0.0f, 1.0f);

    SetShaderValue(lightShader, flickerLoc, &dyingFlicker, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, dangerLoc, &maxVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, panicLoc, &maxVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, exhaustionLoc, &maxExhaustion, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, timeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);

    Vector2 lightScreenPos = gameCamera.GetScreenPosition(deathFocusPos);
    float pos[2] = { lightScreenPos.x, (float)virtualHeight - lightScreenPos.y };
    SetShaderValue(lightShader, playerPosLoc, pos, SHADER_UNIFORM_VEC2);

    if (deathTimer >= deathDuration) {
        GameManager::Instance().ChangeState(new GameOverState());
    }
}

void GameplayState::Update() {
    float dt = GetFrameTime();

    if (isVictoryTriggered) {
        GameManager::Instance().ChangeState(new VictoryState());
        return;
    }

    if (sumDisplayTimer > 0.0f) {
        sumDisplayTimer -= dt;
        if (sumDisplayTimer < 0.0f) {
            sumDisplayTimer = 0.0f;
        }
    }

    if (isDying) {
        UpdateDeathSequence(dt);
        return;
    }

    if (InputManager::IsPausePressed() && tpPhase == TeleportPhase::NONE) {
        isPaused = !isPaused;
        if (isPaused) {
            hudUI.ResetPauseMenu();
            PauseMusicStream(bgMusic);
            if (guard) guard->SetAudioPaused(true);
        }
        else {
            ResumeMusicStream(bgMusic);
            if (guard) guard->SetAudioPaused(false);
        }
    }

    if (isPaused) {
        PauseMenuAction action = hudUI.UpdatePauseMenu(dt);
        if (action == PauseMenuAction::RESUME) {
            isPaused = false;
            ResumeMusicStream(bgMusic);
            if (guard) guard->SetAudioPaused(false);
        }
        else if (action == PauseMenuAction::RESTART) {
            GameManager::Instance().ChangeState(new GameplayState());
            return;
        }
        else if (action == PauseMenuAction::MAIN_MENU) {
            GameManager::Instance().ChangeState(new MainMenuState());
            return;
        }
        return;
    }

    if (InputManager::IsDebugTogglePressed()) {
        showDebugOverlay = !showDebugOverlay;
        float activeRadius = showDebugOverlay ? 2500.0f : lightRadius;
        SetShaderValue(lightShader, radiusLoc, &activeRadius, SHADER_UNIFORM_FLOAT);
    }

    if (!player) return;

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
            player->ResetVelocity();

            Rectangle newFoot = player->GetFootCollider();
            lightWorldPos = {
                newFoot.x + newFoot.width * 0.5f,
                newFoot.y + newFoot.height * 0.5f
            };
            lightVelocity = { 0.0f, 0.0f };

            tpPhase = TeleportPhase::FADING_IN;
            currentCamNumber = (currentCamNumber % 8) + 1;
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

    floorHint.time = shaderTime;
    if (!isMovementBlocked) {
        if (player->IsMoving()) {
            hasMovedOnce = true;
        }
        if (player->IsSprinting()) {
            hasSprintedOnce = true;
        }
    }
    if (hasMovedOnce && floorHint.moveAlpha > 0.0f) {
        floorHint.moveAlpha = fmaxf(0.0f, floorHint.moveAlpha - dt * 1.15f);
    }
    if (hasSprintedOnce && floorHint.sprintAlpha > 0.0f) {
        floorHint.sprintAlpha = fmaxf(0.0f, floorHint.sprintAlpha - dt * 1.15f);
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

        GuardState nowState = guard->GetCurrentState();
        if (nowState == GuardState::CHASE && lastGuardState != GuardState::CHASE) {
            soundSynth.PlayGuardAlert();
            detectionFlashTimer = 0.42f;
            damageImpulse = fmaxf(damageImpulse, 0.65f);
            gameCamera.AddTrauma(0.38f);
        }
        lastGuardState = nowState;
    }

    if (detectionFlashTimer > 0.0f) {
        detectionFlashTimer -= dt;
        if (detectionFlashTimer < 0.0f) detectionFlashTimer = 0.0f;
    }

    for (size_t i = 0; i < sprintDust.size(); ) {
        sprintDust[i].pos = Vector2Add(sprintDust[i].pos, Vector2Scale(sprintDust[i].vel, dt));
        sprintDust[i].life -= dt;
        if (sprintDust[i].life <= 0.0f) {
            sprintDust.erase(sprintDust.begin() + i);
        }
        else {
            ++i;
        }
    }

    if (!isMovementBlocked && player->IsMoving() && player->IsSprinting()) {
        sprintDustTimer -= dt;
        if (sprintDustTimer <= 0.0f) {
            sprintDustTimer = 0.065f;
            Rectangle pf = player->GetFootCollider();
            SprintDustParticle p;
            p.pos = {
                pf.x + pf.width * 0.5f + (float)GetRandomValue(-5, 5),
                pf.y + pf.height - 2.0f
            };
            p.vel = {
                (float)GetRandomValue(-14, 14),
                (float)GetRandomValue(-18, -6)
            };
            p.maxLife = (float)GetRandomValue(28, 44) / 100.0f;
            p.life = p.maxLife;
            p.size = (float)GetRandomValue(20, 34) / 10.0f;
            sprintDust.push_back(p);
        }
    }

    if (player->GetLives() <= 0 && !isDying) {
        TraceLog(LOG_WARNING, "GAME OVER! Player died.");
        isDying = true;
        deathTimer = 0.0f;
        Rectangle foot = player->GetFootCollider();
        deathFocusPos = { foot.x + foot.width * 0.5f, foot.y + foot.height * 0.5f };
        if (guard) guard->StopAudio();
        gameCamera.AddTrauma(0.95f);
        return;
    }

    Rectangle footCollider = player->GetFootCollider();
    showInteractHint = false;

    if (!isMovementBlocked) {
        InteractableObject* interactable = gameMap.GetInteractableAt(footCollider);
        if (interactable != nullptr && !interactable->isUsed) {
            showInteractHint = true;

            if (InputManager::IsInteractPressed()) {
                OnInteract(*interactable);
            }
        }
    }

    if (statusNotificationTimer > 0.0f) {
        statusNotificationTimer -= dt;
        if (statusNotificationTimer < 0.0f) statusNotificationTimer = 0.0f;
    }

    gameMap.UpdateTriggers(footCollider);
    const std::string& currentTrigger = gameMap.GetCurrentTriggerMessage();
    bool isInTriggerNow = !currentTrigger.empty();

    if (tpPhase == TeleportPhase::NONE) {
        if (isInTriggerNow && !wasInTrigger) {
            // 1. Вхід у кімнату охоронця (потрібен RoomKey)
            if (currentTrigger == "room3_in" && !player->HasItem("RoomKey")) {
                soundSynth.PlayLockedDoor();
                ShowStatusNotification("Locked! Guard Room Key required.", RED, 2.5f);
                GAME_INFO("TELEPORT", "Blocked entry to '%s': missing RoomKey.", currentTrigger.c_str());
                wasInTrigger = true;
            }
            // 2. Вхід у фінальну кімнату (потрібен MasterKey)
            else if (currentTrigger == "final_in" && !player->HasItem("MasterKey")) {
                soundSynth.PlayLockedDoor();
                ShowStatusNotification("Locked! Master Key required.", RED, 2.5f);
                GAME_INFO("TELEPORT", "Blocked entry to '%s': missing MasterKey.", currentTrigger.c_str());
                wasInTrigger = true;
            }
            else {
                Vector2 newPlayerPos;
                if (gameMap.TryGetTeleportDestination(footCollider, player->GetPosition(), newPlayerPos)) {
                    Vector2 enteredTriggerCenter;
                    if (guard && gameMap.TryGetActiveTriggerCenter(footCollider, enteredTriggerCenter)) {
                        guard->OnPlayerTeleported(enteredTriggerCenter);
                    }

                    soundSynth.PlayCamSwitch();
                    player->ResetVelocity();
                    pendingTeleportPos = newPlayerPos;
                    tpPhase = TeleportPhase::FADING_OUT;
                    wasInTrigger = true;
                }
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
        const float maxDist = 460.0f;
        float normalizedDist = Clamp((distToGuard - minDist) / (maxDist - minDist), 0.0f, 1.0f);
        float linearProx = 1.0f - normalizedDist;

        proximity = powf(linearProx, 1.65f);
        isChasing = (guard->GetCurrentState() == GuardState::CHASE);

        if (isChasing) {
            proximity = Clamp(proximity * 1.2f, 0.0f, 1.0f);
        }
    }

    if (proximity > 0.05f) {
        gameCamera.SetMinTrauma(Lerp(0.10f, 0.36f, proximity));
    }

    float staminaRatio = player->GetCurrentStamina() / player->GetMaxStamina();
    bool effectiveMoving = !isMovementBlocked && player->IsMoving();
    gameCamera.Update(desiredTarget, effectiveMoving, player->IsSprinting(), player->IsExhausted(), staminaRatio, dt);

    float targetZoom = baseCameraZoom;
    if (tpPhase == TeleportPhase::FADING_OUT) {
        targetZoom = baseCameraZoom + tpFadeAlpha * 0.38f;
    }
    else if (tpPhase == TeleportPhase::FADING_IN) {
        targetZoom = baseCameraZoom + tpFadeAlpha * 0.25f;
    }
    else if (detectionFlashTimer > 0.0f) {
        float flashWave = sinf((detectionFlashTimer / 0.42f) * PI);
        targetZoom = baseCameraZoom + flashWave * 0.14f;
    }
    gameCamera.SetZoom(Lerp(gameCamera.GetZoom(), targetZoom, 1.0f - expf(-10.0f * dt)));
    SyncShaderCameraUniforms();

    float flickerFreq = Lerp(8.5f, 24.0f, proximity);
    float waveAmp = Lerp(0.012f, 0.082f, proximity);
    float noiseAmp = Lerp(0.005f, 0.068f, proximity);

    flickerTimer += dt * flickerFreq;
    float rawNoise = ((float)GetRandomValue(-100, 100) / 100.0f) * noiseAmp;
    float noiseBlend = 1.0f - expf(-25.0f * dt);
    smoothedNoise = Lerp(smoothedNoise, rawNoise, noiseBlend);

    if (lightDipTimer > 0.0f) {
        lightDipTimer -= dt;
        if (lightDipTimer < 0.0f) lightDipTimer = 0.0f;
    }
    else if (proximity > 0.16f) {
        float triggerChancePerSec = proximity * 3.8f;
        if (((float)GetRandomValue(0, 10000) / 10000.0f) < triggerChancePerSec * dt) {
            lightDipDuration = (float)GetRandomValue(14, 21) / 100.0f;
            lightDipTimer = lightDipDuration;
            lightDipDepth = ((float)GetRandomValue(9, 19) / 100.0f) * proximity;
        }
    }

    float activeDip = 0.0f;
    if (lightDipTimer > 0.0f && lightDipDuration > 0.0f) {
        float dipProgress = 1.0f - (lightDipTimer / lightDipDuration);
        activeDip = sinf(dipProgress * PI) * lightDipDepth;
    }

    currentFlicker = 1.0f + (sinf(flickerTimer) * waveAmp) + smoothedNoise - activeDip;
    currentFlicker = Clamp(currentFlicker, 0.58f, 1.08f);
    SetShaderValue(lightShader, flickerLoc, &currentFlicker, SHADER_UNIFORM_FLOAT);

    smoothedDanger = Lerp(smoothedDanger, proximity, 1.0f - expf(-6.0f * dt));
    SetShaderValue(lightShader, dangerLoc, &smoothedDanger, SHADER_UNIFORM_FLOAT);

    float targetExhaustion = 0.0f;
    if (staminaRatio < 0.40f) {
        targetExhaustion = Clamp((0.40f - staminaRatio) / 0.40f, 0.0f, 1.0f);
    }
    if (player->IsExhausted()) {
        targetExhaustion = fmaxf(targetExhaustion, 0.78f);
    }
    smoothedExhaustion = Lerp(smoothedExhaustion, targetExhaustion, 1.0f - expf(-4.5f * dt));

    float targetPanic = Clamp(smoothedDanger * (isChasing ? 1.15f : 0.75f) + damageImpulse * 0.85f, 0.0f, 1.0f);
    smoothedPanic = Lerp(smoothedPanic, targetPanic, 1.0f - expf(-5.5f * dt));

    SetShaderValue(lightShader, exhaustionLoc, &smoothedExhaustion, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, panicLoc, &smoothedPanic, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightShader, timeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);

    Vector2 lightScreenPos = gameCamera.GetScreenPosition(lightWorldPos);
    float pos[2] = { lightScreenPos.x, (float)virtualHeight - lightScreenPos.y };
    SetShaderValue(lightShader, playerPosLoc, pos, SHADER_UNIFORM_VEC2);
}

void GameplayState::Draw() {
    BeginTextureMode(targetTexture);
    ClearBackground(BLACK);

    BeginMode2D(gameCamera.GetCamera());
    renderSystem.RenderWorld(gameMap, player.get(), guard.get(), &floorHint);

    if (player && sumDisplayTimer > 0.0f) {
        Vector2 playerPos = player->GetPosition();
        Vector2 headPos = {
            playerPos.x + (32.0f * player->GetScale()) * 0.5f,
            playerPos.y
        };
        hudUI.DrawDiceRollWorld(headPos, lastDice1, lastDice2, lastDiceSum, sumDisplayTimer, sumDisplayDuration);
    }

    for (const auto& p : sprintDust) {
        float ratio = Clamp(p.life / p.maxLife, 0.0f, 1.0f);
        float radius = p.size * (1.55f - ratio * 0.55f);
        DrawCircleV(p.pos, radius, ColorAlpha(Color{ 185, 178, 165, 255 }, ratio * 0.34f));
    }

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

    BeginShaderMode(lightShader);
    DrawTexturePro(targetTexture.texture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
    EndShaderMode();

    if (player && !isDying) {
        GameplayHudData hudData;
        hudData.lives = player->GetLives();
        hudData.currentStamina = player->GetCurrentStamina();
        hudData.maxStamina = player->GetMaxStamina();
        hudData.isExhausted = player->IsExhausted();
        hudData.shaderTime = shaderTime;
        hudData.cardFlickerTimer = cardFlickerTimer;
        hudData.currentCamNumber = currentCamNumber;
        hudData.hasRoomKey = player->HasItem("RoomKey");
        hudData.hasSafeKeyCard = player->HasItem("SafeKeyCard");
        hudData.hasMasterKey = player->HasItem("MasterKey");

        hudUI.DrawHud(hudData);

        if (showInteractHint && !isPaused && !isDying) {
            hudUI.DrawInteractPrompt(shaderTime);
        }

        if (statusNotificationTimer > 0.0f && !statusNotificationText.empty() && !isPaused && !isDying) {
            hudUI.DrawStatusNotification(
                statusNotificationText,
                statusNotificationColor,
                statusNotificationTimer,
                statusNotificationDuration,
                shaderTime
            );
        }

        if (showDebugOverlay) {
            Rectangle foot = player->GetFootCollider();
            float dbgY = windowHeight - 178.0f;
            DrawRectangle(28, (int)dbgY, 360, 82, Fade(BLACK, 0.85f));
            DrawRectangleLines(28, (int)dbgY, 360, 82, GREEN);

            DrawFPS(38, (int)dbgY + 10);
            DrawText(TextFormat("Player Pos: X: %.1f | Y: %.1f", foot.x, foot.y), 38, (int)dbgY + 34, 18, GREEN);
            DrawText(TextFormat("Tile Coords: X: %d | Y: %d", (int)(foot.x / 32.0f), (int)(foot.y / 32.0f)), 38, (int)dbgY + 56, 18, YELLOW);
        }
    }

    hudUI.DrawTeleportOverlay(tpFadeAlpha, currentCamNumber);

    if (isDying) {
        hudUI.DrawDeathOverlay(deathTimer, deathDuration);
    }

    if (isPaused) {
        hudUI.DrawPauseMenu();
    }
}