#pragma once

#include "Map.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "AssetManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

class MapParser {
public:
    // Головний метод: парсить файл мапи і заповнює структуру MapData
    static bool Parse(const std::string& mapPath, const std::string& defaultTilesetKey, MapData& outData);

private:
    static std::string ToLower(std::string str);
    static std::string ExtractAssetKey(const std::string& rawPath);

    static void ParseTilesets(const std::string& content, const std::string& mapDir, const std::string& defaultTilesetKey, MapData& outData);
    static void ParseTileLayers(const std::string& content, MapData& outData);
    static void ParseObjectLayers(const std::string& content, MapData& outData);

    static std::vector<int> ParseCSV(const std::string& content, const std::string& layerName);
    static void ParseSpawnLayer(const std::string& content, const std::string& layerName, MapData& outData);
    static void ParseVisualObjectLayer(const std::string& content, const std::string& layerName, int layerOrder, MapData& outData);
    static void ParseRectLayer(
        const std::string& content,
        const std::string& layerName,
        std::vector<Rectangle>& rects,
        std::vector<std::string>* names,
        std::unordered_map<std::string, Rectangle>* targets = nullptr,
        std::vector<InteractableObject>* interactables = nullptr
    );
};