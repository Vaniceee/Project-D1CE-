#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

struct TilesetInfo {
    int firstgid = 1;
    std::string name;
    Texture2D texture = { 0 }; // Для звичайних спрайтшитів (папка tiles)
    int tileWidth = 32;
    int tileHeight = 32;

    // Для Collection of Images (папка objects): локальний ID -> окрема текстура
    std::unordered_map<int, Texture2D> individualTextures;
};

struct MapVisualObject {
    uint32_t rawGid = 0;
    float x = 0.0f;
    float y = 0.0f; // У Tiled для тайл-об'єктів це НИЖНІЙ лівий кут
    float width = 0.0f;
    float height = 0.0f;
    int layerOrder = 0; // Порядок шару для об'єктів з однаковим Y
};

struct RenderItem {
    float sortY;
    int order;
    enum class Type { MAP_OBJECT, PLAYER, GUARD } type;
    const MapVisualObject* mapObj = nullptr;
};

class GameMap {
public:
    bool Load(const std::string& mapPath, const std::string& defaultTilesetKey = "");
    void Unload();

    void DrawBelowPlayer();
    void DrawAbovePlayer();
    void DrawVisualObject(const MapVisualObject& obj) const;

    bool CheckWallCollision(Rectangle playerHitbox) const;
    void UpdateDoorTriggers(Rectangle playerHitbox);

    int GetWidth() const { return mapWidth; }
    int GetHeight() const { return mapHeight; }
    int GetTileWidth() const { return tileWidth; }
    int GetTileHeight() const { return tileHeight; }
    const std::string& GetCurrentDoorMessage() const { return currentDoorMessage; }
    const std::vector<MapVisualObject>& GetVisualObjects() const { return visualObjects; }

    Vector2 GetPlayerSpawn() const { return playerSpawn; }
    const std::vector<Vector2>& GetGuardSpawns() const { return guardSpawns; }

private:
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

    std::string currentDoorMessage = "";

    Vector2 playerSpawn = { 1944.0f, 2464.0f }; // Значення за замовчуванням
    std::vector<Vector2> guardSpawns;

    void ParseSpawnLayer(const std::string& content, const std::string& layerName);

    std::vector<int> ParseCSV(const std::string& content, const std::string& layerName);
    void ParseObjectLayer(const std::string& content, const std::string& layerName, std::vector<Rectangle>& rects, std::vector<std::string>* names);
    void ParseVisualObjectLayer(const std::string& content, const std::string& layerName, int layerOrder);
    void DrawSingleLayer(const std::vector<int>& layerData);
};