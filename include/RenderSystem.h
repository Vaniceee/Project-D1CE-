#pragma once

#include "Map.h"
#include "Player.h"
#include "Guard.h"
#include <vector>

struct FloorTutorialHint {
    Vector2 anchorPos = { 0.0f, 0.0f };
    float moveAlpha = 1.0f;
    float sprintAlpha = 1.0f;
    float time = 0.0f;
};

struct RenderItem {
    float sortY;
    int order;
    enum class Type { MAP_OBJECT, PLAYER, GUARD } type;
    const MapVisualObject* mapObj = nullptr;
};

class RenderSystem {
public:
    // Малює весь ігровий світ у межах активної 2D-камери
    void RenderWorld(const GameMap& gameMap, Player* player, Guard* guard, const FloorTutorialHint* floorHint = nullptr);

private:
    std::vector<RenderItem> renderQueue;
    void DrawFloorTutorialHint(const FloorTutorialHint& hint) const;
    void DrawObjectShadows(const std::vector<MapVisualObject>& mapObjects) const;
};