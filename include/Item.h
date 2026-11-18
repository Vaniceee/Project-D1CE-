#pragma once

#include "raylib.h"
#include <string>

class Item {
protected:
    Vector2 position;
    Texture2D texture;
    std::string name;
    bool isPickedUp;

public:
    Item(Vector2 startPos, const std::string& itemName);
    virtual ~Item() = default;

    virtual void Draw() = 0;

    // Getters and setters
    Vector2 GetPosition() const;
    void SetPosition(Vector2 newPos);

    std::string GetName() const;
    bool IsPickedUp() const;
    void SetPickedUp(bool pickedUp);
};
