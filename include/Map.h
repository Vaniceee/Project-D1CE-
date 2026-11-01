#pragma once

#include "raylib.h"
#include <vector>
#include <string>

// Структура для опису кожного тайлсету на мапі
struct TilesetInfo {
    int firstgid = 1;
    std::string name;
    Texture2D texture;
    int tileWidth = 32;
    int tileHeight = 32;
};

class GameMap {
public:
    // Основні функції завантаження та вивантаження
    bool Load(const std::string& mapPath, const std::string& defaultTilesetKey = "");
    void Unload();

    // Функції рендерингу (малювання)
    void DrawBelowPlayer();
    void DrawAbovePlayer();

    // Перевірки ігровій логіки (колізії та тригери)
    bool CheckWallCollision(Rectangle playerHitbox);
    void UpdateDoorTriggers(Rectangle playerHitbox);

    // Гетери для отримання інформації про мапу
    int GetWidth() const { return mapWidth; }
    int GetHeight() const { return mapHeight; }
    int GetTileWidth() const { return tileWidth; }
    int GetTileHeight() const { return tileHeight; }
    const std::string& GetCurrentDoorMessage() const { return currentDoorMessage; }

private:
    // Параметри сітки карти
    int mapWidth = 0;
    int mapHeight = 0;
    int tileWidth = 32;
    int tileHeight = 32;

    // Колекції шарів та об'єктів
    std::vector<TilesetInfo> tilesets;
    std::vector<std::vector<int>> belowPlayerLayers;
    std::vector<std::vector<int>> abovePlayerLayers;
    std::vector<Rectangle> collisionRects;
    std::vector<Rectangle> doorTriggers;
    std::vector<std::string> doorNumbers;

    std::string currentDoorMessage = "";

    // Внутрішні допоміжні методи парсингу
    std::vector<int> ParseCSV(const std::string& content, const std::string& layerName);
    void ParseObjectLayer(const std::string& content, const std::string& layerName, std::vector<Rectangle>& rects, std::vector<std::string>* names);
    void DrawSingleLayer(const std::vector<int>& layerData);
};