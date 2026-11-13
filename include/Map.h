#pragma once

#include "raylib.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct TilesetInfo {
    int firstgid = 1;
    std::string name;
    Texture2D texture = { 0 };
    int tileWidth = 32;
    int tileHeight = 32;
    std::unordered_map<int, Texture2D> individualTextures;
};

struct MapVisualObject {
    uint32_t rawGid = 0;
    float x = 0.0f;
    float y = 0.0f; // У Tiled для тайл-об'єктів це НИЖНІЙ лівий кут
    float width = 0.0f;
    float height = 0.0f;
    int layerOrder = 0;
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
    std::vector<Rectangle> collisionRects;
    std::vector<Rectangle> doorTriggers;
    std::vector<std::string> doorNumbers;
    std::vector<MapVisualObject> visualObjects;

    Vector2 playerSpawn = { 1944.0f, 2464.0f };
    std::vector<Vector2> guardSpawns;
};

class GameMap {
public:
    bool Load(const std::string& mapPath, const std::string& defaultTilesetKey = "");
    void Unload();

    // Рендеринг
    void DrawBelowPlayer() const;
    void DrawAbovePlayer() const;
    void DrawVisualObject(const MapVisualObject& obj) const;

    // Ігрова логіка
    bool CheckWallCollision(Rectangle playerHitbox) const;
    void UpdateDoorTriggers(Rectangle playerHitbox);

    // Гетери
    int GetWidth() const { return data.mapWidth; }
    int GetHeight() const { return data.mapHeight; }
    int GetTileWidth() const { return data.tileWidth; }
    int GetTileHeight() const { return data.tileHeight; }
    const std::string& GetCurrentDoorMessage() const { return currentDoorMessage; }
    const std::vector<MapVisualObject>& GetVisualObjects() const { return data.visualObjects; }
    Vector2 GetPlayerSpawn() const { return data.playerSpawn; }
    const std::vector<Vector2>& GetGuardSpawns() const { return data.guardSpawns; }

private:
    MapData data;
    std::string currentDoorMessage = "";

    void DrawSingleLayer(const std::vector<int>& layerData) const;
};