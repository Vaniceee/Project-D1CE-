#include "Entity.h"

Entity::Entity(Vector2 startPos, float startSpeed, float width, float height)
    : position(startPos), speed(startSpeed), velocity({ 0.0f, 0.0f }) {
    collider = { startPos.x,startPos.y,width,height };
}

Vector2 Entity::GetPosition() const {
    return position;
}
void Entity::SetPosition(Vector2 newPos) {
    position = newPos;
    collider.x = position.x;
    collider.y = position.y;
}
Rectangle Entity::GetCollider() const {
    return collider;
}