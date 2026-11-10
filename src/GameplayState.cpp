#include "GameplayState.h"
#include "raymath.h"
#include <string>
#include <cmath>


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

    float ambient[4] = { 0.04f, 0.04f, 0.08f, 1.0f }; // Тон темряви навколо
    SetShaderValue(lightShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    // ============================================

    AssetManager& am = AssetManager::Instance();

    // Завантаження графіки для рівня
    am.LoadTexture("player_spritesheet", Config::PATH_PNG_PLAYER_SPRITESHEET);
    am.LoadTexture("1 other objects", Config::PATH_PNG_OTHER_OBJECTS);
    am.LoadTexture("1_FLOOR", Config::PATH_PNG_FLOOR);
    am.LoadTexture("1_RUGS", Config::PATH_PNG_RUGS);
    am.LoadTexture("Playing tables", Config::PATH_PNG_PLAYING_TABLES);
    am.LoadTexture("Slot_machines", Config::PATH_PNG_SLOT_MACHINES);
    am.LoadTexture("WALLS 1 32x32", Config::PATH_PNG_WALLS_1);
    am.LoadTexture("WALLS 2 32x32", Config::PATH_PNG_WALLS_2);
    am.LoadTexture("Slot machine side", Config::PATH_PNG_SLOT_MACHINE_SIDE);
    am.LoadTexture("Tables 1", Config::PATH_PNG_TABLES_1);
    am.LoadTexture("Couches 1", Config::PATH_PNG_COUCHES_1);

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
    Vector2 startPos = { 1944.0f, 2464.0f };
    player = new Player(startPos, 112.0f, 32.0f, 64.0f, am.GetTexture("player_spritesheet"), 1.45f);
    guard = new Guard({ 2200.0f, 2464.0f }, 70.0f);

    camera.target = startPos;
    cameraCurrentTarget = startPos; 
   
    camera.offset = { (float)virtualWidth / 2.0f, (float)virtualHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.8f;

    //======================MUSIC======================
    AssetManager::Instance().LoadMusic("ambient", Config::PATH_AUDIO_AMBIENT);
    bgMusic = AssetManager::Instance().GetMusic("ambient");

    SetMusicVolume(bgMusic, 0.046f);
    bgMusic.looping = true;

    PlayMusicStream(bgMusic);
    //=================================================
}

void GameplayState::Cleanup() {
    // Вивантажуємо мапу
    gameMap.Unload();

    if (lightShader.id != 0) {
        UnloadShader(lightShader);
    }

    if (targetTexture.id != 0) {
        UnloadRenderTexture(targetTexture);
    }

    if (player) {
        delete player;
        player = nullptr;
    }

    if (guard) {
        delete guard;
        guard = nullptr;
    }

    AssetManager::Instance().UnloadAll();
}

void GameplayState::Update() {
    // -------------------------------------------------------------------------
    // ТУТ БУДЕ ЛОГІКА ПАУЗИ В МАЙБУТНЬОМУ
    // -------------------------------------------------------------------------

    if (player) {
        UpdateMusicStream(bgMusic);
        player->Update(gameMap);
        float dt = GetFrameTime();

        // === ЛОГІКА АНІМАЦІЇ UI КАРТИ ЖИТТІВ ===
        int currentLives = player->GetLives();
        if (currentLives < lastPlayerLives) {
            cardFlickerTimer = cardFlickerDuration;
            lastPlayerLives = currentLives;
        }

        if (cardFlickerTimer > 0.0f) {
            cardFlickerTimer -= dt;
            if (cardFlickerTimer < 0.0f) cardFlickerTimer = 0.0f;
        }
        // ========================================

        if (guard) {
            guard->Update(*player, dt);
        }

        // Перевірка на кінець життів
        if (player->GetLives() <= 0 && !isGameOverProcessed) {
            TraceLog(LOG_WARNING, "GAME OVER! Player died.");
            isGameOverProcessed = true; 
        }



        Rectangle footCollider = player->GetFootCollider();

        // 1. Оновлення тригерів дверей / взаємодій
        gameMap.UpdateDoorTriggers(footCollider);

        // 2. ЦІЛЬОВА ПОЗИЦІЯ ГРАВЦЯ (центр ніг)
        Vector2 desiredTarget = {
            footCollider.x + footCollider.width / 2.0f,
            footCollider.y + footCollider.height / 2.0f
        };

        

      
        bool isMoving = player->IsMoving();
        bool isSprinting = player->IsSprinting();

        // Idle  -> Walk  -> Sprint 
        float targetSpeed = isMoving ? (isSprinting ? 14.0f : 9.0f) : 2.0f;
        float targetAmpX = isMoving ? (isSprinting ? 1.9f : 1.0f) : 0.5f;
        float targetAmpY = isMoving ? (isSprinting ? 2.5f : 1.5f) : 0.9f;

        // Плавний перехід параметрів похитування
        float blendAlpha = 1.0f - expf(-8.0f * dt);
        currentBreathSpeed = Lerp(currentBreathSpeed, targetSpeed, blendAlpha);
        currentAmpX = Lerp(currentAmpX, targetAmpX, blendAlpha);
        currentAmpY = Lerp(currentAmpY, targetAmpY, blendAlpha);

        breathTimer += dt * currentBreathSpeed;

        // Фігура Ліссажу
        Vector2 breathOffset = {
            cosf(breathTimer * 0.5f) * currentAmpX,
            sinf(breathTimer) * currentAmpY
        };

        // --- 2. НАКОПИЧЕННЯ ТА ЗГАСАННЯ ТРАВМИ (TRAUMA SHAKE) ---
        if (player->IsExhausted()) {
            // При повному виснаженні фіксуємо мінімальний рівень травми
            if (trauma < 0.40f) trauma = 0.40f;
        }
        else {
            // Якщо стаміна менше 35%, додаємо м'яку мікро-вібрацію
            float staminaRatio = player->GetCurrentStamina() / player->GetMaxStamina();
            if (staminaRatio < 0.35f) {
                float lowStaminaFactor = (0.35f - staminaRatio) / 0.35f;
                float targetTrauma = lowStaminaFactor * 0.22f;
                if (trauma < targetTrauma) trauma = targetTrauma;
            }
        }

        // Поступове заспокоєння камери з часом
        if (trauma > 0.0f) {
            trauma -= dt * 0.7f;
            if (trauma < 0.0f) trauma = 0.0f;
        }

        // Розрахунок квадратичної інтенсивності зсуву (trauma^2)
        float shakeIntensity = trauma * trauma;
        Vector2 shakeOffset = {
            ((float)GetRandomValue(-100, 100) / 100.0f) * maxShakeOffset * shakeIntensity,
            ((float)GetRandomValue(-100, 100) / 100.0f) * maxShakeOffset * shakeIntensity
        };

        // 3. ПЛАВНЕ СЛІДУВАННЯ КАМЕРИ 
        float cameraAlpha = 1.0f - expf(-cameraLerpSpeed * dt);
        cameraCurrentTarget = Vector2Lerp(cameraCurrentTarget, desiredTarget, cameraAlpha);

        
        camera.target = Vector2Add(cameraCurrentTarget, Vector2Add(breathOffset, shakeOffset));

        // 4. Мерехтіння та передача позиції гравця в шейдер
        flickerTimer += dt;
        float noise = (float)GetRandomValue(-10, 10) / 900.0f;
        currentFlicker = 1.0f + (sinf(flickerTimer * 10.0f) * 0.025f) + noise;
        SetShaderValue(lightShader, flickerLoc, &currentFlicker, SHADER_UNIFORM_FLOAT);

        
        Vector2 playerScreenPos = GetWorldToScreen2D(desiredTarget, camera);

        // Flip Y під OpenGL координати
        float pos[2] = {
            playerScreenPos.x,
            (float)virtualHeight - playerScreenPos.y
        };

        SetShaderValue(lightShader, playerPosLoc, pos, SHADER_UNIFORM_VEC2);
    }
}

void GameplayState::Draw() {
    // 1. РЕНДЕРИНГ ІГРОВОГО СВІТУ (малюємо всередині текстури рендерингу)
    BeginTextureMode(targetTexture);
    ClearBackground(BLACK);

    BeginMode2D(camera);

    // Спершу малюємо тайли, що знаходяться ПІД гравцем (підлога, килими тощо)
    gameMap.DrawBelowPlayer();

    if (player) {
        player->Draw();
    }

    //  малювання НПС-мутанта
    if (guard) {
        guard->Draw();
    }

    // Потім малюємо шари НАД гравцем (дах, верхні частини стін тощо)
    gameMap.DrawAbovePlayer();

    EndMode2D();

    EndTextureMode();

    // 2. ЛОГІКА МАСШТАБУВАННЯ
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

    // -------------------------------------------------------------------------
    //  ВІДОБРАЖЕННЯ ІНТЕРФЕЙСУ 
    // -------------------------------------------------------------------------
    Texture2D cardTex = AssetManager::Instance().GetTexture("card_lives");
    if (player) {
        // --- ШКАЛА СТАМІНИ ---
        float staminaRatio = player->GetCurrentStamina() / player->GetMaxStamina();
        if (staminaRatio < 0.0f) staminaRatio = 0.0f;
        if (staminaRatio > 1.0f) staminaRatio = 1.0f;

       
        int staminaFrame = (int)std::round(staminaRatio * 10.0f);

        std::string staminaTexName = "stamina_" + std::to_string(staminaFrame);
        Texture2D staminaTex = AssetManager::Instance().GetTexture(staminaTexName);

        if (staminaTex.id > 0) {
            Vector2 position = { 20.0f, 400.0f }; 
            float scaleTex = 2.5f;                
            float rotation = 270.0f;

            DrawTextureEx(staminaTex, position, rotation, scaleTex, WHITE);
            
        }

        // --- ВІДОБРАЖЕННЯ КАРТИ ЖИТТІВ (4 кадри 32x32, спрайтшит 128x32) ---
        Texture2D cardTex = AssetManager::Instance().GetTexture("card_lives");
        if (cardTex.id > 0) {
            const float frameWidth = 32.0f;
            const float frameHeight = 32.0f;

            int frameIndex = 0;
            if (player->GetLives() == 2) {
                frameIndex = 1;
            }
            else if (player->GetLives() == 1) {
                frameIndex = 2;
            }
            else if (player->GetLives() <= 0) {
                frameIndex = 3;
            }

            Rectangle cardSourceRec = {
                (float)frameIndex * frameWidth,
                0.0f,
                frameWidth,
                frameHeight
            };

            Vector2 cardPos = { 55.0f, 105.0f }; // Позиція над стаміною
            float cardScale = 4.5f;              // Розмір 80x80 px

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



        // --- ДЕБАГ ПАНЕЛЬ ---
        Rectangle foot = player->GetFootCollider();
        DrawRectangle(10, 10, 350, 80, Fade(BLACK, 0.8f));
        DrawRectangleLines(10, 10, 350, 80, GREEN);

        DrawFPS(20, 20);
        DrawText(TextFormat("Player Pos: X: %.1f | Y: %.1f", foot.x, foot.y), 20, 45, 18, GREEN);
        DrawText(TextFormat("Tile Coords: X: %d | Y: %d", (int)(foot.x / 32.0f), (int)(foot.y / 32.0f)), 20, 65, 18, YELLOW);
    }
}