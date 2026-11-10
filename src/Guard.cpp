#include "Guard.h"
#include "raymath.h"

Guard::Guard(Vector2 startPos, float startSpeed, float width, float height)
    : Entity(startPos, startSpeed, width, height) {
    RecalculateFootCollider();
}

void Guard::RecalculateFootCollider() {
    float scaledWidth = collider.width * scale;
    float scaledHeight = collider.height * scale;

    footBoxWidth = scaledWidth * 0.6f;
    footBoxHeight = scaledHeight * 0.25f;

    footOffsetX = (scaledWidth - footBoxWidth) * 0.5f;
    footOffsetY = scaledHeight - footBoxHeight;
}

Rectangle Guard::GetFootCollider() const {
    return { position.x + footOffsetX, position.y + footOffsetY, footBoxWidth, footBoxHeight };
}

Vector2 Guard::GetFootCenter() const {
    Rectangle foot = GetFootCollider();
    return { foot.x + foot.width / 2.0f, foot.y + foot.height / 2.0f };
}

void Guard::Update(Player& player, float dt) {
    // 1. Оновлення таймера перезарядки НПС
    if (attackCooldownTimer > 0.0f) {
        attackCooldownTimer -= dt;
        if (attackCooldownTimer < 0.0f) attackCooldownTimer = 0.0f;
    }

    // 2. Розрахунок відстані МІЖ ЦЕНТРАМИ
    Vector2 guardFootCenter = GetFootCenter();
    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerFootCenter = { playerFoot.x + playerFoot.width / 2.0f, playerFoot.y + playerFoot.height / 2.0f };

    float distToPlayer = Vector2Distance(guardFootCenter, playerFootCenter);

    // Проста перевірка виявлення (Заглушка FSM):
    
    if (distToPlayer <= detectionRadius) {
        currentState = GuardState::CHASE;
    }
    else {
        currentState = GuardState::PATROL;
    }

    // Виконання поточної поведінки FSM
    switch (currentState) {
    case GuardState::PATROL:
        UpdatePatrolState(dt);
        break;
    case GuardState::INVESTIGATE:
        UpdateInvestigateState(dt);
        break;
    case GuardState::CHASE:
        UpdateChaseState(player, dt);
        break;
    }

    // Нанесення шкоди при базі (без окремого стану атаки)
    if (distToPlayer <= attackRadius && attackCooldownTimer <= 0.0f) {
        if (!player.IsInvincible()) {
            player.TakeDamage(1);
            attackCooldownTimer = attackCooldownDuration; 
        }
    }

    // Оновлення хитбокса
    collider.x = position.x + footOffsetX;
    collider.y = position.y + footOffsetY;
}

void Guard::UpdatePatrolState(float dt) {
    // Тимчасова заглушка: НПС стоїть на місці у патрулі
}

void Guard::UpdateInvestigateState(float dt) {
    // Тимчасова заглушка для пошуку шуму
}

void Guard::UpdateChaseState(Player& player, float dt) {
    Vector2 guardFootCenter = GetFootCenter();
    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerFootCenter = { playerFoot.x + playerFoot.width / 2.0f, playerFoot.y + playerFoot.height / 2.0f };

    Vector2 dir = Vector2Subtract(playerFootCenter, guardFootCenter);
    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);

        // Якщо НПС на кулдауні — швидкість падає до 25% (slowdownFactor)
        float currentMoveSpeed = (attackCooldownTimer > 0.0f) ? (speed * slowdownFactor) : speed;

        position.x += dir.x * currentMoveSpeed * dt;
        position.y += dir.y * currentMoveSpeed * dt;
    }
}

void Guard::Draw() {
    // 1. Червоний прямокутник-заглушка замість текстури спрайта
    Color guardColor = (attackCooldownTimer > 0.0f) ? MAROON : RED;
    DrawRectangleRec({ position.x, position.y, collider.width * scale, collider.height * scale }, guardColor);

    Vector2 center = GetFootCenter();
    DrawCircleLinesV(center, detectionRadius, ColorAlpha(YELLOW, 0.3f));
    DrawCircleLinesV(center, attackRadius, ColorAlpha(RED, 0.6f));

    DrawRectangleLinesEx(GetFootCollider(), 1.0f, MAROON);
}