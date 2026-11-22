#pragma once

#include "Map.h"
#include "AssetManager.h"
#include "Logger.h"

#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

class MapParser {
public:
    static bool Parse(const std::string& mapPath, const std::string& defaultTilesetKey, MapData& outData);
};