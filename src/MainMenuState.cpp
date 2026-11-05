#include "MainMenuState.h"
#include "GameplayState.h"
#include "GameManager.h"
#include "Logger.h"

void MainMenuState::Init() {
    AssetManager& am = AssetManager::Instance();

    // 1. ШАБЛОН: Спроба завантаження фону (якщо файлу немає — AssetManager обробить це)
    am.LoadTexture("MainMenuBG.jpg", Config::PATH_PNG_MENU_BG);
    bgTexture = am.GetTexture("MainMenuBG.jpg");
    hasBackground = (bgTexture.id > 0);

    // 2. ШАБЛОН: Спроба завантаження фонової музики
    am.LoadMusic("menu_music", Config::PATH_AUDIO_MENU_MUSIC);
    bgMusic = am.GetMusic("menu_music");
    hasMusic = (bgMusic.stream.buffer != nullptr);

    if (hasMusic) {
        SetMusicVolume(bgMusic, 0.5f);
        bgMusic.looping = true;
        PlayMusicStream(bgMusic);
    }

    // 3. Розрахунок позицій кнопок по центру екрана
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float btnWidth = 280.0f;
    float btnHeight = 60.0f;
    float btnX = (screenW - btnWidth) / 2.0f;

    // Кнопка "Грати" (трохи вище центру)
    playButton.bounds = { btnX, (screenH / 2.0f) - 40.0f, btnWidth, btnHeight };
    playButton.text = "PLAY";

    // Кнопка "Вихід" (під кнопкою "Грати")
    exitButton.bounds = { btnX, (screenH / 2.0f) + 40.0f, btnWidth, btnHeight };
    exitButton.text = "EXIT";
}

void MainMenuState::Cleanup() {
    if (hasMusic) {
        StopMusicStream(bgMusic);
    }
    // Пам'ять для текстур/музики очищується централізовано через AssetManager
}

void MainMenuState::Update() {
    // Підкачка аудіобуфера, якщо музика є
    if (hasMusic) {
        UpdateMusicStream(bgMusic);
    }

    Vector2 mousePos = GetMousePosition();

    // Перевірка наведення миші на кнопки
    playButton.isHovered = CheckCollisionPointRec(mousePos, playButton.bounds);
    exitButton.isHovered = CheckCollisionPointRec(mousePos, exitButton.bounds);

    // Обробка кліків
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (playButton.isHovered) {
            // Перехід у стан геймплею
           
             GameManager::Instance().ChangeState(new GameplayState());
        }
        else if (exitButton.isHovered) {
            // Закриття програми
            CloseWindow();
        }
    }
}

void MainMenuState::Draw() {
    ClearBackground(BLACK);

    // 1. МАНЮВАННЯ ФОНУ (Якщо є текстура — малюємо її, інакше залишається BLACK)
    if (hasBackground) {
        DrawTexturePro(
            bgTexture,
            Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
            Rectangle{ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },
            Vector2{ 0, 0 },
            0.0f,
            WHITE
        );
    }

    // 2. ЗАГОЛОВОК ГРИ (Тимчасовий текстовий логотип)
    const char* titleText = "_PR0JECT_D1CE";
    int fontSize = 60;
    int textWidth = MeasureText(titleText, fontSize);
    DrawText(titleText, (GetScreenWidth() - textWidth) / 2, GetScreenHeight() / 4, fontSize, GOLD);

    // 3. МАЛЮВАННЯ КНОПОК
    DrawButton(playButton);
    DrawButton(exitButton);
}

void MainMenuState::DrawButton(const MenuButton& btn) {
    // Кольори для звичайного стану та при наведенні
    Color rectColor = btn.isHovered ? Fade(GOLD, 0.3f) : Fade(DARKGRAY, 0.6f);
    Color borderColor = btn.isHovered ? GOLD : GRAY;
    Color textColor = btn.isHovered ? WHITE : LIGHTGRAY;

    // Фон кнопки
    DrawRectangleRec(btn.bounds, rectColor);
    // Обводка
    DrawRectangleLinesEx(btn.bounds, 2.0f, borderColor);

    // Текст по центру кнопки
    int fontSize = 24;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = btn.bounds.x + (btn.bounds.width - textWidth) / 2.0f;
    float textY = btn.bounds.y + (btn.bounds.height - fontSize) / 2.0f;

    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);
}