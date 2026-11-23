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
    // Малює весь ігровий світ у межах активної 2D-камери з відсіканням тайлів та ізоляцією кімнат
    void RenderWorld(
        const GameMap& gameMap,
        Player* player,
        Guard* guard,
        const Camera2D& camera,
        int virtualWidth,
        int virtualHeight,
        const FloorTutorialHint* floorHint = nullptr
    );

    int GetLastActiveRoomIndex() const { return lastActiveRoomIndex; }

private:
    std::vector<RenderItem> renderQueue;
    std::vector<const MapVisualObject*> visibleObjectsCache;
    int lastActiveRoomIndex = -1;

    static Rectangle ComputeCameraWorldViewRect(const Camera2D& camera, int virtualWidth, int virtualHeight, float padding = 64.0f);
    void DrawFloorTutorialHint(const FloorTutorialHint& hint) const;
    void DrawObjectShadows(const std::vector<const MapVisualObject*>& visibleObjects) const;
};