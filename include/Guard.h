#pragma once
#include "Entity.h"
#include "Player.h"
#include "raylib.h"

class GameMap;

enum class GuardState {
    PATROL,
    INVESTIGATE,
    CHASE
};

class Guard : public Entity {
public:
    Guard(Vector2 startPos, float startSpeed = 120.0f, float width = 32.0f, float height = 64.0f);

    void Update() override {}
    void Update(Player& player, const GameMap& gameMap, float dt);
    void Draw() override;

    GuardState GetCurrentState() const { return currentState; }
    Rectangle GetFootCollider() const;

private:
    GuardState currentState = GuardState::PATROL;
    float scale = 1.0f;

    // Параметри виявлення та атаки
    float detectionRadius = 200.0f;
    float attackRadius = 42.0f;

    // Механіка КД та сповільнення
    float attackCooldownTimer = 0.0f;
    float attackCooldownDuration = 1.8f;
    float slowdownFactor = 0.25f;

    // Фіксація напрямку виштовхування з кутів (як у Player)
    int activeNudgeY = 0;
    int activeNudgeX = 0;

    // Розміри коллайдера ніг
    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;

    void RecalculateFootCollider();
    Vector2 GetFootCenter() const;

    // Універсальний осьовий рух із колізіями та обходом кутів
    void MoveWithCollisions(Vector2 moveDir, float moveStep, const GameMap& gameMap);

    // Поведінка станів (FSM)
    void UpdatePatrolState(const GameMap& gameMap, float dt);
    void UpdateInvestigateState(const GameMap& gameMap, float dt);
    void UpdateChaseState(Player& player, const GameMap& gameMap, float dt);
};