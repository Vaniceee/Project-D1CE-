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
    static PlayerDirection GetLastDirection();
    static bool IsInteractPressed();
    static bool IsSprintPressed();
    static bool IsDebugTogglePressed();
    static bool IsPausePressed();
};
