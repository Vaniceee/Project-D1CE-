#pragma once
#include "Entity.h"
#include "InputManager.h"

class Player : public Entity {
public:
    Player(Vector2 startPos, float startSpeed, float width, float height);

    void Update() override;
    void Draw() override;

    PlayerDirection GetCurrentDirection() const;

    Rectangle GetFootCollider() const;

private:
    float currentSpeed;


    // Стаміна параметри
    float maxStamina = 100.0f;
    float currentStamina = 100.0f;
    float staminaDrainRate = 30.0f;
    float staminaRegenRate = 20.0f;
    bool isExhausted = false;


    // Розміри та зсув для коллайдера в ногах
    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;
};