#pragma once

#include "Map.h"
#include "Player.h"
#include "Guard.h"
#include <vector>

struct RenderItem {
    float sortY;
    int order;
    enum class Type { MAP_OBJECT, PLAYER, GUARD } type;
    const MapVisualObject* mapObj = nullptr;
};

class RenderSystem {
public:
    // Малює весь ігровий світ у межах активної 2D-камери
    void RenderWorld(const GameMap& gameMap, Player* player, Guard* guard);

private:
    std::vector<RenderItem> renderQueue;
};