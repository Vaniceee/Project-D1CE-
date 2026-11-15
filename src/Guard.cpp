#include "Guard.h"
#include "Map.h"
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

void Guard::MoveWithCollisions(Vector2 moveDir, float moveStep, const GameMap& gameMap) {
    const float nudgeTolerance = 13.0f; // Ïîð³ã ó ï³êñåëÿõ äëÿ îáõîäó êóò³â (ÿê ó Player)

    // =========================================================================
    // 1. ÐÓÕ ÒÀ ÊÎË²Ç²¯ ÏÎ ÎÑ² X (Ç ÂÈØÒÎÂÕÓÂÀÍÍßÌ ÏÎ Y)
    // =========================================================================
    if (moveDir.x != 0.0f) {
        position.x += moveDir.x * moveStep;

        if (gameMap.CheckWallCollision(GetFootCollider())) {
            position.x -= moveDir.x * moveStep;

            if (activeNudgeY == 0) {
                Rectangle testUp = GetFootCollider();
                testUp.y -= nudgeTolerance;
                testUp.x += moveDir.x * moveStep;

                Rectangle testDown = GetFootCollider();
                testDown.y += nudgeTolerance;
                testDown.x += moveDir.x * moveStep;

                bool canPassUp = !gameMap.CheckWallCollision(testUp);
                bool canPassDown = !gameMap.CheckWallCollision(testDown);

                if (canPassUp && !canPassDown) {
                    activeNudgeY = -1;
                }
                else if (canPassDown && !canPassUp) {
                    activeNudgeY = 1;
                }
            }

            if (activeNudgeY == -1) {
                position.y -= moveStep;
                moveDir.y = 0.0f;
            }
            else if (activeNudgeY == 1) {
                position.y += moveStep;
                moveDir.y = 0.0f;
            }
        }
        else {
            activeNudgeY = 0;
        }
    }
    else {
        activeNudgeY = 0;
    }

    // =========================================================================
    // 2. ÐÓÕ ÒÀ ÊÎË²Ç²¯ ÏÎ ÎÑ² Y (Ç ÂÈØÒÎÂÕÓÂÀÍÍßÌ ÏÎ X)
    // =========================================================================
    if (moveDir.y != 0.0f) {
        position.y += moveDir.y * moveStep;

        if (gameMap.CheckWallCollision(GetFootCollider())) {
            position.y -= moveDir.y * moveStep;

            if (activeNudgeX == 0) {
                Rectangle testLeft = GetFootCollider();
                testLeft.x -= nudgeTolerance;
                testLeft.y += moveDir.y * moveStep;

                Rectangle testRight = GetFootCollider();
                testRight.x += nudgeTolerance;
                testRight.y += moveDir.y * moveStep;

                bool canPassLeft = !gameMap.CheckWallCollision(testLeft);
                bool canPassRight = !gameMap.CheckWallCollision(testRight);

                if (canPassLeft && !canPassRight) {
                    activeNudgeX = -1;
                }
                else if (canPassRight && !canPassLeft) {
                    activeNudgeX = 1;
                }
            }

            if (activeNudgeX == -1) {
                position.x -= moveStep;
                moveDir.x = 0.0f;
            }
            else if (activeNudgeX == 1) {
                position.x += moveStep;
                moveDir.x = 0.0f;
            }
        }
        else {
            activeNudgeX = 0;
        }
    }
    else {
        activeNudgeX = 0;
    }
}

void Guard::Update(Player& player, const GameMap& gameMap, float dt) {
    if (attackCooldownTimer > 0.0f) {
        attackCooldownTimer -= dt;
        if (attackCooldownTimer < 0.0f) attackCooldownTimer = 0.0f;
    }

    Vector2 guardFootCenter = GetFootCenter();
    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerFootCenter = { playerFoot.x + playerFoot.width / 2.0f, playerFoot.y + playerFoot.height / 2.0f };

    float distToPlayer = Vector2Distance(guardFootCenter, playerFootCenter);

    if (distToPlayer <= detectionRadius) {
        currentState = GuardState::CHASE;
    }
    else {
        currentState = GuardState::PATROL;
    }

    switch (currentState) {
    case GuardState::PATROL:
        UpdatePatrolState(gameMap, dt);
        break;
    case GuardState::INVESTIGATE:
        UpdateInvestigateState(gameMap, dt);
        break;
    case GuardState::CHASE:
        UpdateChaseState(player, gameMap, dt);
        break;
    }

    if (distToPlayer <= attackRadius && attackCooldownTimer <= 0.0f) {
        if (!player.IsInvincible()) {
            player.TakeDamage(1);
            attackCooldownTimer = attackCooldownDuration;
        }
    }

    collider.x = position.x + footOffsetX;
    collider.y = position.y + footOffsetY;
}

void Guard::UpdatePatrolState(const GameMap& gameMap, float dt) {
    // Çàãëóøêà: êîëè äîäàìî ïàòðóëü, ðóõàòèìåìîñü ÷åðåç MoveWithCollisions(...)
}

void Guard::UpdateInvestigateState(const GameMap& gameMap, float dt) {
    // Çàãëóøêà: êîëè äîäàìî ðîçñë³äóâàííÿ øóìó, ðóõàòèìåìîñü ÷åðåç MoveWithCollisions(...)
}

void Guard::UpdateChaseState(Player& player, const GameMap& gameMap, float dt) {
    Vector2 guardFootCenter = GetFootCenter();
    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerFootCenter = { playerFoot.x + playerFoot.width / 2.0f, playerFoot.y + playerFoot.height / 2.0f };

    Vector2 dir = Vector2Subtract(playerFootCenter, guardFootCenter);
    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);

        float currentMoveSpeed = (attackCooldownTimer > 0.0f) ? (speed * slowdownFactor) : speed;
        float moveStep = currentMoveSpeed * dt;

        MoveWithCollisions(dir, moveStep, gameMap);
    }
}

void Guard::Draw() {
    Color guardColor = (attackCooldownTimer > 0.0f) ? MAROON : RED;
    DrawRectangleRec({ position.x, position.y, collider.width * scale, collider.height * scale }, guardColor);

    Vector2 center = GetFootCenter();
    DrawCircleLinesV(center, detectionRadius, ColorAlpha(YELLOW, 0.3f));
    DrawCircleLinesV(center, attackRadius, ColorAlpha(RED, 0.6f));

    DrawRectangleLinesEx(GetFootCollider(), 1.0f, MAROON);
}