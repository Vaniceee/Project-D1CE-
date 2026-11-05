#include "MainMenuState.h"
#include "GameplayState.h"
#include "GameManager.h"
#include "Logger.h"

void MainMenuState::Init() {
    AssetManager& am = AssetManager::Instance();

    // 1. ШАБЛОН: Спроба завантаження фону 
    am.LoadTexture("MainMenuBG.jpg", Config::PATH_PNG_MENU_BG);
    bgTexture = am.GetTexture("MainMenuBG.jpg");
    hasBackground = (bgTexture.id > 0);

    // 2. ШАБЛОН: Спроба завантаження фонової музики
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

    // 4. Скидання стану переходу
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
  
}

void MainMenuState::Update() {
    float deltaTime = GetFrameTime();
    
    if (hasMusic) {
        UpdateMusicStream(bgMusic);
    }

    if (isTransitioning) {
        transitionTimer += deltaTime;
        float progress = transitionTimer / transitionDuration; 

        // 1. Затухання гучності та заниження Pitch фонової музики
        if (hasMusic) {
            float currentVolume = 0.24f * (1.0f - progress);
            float currentPitch = 1.0f - (progress * 0.4f); 

            SetMusicVolume(bgMusic, currentVolume);
            SetMusicPitch(bgMusic, currentPitch);
        }

        // 2. Плавне затухання звуку старту (згасає від початкової гучності до 0.0)
        if (hasStartSound) {
            float startSoundBaseVolume = 0.5f; 
            float currentStartVolume = startSoundBaseVolume * (1.2f - progress);

            SetSoundVolume(startSound, currentStartVolume);
        }




        // 2. Після завершення 4 секунд переходимо у геймплей
        if (transitionTimer >= transitionDuration) {
            GameManager::Instance().ChangeState(new GameplayState());
            return;
        }
    }
    else {
        // --- ЗВИЧАЙНИЙ СТАН МЕНЮ ---
        Vector2 mousePos = GetMousePosition();

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
            }
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
    //---------ПЕРЕХІД--------------
    if (isTransitioning) {
        float progress = transitionTimer / transitionDuration; // 0.0f -> 1.0f

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