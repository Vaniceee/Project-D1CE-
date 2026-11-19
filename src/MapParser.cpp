#include "MapParser.h"

namespace {
    void ParseTilesetTilesData(const std::string& jsonBlock, const std::string& mapDir, TilesetInfo& ts) {
        size_t tilesArrayPos = jsonBlock.find("\"tiles\":[");
        if (tilesArrayPos == std::string::npos) return;

        size_t arrEnd = tilesArrayPos + 9;
        int bracketDepth = 1;
        while (arrEnd < jsonBlock.size() && bracketDepth > 0) {
            if (jsonBlock[arrEnd] == '[') bracketDepth++;
            else if (jsonBlock[arrEnd] == ']') bracketDepth--;
            arrEnd++;
        }
        std::string tilesArrayStr = jsonBlock.substr(tilesArrayPos + 9, arrEnd - (tilesArrayPos + 9) - 1);

        size_t objStart = tilesArrayStr.find('{');
        while (objStart != std::string::npos) {
            size_t objEnd = objStart + 1;
            int braceDepth = 1;
            while (objEnd < tilesArrayStr.size() && braceDepth > 0) {
                if (tilesArrayStr[objEnd] == '{') braceDepth++;
                else if (tilesArrayStr[objEnd] == '}') braceDepth--;
                objEnd++;
            }

            std::string tileEntry = tilesArrayStr.substr(objStart, objEnd - objStart);

            size_t idPos = tileEntry.find("\"id\":");
            if (idPos != std::string::npos) {
                size_t idStart = idPos + 5;
                size_t idEnd = tileEntry.find_first_of(",}", idStart);
                int localId = -1;
                try { localId = std::stoi(tileEntry.substr(idStart, idEnd - idStart)); }
                catch (...) { localId = -1; }

                if (localId >= 0) {
                    // 1. Індивідуальне зображення для Collection of Images
                    size_t imgPos = tileEntry.find("\"image\":\"");
                    if (imgPos != std::string::npos) {
                        size_t imgStart = imgPos + 9;
                        size_t imgEnd = tileEntry.find("\"", imgStart);
                        std::string rawImgPath = tileEntry.substr(imgStart, imgEnd - imgStart);

                        std::string key = ts.name + "_" + rawImgPath;
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
                    }

                    // 2. Вбудовані прямокутники колізій всередині тайла/об'єкта (якщо є)
                    size_t ogPos = tileEntry.find("\"objectgroup\":");
                    if (ogPos != std::string::npos) {
                        size_t ogObjsPos = tileEntry.find("\"objects\":[", ogPos);
                        if (ogObjsPos != std::string::npos) {
                            size_t ogEnd = ogObjsPos + 11;
                            int ogDepth = 1;
                            while (ogEnd < tileEntry.size() && ogDepth > 0) {
                                if (tileEntry[ogEnd] == '[') ogDepth++;
                                else if (tileEntry[ogEnd] == ']') ogDepth--;
                                ogEnd++;
                            }
                            std::string ogObjsStr = tileEntry.substr(ogObjsPos + 11, ogEnd - (ogObjsPos + 11) - 1);

                            size_t cStart = ogObjsStr.find('{');
                            while (cStart != std::string::npos) {
                                size_t cEnd = cStart + 1;
                                int cDepth = 1;
                                while (cEnd < ogObjsStr.size() && cDepth > 0) {
                                    if (ogObjsStr[cEnd] == '{') cDepth++;
                                    else if (ogObjsStr[cEnd] == '}') cDepth--;
                                    cEnd++;
                                }
                                std::string cObj = ogObjsStr.substr(cStart, cEnd - cStart);

                                auto getF = [&](const std::string& k) -> float {
                                    size_t p = cObj.find("\"" + k + "\":");
                                    if (p == std::string::npos) return 0.0f;
                                    size_t s = p + k.length() + 3;
                                    size_t e = cObj.find_first_of(",}", s);
                                    try { return std::stof(cObj.substr(s, e - s)); }
                                    catch (...) { return 0.0f; }
                                    };

                                float w = getF("width");
                                float h = getF("height");
                                if (w > 0.0f && h > 0.0f) {
                                    ts.tileCollisions[localId].push_back({ getF("x"), getF("y"), w, h });
                                }

                                cStart = ogObjsStr.find('{', cEnd);
                            }
                        }
                    }
                }
            }

            objStart = tilesArrayStr.find('{', objEnd);
        }
    }

    void AddTileCollisionsToWorld(
        uint32_t rawGid,
        float worldTopLeftX,
        float worldTopLeftY,
        float targetWidth,
        float targetHeight,
        MapData& outData)
    {
        constexpr uint32_t FLIP_H = 0x80000000;
        constexpr uint32_t FLIP_V = 0x40000000;
        constexpr uint32_t FLIP_D = 0x20000000;
        constexpr uint32_t ALL_FLAGS = FLIP_H | FLIP_V | FLIP_D;

        uint32_t cleanGid = rawGid & ~ALL_FLAGS;
        if (cleanGid == 0) return;

        const TilesetInfo* activeTs = nullptr;
        int localId = 0;
        for (int i = (int)outData.tilesets.size() - 1; i >= 0; --i) {
            if (cleanGid >= (uint32_t)outData.tilesets[i].firstgid) {
                activeTs = &outData.tilesets[i];
                localId = cleanGid - outData.tilesets[i].firstgid;
                break;
            }
        }
        if (!activeTs) return;

        auto colIt = activeTs->tileCollisions.find(localId);
        if (colIt == activeTs->tileCollisions.end()) return;

        float origW = (float)activeTs->tileWidth;
        float origH = (float)activeTs->tileHeight;
        auto texIt = activeTs->individualTextures.find(localId);
        if (texIt != activeTs->individualTextures.end() && texIt->second.id != 0) {
            origW = (float)texIt->second.width;
            origH = (float)texIt->second.height;
        }

        float scaleX = (origW > 0.0f) ? (targetWidth / origW) : 1.0f;
        float scaleY = (origH > 0.0f) ? (targetHeight / origH) : 1.0f;
        bool flipH = (rawGid & FLIP_H) != 0;
        bool flipV = (rawGid & FLIP_V) != 0;

        for (const Rectangle& localCol : colIt->second) {
            float cw = localCol.width * scaleX;
            float ch = localCol.height * scaleY;
            float cx = localCol.x * scaleX;
            float cy = localCol.y * scaleY;

            if (flipH) cx = targetWidth - (cx + cw);
            if (flipV) cy = targetHeight - (cy + ch);

            outData.autoCollisionRects.push_back({
                worldTopLeftX + cx,
                worldTopLeftY + cy,
                cw,
                ch
                });
        }
    }
}

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

    GAME_INFO("MAP_PARSER", "Map parsed: %s (Manual cols: %zu, Auto cols: %zu, Visuals: %zu)",
        mapPath.c_str(), outData.collisionRects.size(), outData.autoCollisionRects.size(), outData.visualObjects.size());

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
        try { ts.firstgid = std::stoi(content.substr(startVal, endVal - startVal)); }
        catch (...) { ts.firstgid = 1; }

        size_t nextTsPos = content.find("\"firstgid\":", tsPos + 11);
        std::string tsBlock = (nextTsPos != std::string::npos)
            ? content.substr(tsPos, nextTsPos - tsPos)
            : content.substr(tsPos);

        std::string effectiveBlock = tsBlock;
        std::string effectiveDir = mapDir;
        size_t srcPos = tsBlock.find("\"source\":\"");
        if (srcPos != std::string::npos) {
            size_t sStart = srcPos + 10;
            size_t sEnd = tsBlock.find("\"", sStart);
            std::string relSource = tsBlock.substr(sStart, sEnd - sStart);
            std::replace(relSource.begin(), relSource.end(), '\\', '/');
            std::string fullTsPath = mapDir + relSource;

            std::ifstream tsFile(fullTsPath);
            if (tsFile.is_open()) {
                std::stringstream ss;
                ss << tsFile.rdbuf();
                effectiveBlock = ss.str();
                size_t lastSlash = fullTsPath.find_last_of("/\\");
                effectiveDir = (lastSlash != std::string::npos) ? fullTsPath.substr(0, lastSlash + 1) : mapDir;
            }
            else {
                GAME_WARN("MAP_PARSER", "External tileset not found (if it's .tsx, export as .tsj or embed it): %s", fullTsPath.c_str());
            }
        }

        size_t namePos = effectiveBlock.find("\"name\":\"");
        if (namePos != std::string::npos) {
            size_t nStart = namePos + 8;
            size_t nEnd = effectiveBlock.find("\"", nStart);
            ts.name = effectiveBlock.substr(nStart, nEnd - nStart);
        }
        else {
            ts.name = "tileset_" + std::to_string(ts.firstgid);
        }

        auto getField = [&](const std::string& key) -> int {
            size_t pos = effectiveBlock.find("\"" + key + "\":");
            if (pos == std::string::npos) return 0;
            size_t sv = pos + key.length() + 3;
            size_t ev = effectiveBlock.find_first_of(",}", sv);
            try { return std::stoi(effectiveBlock.substr(sv, ev - sv)); }
            catch (...) { return 0; }
            };

        int parsedTsW = getField("tilewidth");
        int parsedTsH = getField("tileheight");
        ts.tileWidth = (parsedTsW > 0) ? parsedTsW : outData.tileWidth;
        ts.tileHeight = (parsedTsH > 0) ? parsedTsH : outData.tileHeight;

        ParseTilesetTilesData(effectiveBlock, effectiveDir, ts);

        if (ts.individualTextures.empty()) {
            std::string assetKey = ts.name;
            ts.texture = AssetManager::Instance().GetTexture(assetKey);
            if (ts.texture.id == 0 && !defaultTilesetKey.empty()) {
                assetKey = defaultTilesetKey;
                ts.texture = AssetManager::Instance().GetTexture(assetKey);
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

        for (int y = 0; y < outData.mapHeight; ++y) {
            for (int x = 0; x < outData.mapWidth; ++x) {
                int idx = x + y * outData.mapWidth;
                if (idx >= 0 && idx < (int)layerData.size() && layerData[idx] != 0) {
                    AddTileCollisionsToWorld(
                        static_cast<uint32_t>(layerData[idx]),
                        (float)(x * outData.tileWidth),
                        (float)(y * outData.tileHeight),
                        (float)outData.tileWidth,
                        (float)outData.tileHeight,
                        outData
                    );
                }
            }
        }

        std::string lowerName = ToLower(layerName);
        if (lowerName.find("roof") != std::string::npos ||
            lowerName.find("above") != std::string::npos ||
            lowerName.find("top") != std::string::npos)
        {
            outData.abovePlayerLayers.push_back(layerData);
        }
        else {
            outData.belowPlayerLayers.push_back(layerData);
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
        }
        else if (lowerLayerName.find("trig") != std::string::npos ||
            lowerLayerName.find("door") != std::string::npos ||
            lowerLayerName.find("interact") != std::string::npos ||
            lowerLayerName.find("двер") != std::string::npos)
        {
            ParseRectLayer(content, layerName, outData.triggers, &outData.triggerNames, &outData.teleportTargets, &outData.interactables);
        }
        else if (lowerLayerName.find("spawn") != std::string::npos ||
            lowerLayerName.find("entit") != std::string::npos)
        {
            ParseSpawnLayer(content, layerName, outData);
        }
        else {
            int order = (lowerLayerName.find("floor") != std::string::npos ||
                lowerLayerName.find("carpet") != std::string::npos ||
                lowerLayerName.find("rug") != std::string::npos)
                ? -1 : visualLayerIndex++;

            ParseVisualObjectLayer(content, layerName, order, outData);
        }

        searchPos = objArrayPos + 10;
    }

    for (auto& topObj : outData.visualObjects) {
        if (topObj.layerOrder <= 0) continue;

        float footX = topObj.x + topObj.width * 0.5f;
        float footY = topObj.y;

        const MapVisualObject* bestSupport = nullptr;
        for (const auto& baseObj : outData.visualObjects) {
            if (baseObj.layerOrder < 0 || baseObj.layerOrder >= topObj.layerOrder) continue;

            float baseLeft = baseObj.x;
            float baseRight = baseObj.x + baseObj.width;
            float baseTop = baseObj.y - baseObj.height;
            float baseBottom = baseObj.y;

            if (footX >= baseLeft && footX <= baseRight &&
                footY >= baseTop && footY <= baseBottom) {
                if (!bestSupport || baseObj.sortY > bestSupport->sortY) {
                    bestSupport = &baseObj;
                }
            }
        }

        if (bestSupport) {
            topObj.sortY = bestSupport->sortY;
        }
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
            }
            else if (spawnName == "guard") {
                outData.guardSpawns.push_back({ x, y });
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
                vObj.sortY = vObj.y;
                outData.visualObjects.push_back(vObj);

                AddTileCollisionsToWorld(
                    vObj.rawGid,
                    vObj.x,
                    vObj.y - vObj.height,
                    vObj.width,
                    vObj.height,
                    outData
                );
            }
        }

        objStart = objectsText.find("{", objEnd);
    }
}

void MapParser::ParseRectLayer(
    const std::string& content,
    const std::string& layerName,
    std::vector<Rectangle>& rects,
    std::vector<std::string>* names,
    std::unordered_map<std::string, Rectangle>* targets,
    std::vector<InteractableObject>* interactables)
{
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

        Rectangle rect = { getFloat("x"), getFloat("y"), getFloat("width"), getFloat("height") };

        std::string objName = "unknown";
        size_t valPos = obj.find("\"name\"");
        if (valPos != std::string::npos) {
            size_t colonPos = obj.find(":", valPos);
            if (colonPos != std::string::npos) {
                size_t quoteStart = obj.find("\"", colonPos + 1);
                if (quoteStart != std::string::npos) {
                    size_t quoteEnd = obj.find("\"", quoteStart + 1);
                    if (quoteEnd != std::string::npos) {
                        objName = obj.substr(quoteStart + 1, quoteEnd - (quoteStart + 1));
                    }
                }
            }
        }

        if (targets && objName.size() > 2 && objName.substr(objName.size() - 2) == "_t") {
            (*targets)[objName] = rect;
        }
        else if (interactables && objName.size() > 2 && objName.substr(objName.size() - 2) == "_i") {
            InteractableObject item;
            item.fullId = objName.substr(0, objName.size() - 2);
            size_t underscorePos = item.fullId.find('_');
            item.type = (underscorePos != std::string::npos) ? item.fullId.substr(0, underscorePos) : item.fullId;
            item.bounds = rect;
            item.isUsed = false;

            interactables->push_back(item);
        }
        else {
            rects.push_back(rect);
            if (names) {
                names->push_back(objName);
            }
        }

        objStart = objectsText.find("{", objEnd);
    }
}