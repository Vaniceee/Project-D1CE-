#pragma once
#include "Entity.h"
#include "InputManager.h"

class Player : public Entity {
public:
    Player(Vector2 startPos, float startSpeed, float width, float height);

    void Update() override;
    void Draw() override;

    PlayerDirection GetCurrentDirection() const;

private:
    float currentSpeed;

    // Розміри та зсув для коллайдера в ногах
    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;

    Rectangle GetFootCollider() const;
};