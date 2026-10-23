#include "Collisions.h"

void Collisions::ClampToBounds(Rectangle& collider, Vector2& position, float canvasWidth, float canvasHeight) {
    // ќбмежуЇмо коллайдер у межах екрана, а пот≥м синхрон≥зуЇмо позиц≥ю sprite/entity
    if (collider.x < 0.0f) {
        collider.x = 0.0f;
    }
    if (collider.x + collider.width > canvasWidth) {
        collider.x = canvasWidth - collider.width;
    }

    if (collider.y < 0.0f) {
        collider.y = 0.0f;
    }
    if (collider.y + collider.height > canvasHeight) {
        collider.y = canvasHeight - collider.height;
    }
}