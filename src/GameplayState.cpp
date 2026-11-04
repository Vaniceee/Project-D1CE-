#include "GameplayState.h"
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
    am.LoadTexture("1 other objects", Config::PATH_PNG_OTHER_OBJECTS);
    am.LoadTexture("1_FLOOR", Config::PATH_PNG_FLOOR);
    am.LoadTexture("1_RUGS", Config::PATH_PNG_RUGS);
    am.LoadTexture("Playing tables", Config::PATH_PNG_PLAYING_TABLES);
    am.LoadTexture("Slot_machines", Config::PATH_PNG_SLOT_MACHINES);
    am.LoadTexture("WALLS 1 32x32", Config::PATH_PNG_WALLS_1);
    am.LoadTexture("WALLS 2 32x32", Config::PATH_PNG_WALLS_2);
    am.LoadTexture("Slot machine side", Config::PATH_PNG_SLOT_MACHINE_SIDE);

    for (int i = 0; i <= 10; ++i) {
        std::string name = "stamina_" + std::to_string(i);
        std::string path = "assets/stamina/bar" + std::to_string(i) + ".png";
        am.LoadTexture(name, path);
    }

    gameMap.Load(Config::PATH_MAP);
    Vector2 startPos = { 2944.0f, 2464.0f };
    player = new Player(startPos, 250.0f, 32.0f, 64.0f);

    camera.target = startPos;
    camera.offset = { (float)virtualWidth / 2.0f, (float)virtualHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.8f;
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

    AssetManager::Instance().UnloadAll();
}

void GameplayState::Update() {
    // -------------------------------------------------------------------------
    // ТУТ БУДЕ ЛОГІКА ПАУЗИ В МАЙБУТНЬОМУ
    // -------------------------------------------------------------------------

    if (player) {
        player->Update(gameMap);
  
        Rectangle footCollider = player->GetFootCollider();


        // Оновлення тригеров дверей / взаємодій
        gameMap.UpdateDoorTriggers(footCollider);
        camera.target = {
            footCollider.x + footCollider.width / 2.0f,
            footCollider.y + footCollider.height / 2.0f

        };
        // Мерехтіння та передача позиції гравця в шейдер
        flickerTimer += GetFrameTime();
        float noise = (float)GetRandomValue(-10, 10) / 400.0f;
        currentFlicker = 1.0f + (sinf(flickerTimer * 5.0f) * 0.025f) + noise;
        SetShaderValue(lightShader, flickerLoc, &currentFlicker, SHADER_UNIFORM_FLOAT);

        Vector2 playerScreenPos = GetWorldToScreen2D(camera.target, camera);
        float pos[2] = { playerScreenPos.x, playerScreenPos.y };
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

        // --- ДЕБАГ ПАНЕЛЬ ---
        Rectangle foot = player->GetFootCollider();
        DrawRectangle(10, 10, 350, 80, Fade(BLACK, 0.8f));
        DrawRectangleLines(10, 10, 350, 80, GREEN);

        DrawFPS(20, 20);
        DrawText(TextFormat("Player Pos: X: %.1f | Y: %.1f", foot.x, foot.y), 20, 45, 18, GREEN);
        DrawText(TextFormat("Tile Coords: X: %d | Y: %d", (int)(foot.x / 32.0f), (int)(foot.y / 32.0f)), 20, 65, 18, YELLOW);
    }
}