#include "GameplayState.h"

void GameplayState::Init() {
    targetTexture = LoadRenderTexture(virtualWidth, virtualHeight);

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

    gameMap.Load(Config::PATH_MAP);

    player = new Player({ 2944.0f, 2464.0f }, 250.0f, 40.0f, 60.0f);
}

void GameplayState::Cleanup() {
    // Вивантажуємо мапу
    gameMap.Unload();

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
        player->Update();

        // Перевірка колізій гравця зі стінами мапи
        // (Примітка: якщо у твого гравця є окремий метод для отримання хітбокса, наприклад player->GetHitbox(), використовуй його)
        Rectangle footCollider = player->GetFootCollider();


        // Оновлення тригеров дверей / взаємодій
        gameMap.UpdateDoorTriggers(footCollider);
    }
}

void GameplayState::Draw() {
    // 1. РЕНДЕРИНГ ІГРОВОГО СВІТУ (малюємо всередині текстури рендерингу)
    BeginTextureMode(targetTexture);
    ClearBackground(MAROON);

    // Спершу малюємо тайли, що знаходяться ПІД гравцем (підлога, килими тощо)
    gameMap.DrawBelowPlayer();

    if (player) {
        player->Draw();
    }

    // Потім малюємо шари НАД гравцем (дах, верхні частини стін тощо)
    gameMap.DrawAbovePlayer();

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

    DrawTexturePro(targetTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);

    // -------------------------------------------------------------------------
    // ТУТ БУДЕ ВІДОБРАЖЕННЯ ІНТЕРФЕЙСУ ПАУЗИ В МАЙБУТНЬОМУ
    // -------------------------------------------------------------------------
}