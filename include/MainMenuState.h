#pragma once
#pragma once

#include "GameState.h"
#include "raylib.h"
#include "AssetManager.h"
#include "config.h"
#include <string>

// Допоміжна структура для кнопок меню
struct MenuButton {
    Rectangle bounds;
    std::string text;
    bool isHovered = false;
};

class MainMenuState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    // ШАБЛОНИ РЕСУРСІВ (Текстура та Музика)
    Texture2D bgTexture = { 0 };
    Music bgMusic = { 0 };
    bool hasBackground = false;
    bool hasMusic = false;


    // --- ПЕРЕХІД І ЗВУК ---
    bool isTransitioning = false;
    float transitionTimer = 0.0f;
    const float transitionDuration = 4.0f; 

    Sound startSound;
    bool hasStartSound = false;

    // Кнопки
    MenuButton playButton;
    MenuButton exitButton;

    // Допоміжний метод малювання кнопок
    void DrawButton(const MenuButton& btn);
};