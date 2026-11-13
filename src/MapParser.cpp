#include "MapParser.h"


std::string MapParser::ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}

std::string MapParser::ExtractAssetKey(const std::string& rawPath) {
    size_t lastSlash = rawPath.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos) ? rawPath.substr(lastSlash + 1) : rawPath;
    size_t lastDot = filename.find_last_of('.');
    return (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
}

bool MapParser::Parse(const std::string& mapPath, const std::string& defaultTilesetKey, MapData& outData) {
    std::ifstream file(mapPath);
    if (!file.is_open()) {
        GAME_ERROR("MAP_PARSER", "Could not open map file: %s", mapPath.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    size_t lastMapSlash = mapPath.find_last_of("/\\");
    std::string mapDir = (lastMapSlash != std::string::npos) ? mapPath.substr(0, lastMapSlash + 1) : "";

    auto getIntField = [&](const std::string& key) -> int {
        size_t pos = content.find("\"" + key + "\":");
        if (pos == std::string::npos) return 0;
        size_t startValue = pos + key.length() + 3;
        size_t endValue = content.find_first_of(",}", startValue);
        try {
            return std::stoi(content.substr(startValue, endValue - startValue));
        }
        catch (...) {
            return 0;
        }
        };

    outData.mapWidth = getIntField("width");
    outData.mapHeight = getIntField("height");
    outData.tileWidth = getIntField("tilewidth");
    outData.tileHeight = getIntField("tileheight");

    ParseTilesets(content, mapDir, defaultTilesetKey, outData);
    ParseTileLayers(content, outData);
    ParseObjectLayers(content, outData);

    GAME_INFO("MAP_PARSER", "Map parsed successfully: %s (Size: %dx%d, Tilesets: %zu, Visual Objects: %zu)",
        mapPath.c_str(), outData.mapWidth, outData.mapHeight, outData.tilesets.size(), outData.visualObjects.size());

    return true;
}

void MapParser::ParseTilesets(const std::string& content, const std::string& mapDir, const std::string& defaultTilesetKey, MapData& outData) {
    size_t tsSearchPos = 0;
    while (true) {
        size_t tsPos = content.find("\"firstgid\":", tsSearchPos);
        if (tsPos == std::string::npos) break;

        TilesetInfo ts;
        size_t startVal = tsPos + 11;
        size_t endVal = content.find_first_of(",}", startVal);
        try {
            ts.firstgid = std::stoi(content.substr(startVal, endVal - startVal));
        }
        catch (...) {
            ts.firstgid = 1;
        }

        size_t nextTsPos = content.find("\"firstgid\":", tsPos + 11);
        std::string tsBlock = (nextTsPos != std::string::npos)
            ? content.substr(tsPos, nextTsPos - tsPos)
            : content.substr(tsPos);

        size_t namePos = tsBlock.find("\"name\":\"");
        if (namePos != std::string::npos) {
            size_t nStart = namePos + 8;
            size_t nEnd = tsBlock.find("\"", nStart);
            ts.name = tsBlock.substr(nStart, nEnd - nStart);
        }
        else {
            ts.name = "tileset_" + std::to_string(ts.firstgid);
        }

        auto getFieldInCurrentBlock = [&](const std::string& key) -> int {
            size_t pos = tsBlock.find("\"" + key + "\":");
            if (pos == std::string::npos) return 0;
            size_t startValue = pos + key.length() + 3;
            size_t endValue = tsBlock.find_first_of(",}", startValue);
            try {
                return std::stoi(tsBlock.substr(startValue, endValue - startValue));
            }
            catch (...) {
                return 0;
            }
            };

        int parsedTsW = getFieldInCurrentBlock("tilewidth");
        int parsedTsH = getFieldInCurrentBlock("tileheight");

        ts.tileWidth = (parsedTsW > 0) ? parsedTsW : outData.tileWidth;
        ts.tileHeight = (parsedTsH > 0) ? parsedTsH : outData.tileHeight;

        size_t tilesArrayPos = tsBlock.find("\"tiles\":[");
        if (tilesArrayPos != std::string::npos) {
            size_t tSearch = tilesArrayPos + 9;
            while (true) {
                size_t idPos = tsBlock.find("\"id\":", tSearch);
                if (idPos == std::string::npos) break;

                size_t idStart = idPos + 5;
                size_t idEnd = tsBlock.find_first_of(",}", idStart);
                int localId = 0;
                try {
                    localId = std::stoi(tsBlock.substr(idStart, idEnd - idStart));
                }
                catch (...) {
                    tSearch = idEnd;
                    continue;
                }

                size_t imgPos = tsBlock.find("\"image\":\"", idEnd);
                if (imgPos == std::string::npos) break;

                size_t imgStart = imgPos + 9;
                size_t imgEnd = tsBlock.find("\"", imgStart);
                std::string rawImgPath = tsBlock.substr(imgStart, imgEnd - imgStart);

                std::string key = ts.name + "_" + ExtractAssetKey(rawImgPath);
                Texture2D tex = AssetManager::Instance().GetTexture(key);

                if (tex.id == 0) {
                    std::string cleanPath = rawImgPath;
                    std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

                    if (cleanPath.rfind("assets/", 0) != 0) {
                        cleanPath = mapDir + cleanPath;
                    }

                    AssetManager::Instance().LoadTexture(key, cleanPath);
                    tex = AssetManager::Instance().GetTexture(key);
                }

                if (tex.id != 0) {
                    ts.individualTextures[localId] = tex;
                }
                else {
                    GAME_ERROR("MAP_PARSER", "Failed to load collection image: '%s' (resolved: '%s')",
                        rawImgPath.c_str(), (mapDir + rawImgPath).c_str());
                }

                tSearch = imgEnd;
            }
            GAME_INFO("MAP_PARSER", "Loaded Collection of Images tileset: '%s' (firstgid: %d, items: %zu)",
                ts.name.c_str(), ts.firstgid, ts.individualTextures.size());
        }
        else {
            std::string assetKey = ts.name;
            ts.texture = AssetManager::Instance().GetTexture(assetKey);

            if (ts.texture.id == 0 && !defaultTilesetKey.empty()) {
                assetKey = defaultTilesetKey;
                ts.texture = AssetManager::Instance().GetTexture(assetKey);
            }

            if (ts.texture.id == 0) {
                GAME_ERROR("MAP_PARSER", "Failed to load texture for tileset '%s' (firstgid: %d)", ts.name.c_str(), ts.firstgid);
            }
            else {
                GAME_INFO("MAP_PARSER", "Loaded tileset: '%s' (firstgid: %d, size: %dx%d) using key: '%s'",
                    ts.name.c_str(), ts.firstgid, ts.tileWidth, ts.tileHeight, assetKey.c_str());
            }
        }

        outData.tilesets.push_back(ts);
        tsSearchPos = endVal;
    }

    if (outData.tilesets.empty() && !defaultTilesetKey.empty()) {
        TilesetInfo fallbackTs;
        fallbackTs.firstgid = 1;
        fallbackTs.name = defaultTilesetKey;
        fallbackTs.texture = AssetManager::Instance().GetTexture(defaultTilesetKey);
        fallbackTs.tileWidth = outData.tileWidth;
        fallbackTs.tileHeight = outData.tileHeight;
        outData.tilesets.push_back(fallbackTs);
        GAME_WARN("MAP_PARSER", "No tilesets found in JSON. Using fallback key: '%s'", defaultTilesetKey.c_str());
    }
}

void MapParser::ParseTileLayers(const std::string& content, MapData& outData) {
    size_t searchPos = 0;
    while (true) {
        size_t typePos = content.find("\"type\":\"tilelayer\"", searchPos);
        if (typePos == std::string::npos) break;

        size_t nameKeyPos = content.rfind("\"name\":\"", typePos);
        if (nameKeyPos == std::string::npos) break;

        size_t nameStart = nameKeyPos + 8;
        size_t nameEnd = content.find("\"", nameStart);
        std::string layerName = content.substr(nameStart, nameEnd - nameStart);

        std::vector<int> layerData = ParseCSV(content, layerName);

        std::string lowerName = ToLower(layerName);
        if (lowerName.find("roof") != std::string::npos ||
            lowerName.find("above") != std::string::npos ||
            lowerName.find("top") != std::string::npos)
        {
            outData.abovePlayerLayers.push_back(layerData);
            GAME_DEBUG("MAP_PARSER", "Layer '%s' auto-sorted as ABOVE player", layerName.c_str());
        }
        else {
            outData.belowPlayerLayers.push_back(layerData);
            GAME_DEBUG("MAP_PARSER", "Layer '%s' auto-sorted as BELOW player", layerName.c_str());
        }

        searchPos = typePos + 18;
    }
}

void MapParser::ParseObjectLayers(const std::string& content, MapData& outData) {
    size_t searchPos = 0;
    int visualLayerIndex = 0;

    while (true) {
        size_t objArrayPos = content.find("\"objects\":[", searchPos);
        if (objArrayPos == std::string::npos) break;

        size_t nameKeyPos = content.rfind("\"name\":\"", objArrayPos);
        if (nameKeyPos == std::string::npos) {
            searchPos = objArrayPos + 10;
            continue;
        }

        size_t nameStart = nameKeyPos + 8;
        size_t nameEnd = content.find("\"", nameStart);
        std::string layerName = content.substr(nameStart, nameEnd - nameStart);
        std::string lowerLayerName = ToLower(layerName);

        if (lowerLayerName.find("colli") != std::string::npos ||
            lowerLayerName.find("wall") != std::string::npos ||
            lowerLayerName.find("стін") != std::string::npos)
        {
            ParseRectLayer(content, layerName, outData.collisionRects, nullptr);
            GAME_DEBUG("MAP_PARSER", "Auto-detected COLLISION layer: '%s'", layerName.c_str());
        }
        else if (lowerLayerName.find("trig") != std::string::npos ||
            lowerLayerName.find("door") != std::string::npos ||
            lowerLayerName.find("interact") != std::string::npos ||
            lowerLayerName.find("двер") != std::string::npos)
        {
            ParseRectLayer(content, layerName, outData.doorTriggers, &outData.doorNumbers);
            GAME_DEBUG("MAP_PARSER", "Auto-detected TRIGGER/DOOR layer: '%s'", layerName.c_str());
        }
        else if (lowerLayerName.find("spawn") != std::string::npos ||
            lowerLayerName.find("entit") != std::string::npos)
        {
            ParseSpawnLayer(content, layerName, outData);
            GAME_DEBUG("MAP_PARSER", "Auto-detected SPAWN layer: '%s'", layerName.c_str());
        }
        else {
            ParseVisualObjectLayer(content, layerName, visualLayerIndex++, outData);
            GAME_DEBUG("MAP_PARSER", "Auto-detected VISUAL object layer: '%s'", layerName.c_str());
        }

        searchPos = objArrayPos + 10;
    }
}

std::vector<int> MapParser::ParseCSV(const std::string& content, const std::string& layerName) {
    std::vector<int> result;
    size_t namePos = content.find("\"name\":\"" + layerName + "\"");
    if (namePos == std::string::npos) return result;

    size_t startLayer = content.rfind("{", namePos);
    size_t endLayer = content.find("}", namePos);
    if (startLayer == std::string::npos || endLayer == std::string::npos) return result;

    std::string layerChunks = content.substr(startLayer, endLayer - startLayer);
    size_t dataPos = layerChunks.find("\"data\":[");
    if (dataPos == std::string::npos) return result;

    size_t startIdx = dataPos + 8;
    size_t endIdx = layerChunks.find("]", startIdx);
    if (endIdx == std::string::npos) return result;

    std::string csv = layerChunks.substr(startIdx, endIdx - startIdx);
    std::stringstream ss(csv);
    std::string token;

    while (std::getline(ss, token, ',')) {
        token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) {
            return std::isspace(c);
            }), token.end());

        if (!token.empty()) {
            try {
                result.push_back(static_cast<int>(std::stoul(token)));
            }
            catch (...) {
                result.push_back(0);
            }
        }
    }
    return result;
}

void MapParser::ParseSpawnLayer(const std::string& content, const std::string& layerName, MapData& outData) {
    size_t layerPos = content.find("\"name\":\"" + layerName + "\"");
    if (layerPos == std::string::npos) return;

    size_t objectsPos = content.find("\"objects\":[", layerPos);
    if (objectsPos == std::string::npos) return;

    size_t endObjects = objectsPos + 11;
    int bracketDepth = 1;
    while (endObjects < content.size() && bracketDepth > 0) {
        if (content[endObjects] == '[') bracketDepth++;
        else if (content[endObjects] == ']') bracketDepth--;
        endObjects++;
    }

    std::string objectsText = content.substr(objectsPos, endObjects - objectsPos);
    size_t objStart = objectsText.find("{");

    while (objStart != std::string::npos) {
        size_t objEnd = objStart + 1;
        int braceDepth = 1;
        while (objEnd < objectsText.size() && braceDepth > 0) {
            if (objectsText[objEnd] == '{') braceDepth++;
            else if (objectsText[objEnd] == '}') braceDepth--;
            objEnd++;
        }

        std::string obj = objectsText.substr(objStart, objEnd - objStart);

        auto getFloat = [&](const std::string& key) -> float {
            size_t pos = obj.find("\"" + key + "\":");
            if (pos == std::string::npos) return 0.0f;
            size_t startValue = pos + key.length() + 3;
            size_t endValue = obj.find_first_of(",}", startValue);
            try { return std::stof(obj.substr(startValue, endValue - startValue)); }
            catch (...) { return 0.0f; }
            };

        float x = getFloat("x");
        float y = getFloat("y");

        size_t namePos = obj.find("\"name\":\"");
        if (namePos != std::string::npos) {
            size_t nameStart = namePos + 8;
            size_t nameEnd = obj.find("\"", nameStart);
            std::string spawnName = ToLower(obj.substr(nameStart, nameEnd - nameStart));

            if (spawnName == "player") {
                outData.playerSpawn = { x, y };
                GAME_INFO("MAP_PARSER", "Player spawn set to: (%.1f, %.1f)", x, y);
            }
            else if (spawnName == "guard") {
                outData.guardSpawns.push_back({ x, y });
                GAME_INFO("MAP_PARSER", "Guard spawn added at: (%.1f, %.1f)", x, y);
            }
        }

        objStart = objectsText.find("{", objEnd);
    }
}

void MapParser::ParseVisualObjectLayer(const std::string& content, const std::string& layerName, int layerOrder, MapData& outData) {
    size_t layerPos = content.find("\"name\":\"" + layerName + "\"");
    if (layerPos == std::string::npos) return;

    size_t objectsPos = content.find("\"objects\":[", layerPos);
    if (objectsPos == std::string::npos) return;

    size_t endObjects = objectsPos + 11;
    int bracketDepth = 1;
    while (endObjects < content.size() && bracketDepth > 0) {
        if (content[endObjects] == '[') bracketDepth++;
        else if (content[endObjects] == ']') bracketDepth--;
        endObjects++;
    }

    std::string objectsText = content.substr(objectsPos, endObjects - objectsPos);
    size_t objStart = objectsText.find("{");

    while (objStart != std::string::npos) {
        size_t objEnd = objStart + 1;
        int braceDepth = 1;
        while (objEnd < objectsText.size() && braceDepth > 0) {
            if (objectsText[objEnd] == '{') braceDepth++;
            else if (objectsText[objEnd] == '}') braceDepth--;
            objEnd++;
        }

        std::string obj = objectsText.substr(objStart, objEnd - objStart);

        size_t gidPos = obj.find("\"gid\":");
        if (gidPos != std::string::npos) {
            auto getFloat = [&](const std::string& key) -> float {
                size_t pos = obj.find("\"" + key + "\":");
                if (pos == std::string::npos) return 0.0f;
                size_t startValue = pos + key.length() + 3;
                size_t endValue = obj.find_first_of(",}", startValue);
                try { return std::stof(obj.substr(startValue, endValue - startValue)); }
                catch (...) { return 0.0f; }
                };

            size_t startVal = gidPos + 6;
            size_t endVal = obj.find_first_of(",}", startVal);

            MapVisualObject vObj;
            try {
                vObj.rawGid = static_cast<uint32_t>(std::stoul(obj.substr(startVal, endVal - startVal)));
            }
            catch (...) {
                vObj.rawGid = 0;
            }

            if (vObj.rawGid != 0) {
                vObj.x = getFloat("x");
                vObj.y = getFloat("y");
                vObj.width = getFloat("width");
                vObj.height = getFloat("height");
                vObj.layerOrder = layerOrder;
                outData.visualObjects.push_back(vObj);
            }
        }

        objStart = objectsText.find("{", objEnd);
    }
}

void MapParser::ParseRectLayer(const std::string& content, const std::string& layerName, std::vector<Rectangle>& rects, std::vector<std::string>* names) {
    size_t layerPos = content.find("\"name\":\"" + layerName + "\"");
    if (layerPos == std::string::npos) return;

    size_t objectsPos = content.find("\"objects\":[", layerPos);
    if (objectsPos == std::string::npos) return;

    size_t endObjects = objectsPos + 11;
    int bracketDepth = 1;
    while (endObjects < content.size() && bracketDepth > 0) {
        if (content[endObjects] == '[') bracketDepth++;
        else if (content[endObjects] == ']') bracketDepth--;
        endObjects++;
    }

    std::string objectsText = content.substr(objectsPos, endObjects - objectsPos);
    size_t objStart = objectsText.find("{");

    while (objStart != std::string::npos) {
        size_t objEnd = objStart + 1;
        int braceDepth = 1;
        while (objEnd < objectsText.size() && braceDepth > 0) {
            if (objectsText[objEnd] == '{') braceDepth++;
            else if (objectsText[objEnd] == '}') braceDepth--;
            objEnd++;
        }

        std::string obj = objectsText.substr(objStart, objEnd - objStart);

        auto getFloat = [&](const std::string& key) -> float {
            size_t pos = obj.find("\"" + key + "\":");
            if (pos == std::string::npos) return 0.0f;
            size_t startValue = pos + key.length() + 3;
            size_t endValue = obj.find_first_of(",}", startValue);
            try { return std::stof(obj.substr(startValue, endValue - startValue)); }
            catch (...) { return 0.0f; }
            };

        rects.push_back({ getFloat("x"), getFloat("y"), getFloat("width"), getFloat("height") });

        if (names) {
            size_t valPos = obj.find("\"name\":\"");
            if (valPos != std::string::npos) {
                size_t valStart = valPos + 8;
                size_t valEnd = obj.find("\"", valStart);
                names->push_back(obj.substr(valStart, valEnd - valStart));
            }
            else {
                names->push_back("unknown");
            }
        }
        objStart = objectsText.find("{", objEnd);
    }
}