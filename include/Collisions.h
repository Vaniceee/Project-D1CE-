#pragma once
#include "raylib.h"

class Collisions {
public:
  
    static void ClampToBounds(Rectangle& collider, Vector2& position, float canvasWidth, float canvasHeight);

    // Майбутня заготовка для перевірки колізій зі стінами мапи (Tiled)
    // static bool CheckMapCollision(Rectangle collider, const Map& map);
};
