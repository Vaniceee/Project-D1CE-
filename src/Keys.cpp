#include "Keys.h"

RoomKey::RoomKey(Vector2 startPos)
    : Item(startPos, "RoomKey") {
}

void RoomKey::Draw() {
    // Пізніше тут буде відображення текстури ключа
}


SafeKeyCard::SafeKeyCard(Vector2 startPos)
    : Item(startPos, "SafeKeyCard") {
}

void SafeKeyCard::Draw() {
    // Пізніше тут буде відображення текстури картки
}


MasterKey::MasterKey(Vector2 startPos)
    : Item(startPos, "MasterKey") {
}

void MasterKey::Draw() {
    // Пізніше тут буде відображення текстури головного ключа
}
