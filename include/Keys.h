#pragma once

#include "Item.h"

// Ключ від кімнати НПС
class RoomKey : public Item {
public:
    RoomKey(Vector2 startPos);

    void Draw() override;
};

// Картка для відкриття сейфа
class SafeKeyCard : public Item {
public:
    SafeKeyCard(Vector2 startPos);

    void Draw() override;
};

// Головний ключ від вихідних дверей
class MasterKey : public Item {
public:
    MasterKey(Vector2 startPos);

    void Draw() override;
};
