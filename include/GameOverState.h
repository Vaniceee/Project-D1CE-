#pragma once

#include "GameState.h"
#include "raylib.h"
#include <string>

struct GameOverButton {
    Rectangle bounds;
    std::string text;
    bool isHovered = false;
};

class GameOverState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    // --- ШАБЛОНИ РЕСУРСІВ (заміниш шляхи, коли додаси ассети) ---
    Texture2D bgTexture = { 0 };
    Sound gameOverSound = { 0 };
    Music gameOverMusic = { 0 };
    bool hasBackground = false;
    bool hasSound = false;
    bool hasMusic = false;

    // --- АНІМАЦІЯ СЦЕНИ GAME OVER ---
    float sceneTimer = 0.0f;
    float fadeInDuration = 1.5f;      // Плавна поява екрана з темряви
    float buttonAppearDelay = 1.2f;   // Коли з'являється кнопка меню

    // --- ПЕРЕХІД В ГОЛОВНЕ МЕНЮ ---
    bool isExitingToMenu = false;
    float exitFadeTimer = 0.0f;
    float exitFadeDuration = 1.0f;

    GameOverButton menuButton;

    void DrawButton(const GameOverButton& btn, float alpha);
};
