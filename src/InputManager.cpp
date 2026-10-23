#include "InputManager.h"
#include <cmath>

static PlayerDirection s_LastDirection = PlayerDirection::Down;

Vector2 InputManager::GetMovementDirection() {
    Vector2 direction = { 0.0f, 0.0f };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    direction.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  direction.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  direction.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) direction.x += 1.0f;

    // Фіксація напрямку для анімацій
    if (direction.y < 0.0f) s_LastDirection = PlayerDirection::Up;
    else if (direction.y > 0.0f) s_LastDirection = PlayerDirection::Down;
    else if (direction.x < 0.0f) s_LastDirection = PlayerDirection::Left;
    else if (direction.x > 0.0f) s_LastDirection = PlayerDirection::Right;

    // Нормалізація діагоналей (щоб швидкість по діагоналі не була вищою)
    if (direction.x != 0.0f && direction.y != 0.0f) {
        float normalizationFactor = 0.7071f; // приблизно 1 / sqrt(2)
        direction.x *= normalizationFactor;
        direction.y *= normalizationFactor;
    }

    return direction;
}

PlayerDirection InputManager::GetLastDirection() {
    return s_LastDirection;
}

bool InputManager::IsInteractPressed() {
    // Шаблон події взаємодії (клавіша E)
    return IsKeyPressed(KEY_E);
}