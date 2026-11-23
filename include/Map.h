#pragma once

#include "raylib.h"
#include "Item.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <memory>

struct TilesetInfo {
    int firstgid = 1;
    std::string name;
    Texture2D texture = { 0 };
    int tileWidth = 32;
    int tileHeight = 32;
    std::unordered_map<int, Texture2D> individualTextures;
    std::unordered_map<int, std::vector<Rectangle>> tileCollisions;
};

enum class VisualCategory {
    FloorDeco,  // Завжди під гравцем і меблями, без тіні та автоколізії
    Furniture,  // Y-сортування по obj.y, є тінь і автоколізія
    SmallDeco,  // Якщо центр низу на Furniture -> sortY = table.sortY + 0.01f * subOrder (без тіні/колізії); інакше -> свій obj.y + тінь + колізія
    WallDeco    // Y-сортування по obj.y, без прив'язки до меблів, без тіні та автоколізії
};

struct MapVisualObject {
    uint32_t rawGid = 0;
    float x = 0.0f;
    float y = 0.0f; // У Tiled для тайл-об'єктів це НИЖНІЙ лівий кут
    float width = 0.0f;
    float height = 0.0f;
    VisualCategory category = VisualCategory::Furniture;
    int layerOrder = 0;
    int subOrder = 0;
    float sortY = 0.0f;
    bool hasShadow = true;
    int roomIndex = -1; // Індекс кімнати з шару "rooms" (-1 якщо поза кімнатами)
};

struct RoomZone {
    std::string name;
    Rectangle bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct InteractableObject {
    std::string fullId;       // Наприклад, "locker_1" або "key_room"
    std::string type;         // Наприклад, "locker", "safe", "terminal" (частина до першого '_')
    Rectangle bounds = { 0 }; // Зона спрацьовування перед об'єктом
    bool isUsed = false;      // Прапорець стану (чи вже взаємодіяли)
    std::shared_ptr<Item> storedItem = nullptr; // Предмет всередині
};

// Просторова сітка (Spatial Grid) для швидкої перевірки колізій O(1) по чанках
struct CollisionSpatialGrid {
    float cellSize = 256.0f;
    int cols = 0;
    int rows = 0;
    std::vector<Rectangle> allCollisionRects;
    std::vector<std::vector<int>> cells;
    mutable std::vector<uint32_t> queryVisited;
    mutable uint32_t queryGeneration = 0;
};

// Контейнер для всіх даних, які наповнює MapParser
struct MapData {
    int mapWidth = 0;
    int mapHeight = 0;
    int tileWidth = 32;
    int tileHeight = 32;

    std::vector<TilesetInfo> tilesets;
    std::vector<std::vector<int>> belowPlayerLayers;
    std::vector<std::vector<int>> abovePlayerLayers;
    std::vector<Rectangle> collisionRects;     // Ручні колізії з окремого шару
    std::vector<Rectangle> autoCollisionRects; // Автоматичні колізії з тайлів та об'єктів
    std::vector<Rectangle> triggers;
    std::vector<std::string> triggerNames;
    std::unordered_map<std::string, Rectangle> teleportTargets;
    std::vector<InteractableObject> interactables;
    std::vector<MapVisualObject> visualObjects;
    std::vector<RoomZone> rooms;               // Прямокутники кімнат із шару "rooms"

    Vector2 playerSpawn = { 1944.0f, 2464.0f };
    std::vector<Vector2> guardSpawns;
    std::vector<Vector2> guardPatrolPoints;
};

class GameMap {
public:
    struct TeleportLink {
        Vector2 triggerCenter;
        Vector2 destinationCenter;
    };

    bool Load(const std::string& mapPath, const std::string& defaultTilesetKey = "");
    void Unload();

    // Визначення кімнати за світовою точкою або прямокутником
    int GetRoomIndexAtPoint(Vector2 worldPos) const;
    int GetRoomIndexAtRect(Rectangle rect) const;
    const std::vector<RoomZone>& GetRooms() const { return data.rooms; }

    // Рендеринг з відсіканням позакадрових тайлів та ізоляцією активної кімнати
    void DrawBelowPlayer(Rectangle viewRect, int activeRoomIndex) const;
    void DrawAbovePlayer(Rectangle viewRect, int activeRoomIndex) const;
    void DrawVisualObject(const MapVisualObject& obj) const;
    void DrawDebug(int activeRoomIndex = -1) const;

    // Ігрова логіка
    void DistributeLoot();
    bool CheckWallCollision(Rectangle playerHitbox) const;
    void UpdateTriggers(Rectangle playerHitbox);
    bool TryGetTeleportDestination(Rectangle playerFootHitbox, Vector2 playerPos, Vector2& outNewPlayerPos) const;
    bool TryGetActiveTriggerCenter(Rectangle footHitbox, Vector2& outTriggerCenter) const;
    std::vector<Vector2> GetAllTriggerCenters() const;
    std::vector<TeleportLink> GetAllTeleportLinks() const;
    InteractableObject* GetInteractableAt(Rectangle footHitbox);

    // Гетери
    int GetWidth() const { return data.mapWidth; }
    int GetHeight() const { return data.mapHeight; }
    int GetTileWidth() const { return data.tileWidth; }
    int GetTileHeight() const { return data.tileHeight; }
    const std::string& GetCurrentTriggerMessage() const { return currentTriggerMessage; }
    const std::vector<MapVisualObject>& GetVisualObjects() const { return data.visualObjects; }
    Vector2 GetPlayerSpawn() const { return data.playerSpawn; }
    const std::vector<Vector2>& GetGuardSpawns() const { return data.guardSpawns; }
    const std::vector<Vector2>& GetGuardPatrolPoints() const { return data.guardPatrolPoints; }

private:
    MapData data;
    CollisionSpatialGrid collisionGrid;
    std::string currentTriggerMessage = "";

    void BuildCollisionGrid();
    void AddCollisionToGrid(const Rectangle& rect);
    void DrawSingleLayer(const std::vector<int>& layerData, Rectangle viewRect, int activeRoomIndex) const;
};