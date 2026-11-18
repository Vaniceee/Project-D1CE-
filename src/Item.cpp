#include "Item.h"

Item::Item(Vector2 startPos, const std::string& itemName)
    : position(startPos),
    texture{ 0 },
    name(itemName),
    isPickedUp(false) {
}

Vector2 Item::GetPosition() const {
    return position;
}

void Item::SetPosition(Vector2 newPos) {
    position = newPos;
}

std::string Item::GetName() const {
    return name;
}

bool Item::IsPickedUp() const {
    return isPickedUp;
}

void Item::SetPickedUp(bool pickedUp) {
    isPickedUp = pickedUp;
}
