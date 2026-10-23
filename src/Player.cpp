#include "Player.h"
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

void Player::Update() {
    Vector2 moveDir = InputManager::GetMovementDirection();
    float deltaTime = GetFrameTime();
    float frameSpeed = speed * deltaTime * 60.0f;

    // --- ПОЕТАПНИЙ РУХ (SEPARATED-AXIS MOVEMENT) ДЛЯ КОВЗАННЯ ---

    // 1. Рух та перевірка по осі X
    position.x += moveDir.x * frameSpeed;
    collider.x = position.x + footOffsetX;

    // Перевірка меж екрана віртуальний холст 
    Collisions::ClampToBounds(collider, position, 1280.0f, 720.0f);
    
    position.x = collider.x - footOffsetX;

    // (Тут у майбутньому буде: якщо є зіткнення зі стіною мапи по X -> відкотити position.x)


    // 2. Рух та перевірка по осі Y
    position.y += moveDir.y * frameSpeed;
    collider.y = position.y + footOffsetY;

    // Перевірка меж екрана
    Collisions::ClampToBounds(collider, position, 1280.0f, 720.0f);
    position.y = collider.y - footOffsetY;

    // (Тут у майбутньому буде: якщо є зіткнення зі стіною мапи по Y -> відкотити position.y)


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