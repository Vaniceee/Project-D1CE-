#include "Player.h"
#include "Map.h"
#include "Collisions.h"
#include "raylib.h"

Player::Player(Vector2 startPos, float startSpeed, float width, float height)
    : Entity(startPos, startSpeed, width, height), currentSpeed(startSpeed) {

    // Налаштування хитбокса в ногах:
    footBoxWidth = width * 0.6f;   
    footBoxHeight = height * 0.3f; 

    footOffsetX = (width - footBoxWidth) * 0.5f;   
    footOffsetY = height - footBoxHeight;         

    // Ініціалізуємо коллайдер
    collider = { position.x + footOffsetX, position.y + footOffsetY, footBoxWidth, footBoxHeight };
}

Rectangle Player::GetFootCollider() const {
    return { position.x + footOffsetX, position.y + footOffsetY, footBoxWidth, footBoxHeight };
}

void Player::Update(const GameMap& gameMap) {
    Vector2 moveDir = InputManager::GetMovementDirection();
    float deltaTime = GetFrameTime();



    // ---- СПРИНТ-------------
    bool isMoving = (moveDir.x != 0.0f or moveDir.y != 0.0f);
    float currentMoveSpeed = speed;
    bool wantsToSprint = InputManager::IsSprintPressed() && isMoving && !isExhausted;

    if (wantsToSprint) {

        currentMoveSpeed *= 1.8f;
        currentStamina -= staminaDrainRate * deltaTime;
        if (currentStamina <= 0.0f) {
            currentStamina = 0.0f;
            isExhausted = true;
        }
    }
    else {
        currentStamina += staminaRegenRate * deltaTime;
        if (currentStamina >= maxStamina) {
            currentStamina = maxStamina;
        }
        float minStaminaToSprint = maxStamina * 0.4f;
        if (isExhausted && currentStamina >= minStaminaToSprint) {
            isExhausted = false;
        }
    }


    

    // --- ПОЕТАПНИЙ РУХ (SEPARATED-AXIS MOVEMENT) ДЛЯ КОВЗАННЯ ---
    float frameSpeed = currentMoveSpeed * deltaTime;
    const float nudgeTolerance = 13.0f; // Поріг у пікселях для автоматичного виштовхування з кутів

    // =========================================================================
    // 1. РУХ ТА КОЛІЗІЇ ПО ОСІ X (З ВИШТОВХУВАННЯМ ПО Y)
    // =========================================================================
    if (moveDir.x != 0.0f) {
        position.x += moveDir.x * frameSpeed;

        if (gameMap.CheckWallCollision(GetFootCollider())) {
            // Скасовуємо спробу руху по X
            position.x -= moveDir.x * frameSpeed;

            // ЯКЩО СТАН ЩЕ НЕ ЗАФІКСОВАНО: шукаємо напрямок виштовхування
            if (activeNudgeY == 0) {
                Rectangle testUp = GetFootCollider();
                testUp.y -= nudgeTolerance;
                testUp.x += moveDir.x * frameSpeed;

                Rectangle testDown = GetFootCollider();
                testDown.y += nudgeTolerance;
                testDown.x += moveDir.x * frameSpeed;

                bool canPassUp = !gameMap.CheckWallCollision(testUp);
                bool canPassDown = !gameMap.CheckWallCollision(testDown);

                if (canPassUp && !canPassDown) {
                    activeNudgeY = -1; // Блокуємо стан: тільки ВГОРУ
                }
                else if (canPassDown && !canPassUp) {
                    activeNudgeY = 1;  // Блокуємо стан: тільки ВНИЗ
                }
            }

            // ЗАСТОСУВАННЯ ЗАФІКСОВАНОГО СТАНУ
            if (activeNudgeY == -1) {
                position.y -= frameSpeed; // Примусово виштовхуємо вгору
                moveDir.y = 0.0f;         // Гасимо будь-який інпут вниз
            }
            else if (activeNudgeY == 1) {
                position.y += frameSpeed; // Примусово виштовхуємо вниз
                moveDir.y = 0.0f;         // Гасимо будь-який інпут вгору
            }
        }
        else {
            // Якщо рух по X пройшов без колізій — скидаємо фіксацію стану
            activeNudgeY = 0;
        }
    }
    else {
        // Якщо гравець зупинив рух по X — скидаємо стан
        activeNudgeY = 0;
    }

    // =========================================================================
    // 2. РУХ ТА КОЛІЗІЇ ПО ОСІ Y (З ВИШТОВХУВАННЯМ ПО X)
    // =========================================================================
    if (moveDir.y != 0.0f) {
        position.y += moveDir.y * frameSpeed;

        if (gameMap.CheckWallCollision(GetFootCollider())) {
            // Скасовуємо спробу руху по Y
            position.y -= moveDir.y * frameSpeed;

            // ЯКЩО СТАН ЩЕ НЕ ЗАФІКСОВАНО: шукаємо напрямок виштовхування
            if (activeNudgeX == 0) {
                Rectangle testLeft = GetFootCollider();
                testLeft.x -= nudgeTolerance;
                testLeft.y += moveDir.y * frameSpeed;

                Rectangle testRight = GetFootCollider();
                testRight.x += nudgeTolerance;
                testRight.y += moveDir.y * frameSpeed;

                bool canPassLeft = !gameMap.CheckWallCollision(testLeft);
                bool canPassRight = !gameMap.CheckWallCollision(testRight);

                if (canPassLeft && !canPassRight) {
                    activeNudgeX = -1; // Блокуємо стан: тільки ЛІВОРУЧ
                }
                else if (canPassRight && !canPassLeft) {
                    activeNudgeX = 1;  // Блокуємо стан: тільки ПРАВОРУЧ
                }
            }

            // ЗАСТОСУВАННЯ ЗАФІКСОВАНОГО СТАНУ
            if (activeNudgeX == -1) {
                position.x -= frameSpeed; // Примусово виштовхуємо ліворуч
                moveDir.x = 0.0f;         // Гасимо будь-який інпут праворуч
            }
            else if (activeNudgeX == 1) {
                position.x += frameSpeed; // Примусово виштовхуємо праворуч
                moveDir.x = 0.0f;         // Гасимо будь-який інпут ліворуч
            }
        }
        else {
            // Якщо рух по Y пройшов без колізій — скидаємо фіксацію стану
            activeNudgeX = 0;
        }
    }
    else {
        // Якщо гравець зупинив рух по Y — скидаємо стан
        activeNudgeX = 0;
    }

    // Оновлюємо внутрішній коллайдер після фінального коригування позиції
    collider.x = position.x + footOffsetX;
    collider.y = position.y + footOffsetY;

 // Шаблон взаємодії
    if (InputManager::IsInteractPressed()) {
        // Дія взаємодії
    }

}
   


void Player::Draw() {
    // Малюємо повний габарит гравця (блакитним)
    DrawRectangleRec({ position.x, position.y, collider.width / 0.6f, collider.height / 0.3f }, BLUE);

    // Для відлагодження (debugging) візуально показуємо хитбокс у ногах червоним контуром
    DrawRectangleLinesEx(GetFootCollider(), 1.0f, RED);
}

PlayerDirection Player::GetCurrentDirection() const {
    return InputManager::GetLastDirection();
}