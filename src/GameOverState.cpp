#include "GameOverState.h"
#include "MainMenuState.h"
#include "GameManager.h"
#include "AssetManager.h"
#include "Logger.h"
#include <cmath>

void GameOverState::Init() {
    AssetManager& am = AssetManager::Instance();

    // 1. ШАБЛОН: завантаження фону (поки файлу немає — малюється процедурний темний фон)
    // am.LoadTexture("GameOverBG", "assets/menu/GameOverBG.png");
    bgTexture = am.GetTexture("GameOverBG");
    hasBackground = (bgTexture.id > 0);

    // 2. ШАБЛОН: завантаження звуку поразки / сміху дилера / удару
    // am.LoadSound("sfx_gameover", "assets/sfx/gameover.wav");
    gameOverSound = am.GetSound("sfx_gameover");
    hasSound = (gameOverSound.stream.buffer != nullptr);

    if (hasSound) {
        SetSoundVolume(gameOverSound, 0.6f);
        PlaySound(gameOverSound);
    }

    // 3. ШАБЛОН: завантаження похмурого ембієнту для екрана смерті
    // am.LoadMusic("GameOverTheme", "assets/music/gameover_theme.wav");
    gameOverMusic = am.GetMusic("GameOverTheme");
    hasMusic = (gameOverMusic.stream.buffer != nullptr);

    if (hasMusic) {
        SetMusicVolume(gameOverMusic, 0.25f);
        gameOverMusic.looping = true;
        PlayMusicStream(gameOverMusic);
    }

    sceneTimer = 0.0f;
    isExitingToMenu = false;
    exitFadeTimer = 0.0f;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float btnWidth = 320.0f;
    float btnHeight = 60.0f;
    menuButton.bounds = {
        (screenW - btnWidth) * 0.5f,
        (screenH * 0.65f),
        btnWidth,
        btnHeight
    };
    menuButton.text = "MAIN MENU";

    GAME_INFO("STATE", "Entered GameOverState");
}

void GameOverState::Cleanup() {
    if (hasMusic) {
        StopMusicStream(gameOverMusic);
    }
}

void GameOverState::Update() {
    float dt = GetFrameTime();

    if (hasMusic) {
        UpdateMusicStream(gameOverMusic);
    }

    if (isExitingToMenu) {
        exitFadeTimer += dt;
        if (hasMusic) {
            float progress = exitFadeTimer / exitFadeDuration;
            SetMusicVolume(gameOverMusic, 0.25f * (1.0f - progress));
        }

        if (exitFadeTimer >= exitFadeDuration) {
            GameManager::Instance().ChangeState(new MainMenuState());
            return;
        }
        return;
    }

    sceneTimer += dt;

    // Кнопка стає активною після появи тексту
    if (sceneTimer >= buttonAppearDelay) {
        Vector2 mousePos = GetMousePosition();
        menuButton.isHovered = CheckCollisionPointRec(mousePos, menuButton.bounds);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && menuButton.isHovered) {
            isExitingToMenu = true;
            exitFadeTimer = 0.0f;
        }
    }
}

void GameOverState::Draw() {
    ClearBackground(BLACK);

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float fadeProgress = sceneTimer / fadeInDuration;
    if (fadeProgress > 1.0f) fadeProgress = 1.0f;

    // 1. Фон (текстура або темно-бордова віньєтка)
    if (hasBackground) {
        DrawTexturePro(
            bgTexture,
            Rectangle{ 0, 0, (float)bgTexture.width, (float)bgTexture.height },
            Rectangle{ 0, 0, screenW, screenH },
            Vector2{ 0, 0 },
            0.0f,
            ColorAlpha(WHITE, fadeProgress)
        );
    }
    else {
        DrawRectangleGradientV(
            0, 0, (int)screenW, (int)screenH,
            BLACK,
            ColorAlpha(MAROON, 0.28f * fadeProgress)
        );
    }

    // 2. Заголовок "GAME OVER" з легким тремтінням та мерехтінням
    const char* titleText = "GAME OVER";
    int titleSize = 76;
    int titleWidth = MeasureText(titleText, titleSize);

    float jitterX = 0.0f;
    float jitterY = 0.0f;
    if (sceneTimer < 0.8f) {
        jitterX = (float)GetRandomValue(-4, 4);
        jitterY = (float)GetRandomValue(-3, 3);
    }

    float flicker = 0.85f + 0.15f * sinf(sceneTimer * 6.0f);
    Color titleColor = ColorAlpha(RED, fadeProgress * flicker);

    DrawText(
        titleText,
        (int)((screenW - titleWidth) * 0.5f + jitterX),
        (int)(screenH * 0.30f + jitterY),
        titleSize,
        titleColor
    );

    // Підзаголовок
    const char* subText = "The house always wins...";
    int subSize = 24;
    int subWidth = MeasureText(subText, subSize);
    DrawText(
        subText,
        (int)((screenW - subWidth) * 0.5f),
        (int)(screenH * 0.30f + 90.0f),
        subSize,
        ColorAlpha(LIGHTGRAY, fadeProgress * 0.8f)
    );

    // 3. Кнопка повернення в головне меню
    if (sceneTimer >= buttonAppearDelay) {
        float btnAlpha = (sceneTimer - buttonAppearDelay) / 0.6f;
        if (btnAlpha > 1.0f) btnAlpha = 1.0f;
        DrawButton(menuButton, btnAlpha);
    }

    // 4. Плавне затемнення при виході в меню
    if (isExitingToMenu) {
        float outAlpha = exitFadeTimer / exitFadeDuration;
        if (outAlpha > 1.0f) outAlpha = 1.0f;
        DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, outAlpha));
    }
}

void GameOverState::DrawButton(const GameOverButton& btn, float alpha) {
    Color rectColor = btn.isHovered ? ColorAlpha(RED, 0.35f * alpha) : ColorAlpha(DARKGRAY, 0.6f * alpha);
    Color borderColor = btn.isHovered ? ColorAlpha(RED, alpha) : ColorAlpha(GRAY, alpha);
    Color textColor = btn.isHovered ? ColorAlpha(WHITE, alpha) : ColorAlpha(LIGHTGRAY, alpha);

    DrawRectangleRec(btn.bounds, rectColor);
    DrawRectangleLinesEx(btn.bounds, 2.0f, borderColor);

    int fontSize = 24;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = btn.bounds.x + (btn.bounds.width - textWidth) * 0.5f;
    float textY = btn.bounds.y + (btn.bounds.height - fontSize) * 0.5f;

    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);
}