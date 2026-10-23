#pragma once
#include "raylib.h"

enum class PlayerDirection {
    None,
    Up,
    Down,
    Left,
    Right
};

class InputManager {
public:
    
    static Vector2 GetMovementDirection();

    // Останній активний напрямок (для майбутніх анімацій)
    static PlayerDirection GetLastDirection();

    // Шаблон для події взаємодії (наприклад, клавіша E)
    static bool IsInteractPressed();
};
