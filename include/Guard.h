#pragma once
#include "Entity.h"
#include "Player.h"
#include "raylib.h"

enum class GuardState {
    PATROL,
    INVESTIGATE,
    CHASE
};

class Guard : public Entity {
public:
    Guard(Vector2 startPos, float startSpeed = 120.0f, float width = 32.0f, float height = 64.0f);

    void Update() override {}
    void Update(Player& player, float dt);
    void Draw() override;

    GuardState GetCurrentState() const { return currentState; }
    Rectangle GetFootCollider() const;

private:
    GuardState currentState = GuardState::PATROL;
    float scale = 1.0f;

    // Параметри виявлення та атаки
    float detectionRadius = 200.0f; // Радіус виявлення гравця
    float attackRadius = 42.0f;     // Радіус нанесення шкоди


    // --- МЕХАНІКА КД ТА СПОВІЛЬНЕННЯ ---
    float attackCooldownTimer = 0.0f;
    float attackCooldownDuration = 1.8f; // КД на наступний удар 
    float slowdownFactor = 0.25f;

    // Розміри коллайдера ніг (для майбутніх колізій зі стінами)
    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;

    void RecalculateFootCollider();
    Vector2 GetFootCenter() const;

    // Заглушки під логіку станів (FSM)
    void UpdatePatrolState(float dt);
    void UpdateInvestigateState(float dt);
    void UpdateChaseState(Player& player, float dt);
};
