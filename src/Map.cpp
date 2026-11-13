#include "Map.h"
#include "AssetManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Допоміжна функція для переведення рядка в нижній регістр
static std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}

// Витягує ім'я файлу без шляху та розширення: "png/objects/Tables/table_1.png" -> "table_1"
static std::string ExtractAssetKey(const std::string& rawPath) {
    size_t lastSlash = rawPath.find_last_of("/\\");
    std::string filename = (lastSlash != std::string::npos) ? rawPath.substr(lastSlash + 1) : rawPath;
    size_t lastDot = filename.find_last_of('.');
    return (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
}

// ФУНКЦІЯ ПАРСИНГУ CSV: Зчитує масив цифр (ID плиток) з JSON/TMJ файлу для конкретного шару
std::vector<int> GameMap::ParseCSV(const std::string& content, const std::string& layerName) {
    std::vector<int> result;
    size_t namePos = content.find("\"name\":\"" + layerName + "\"");

    if (namePos == std::string::npos) {
        GAME_WARN("MAP", "Layer not found by name: '%s'", layerName.c_str());
        return result;
    }

    size_t startLayer = content.rfind("{", namePos);
    size_t endLayer = content.find("}", namePos);

    if (startLayer == std::string::npos || endLayer == std::string::npos) return result;

    std::string layerChunks = content.substr(startLayer, endLayer - startLayer);
    size_t dataPos = layerChunks.find("\"data\":[");

    if (dataPos == std::string::npos) {
        GAME_ERROR("MAP", "Array 'data' not found in layer: '%s'", layerName.c_str());
        return result;
    }
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
                // stoul захищає від переповнення при встановлених старших бітах фліпу
                result.push_back(static_cast<int>(std::stoul(token)));
            }
            catch (...) {
                result.push_back(0);
            }
        }
    }

    GAME_DEBUG("MAP", "Layer '%s' parsed. Tiles: %zu", layerName.c_str(), result.size());
    return result;
}


// УНІВЕРСАЛЬНА ФУНКЦІЯ ЗАВАНТАЖЕННЯ МАТРИЦЬ, ОБ'ЄКТІВ ТА БАГАТЬОХ ТАЙЛСЕТІВ
bool GameMap::Load(const std::string& mapPath, const std::string& defaultTilesetKey) {
    std::ifstream file(mapPath);
    if (!file.is_open()) {
        GAME_ERROR("MAP", "Could not open map file: %s", mapPath.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Визначаємо папку, в якій лежить файл мапи (наприклад, "assets/map/")
    size_t lastMapSlash = mapPath.find_last_of("/\\");
    std::string mapDir = (lastMapSlash != std::string::npos) ? mapPath.substr(0, lastMapSlash + 1) : "";

    // 1. Автоматичне зчитування розмірів карти і базових тайлів
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

    mapWidth = getIntField("width");
    mapHeight = getIntField("height");
    tileWidth = getIntField("tilewidth");
    tileHeight = getIntField("tileheight");

    // Очищаємо попередні дані в пам'яті
    Unload();

    // 2. ПАРСИНГ ТАЙЛСЕТІВ (Звичайні спрайтшити з tiles/ та Collection of Images з objects/)
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

        // Шукаємо ім'я тайлсету всередині його блоку
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

        ts.tileWidth = (parsedTsW > 0) ? parsedTsW : tileWidth;
        ts.tileHeight = (parsedTsH > 0) ? parsedTsH : tileHeight;

        // Перевіряємо, чи це Collection of Images (чи є масив "tiles":[)
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

                // Формуємо унікальний ключ для AssetManager: "НазваКолекції_Ім'яФайлу"
                std::string key = ts.name + "_" + ExtractAssetKey(rawImgPath);
                Texture2D tex = AssetManager::Instance().GetTexture(key);

                if (tex.id == 0) {
                    std::string cleanPath = rawImgPath;
                    // Нормалізуємо слеші на випадок Windows-шляхів
                    std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

                    // Tiled зберігає шлях відносно папки з мапою ("png/objects/...")
                    // Додаємо шлях до папки мапи ("assets/map/"), якщо його ще немає
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
                    GAME_ERROR("MAP", "Failed to load collection image: '%s' (resolved path: '%s')",
                        rawImgPath.c_str(), (mapDir + rawImgPath).c_str());
                }

                tSearch = imgEnd;
            }
            GAME_INFO("MAP", "Loaded Collection of Images tileset: '%s' (firstgid: %d, items: %zu)",
                ts.name.c_str(), ts.firstgid, ts.individualTextures.size());
        }
        else {
            // Звичайний спрайтшит із папки tiles (для підлоги, стін, килимів)
            std::string assetKey = ts.name;
            ts.texture = AssetManager::Instance().GetTexture(assetKey);

            if (ts.texture.id == 0 && !defaultTilesetKey.empty()) {
                assetKey = defaultTilesetKey;
                ts.texture = AssetManager::Instance().GetTexture(assetKey);
            }

            if (ts.texture.id == 0) {
                GAME_ERROR("MAP", "Failed to load texture for tileset '%s' (firstgid: %d)", ts.name.c_str(), ts.firstgid);
            }
            else {
                GAME_INFO("MAP", "Loaded tileset: '%s' (firstgid: %d, size: %dx%d) using asset key: '%s'",
                    ts.name.c_str(), ts.firstgid, ts.tileWidth, ts.tileHeight, assetKey.c_str());
            }
        }

        tilesets.push_back(ts);
        tsSearchPos = endVal;
    }

    // Fallback, якщо секція тайлсетів у JSON відсутня
    if (tilesets.empty() && !defaultTilesetKey.empty()) {
        TilesetInfo fallbackTs;
        fallbackTs.firstgid = 1;
        fallbackTs.name = defaultTilesetKey;
        fallbackTs.texture = AssetManager::Instance().GetTexture(defaultTilesetKey);
        fallbackTs.tileWidth = tileWidth;
        fallbackTs.tileHeight = tileHeight;
        tilesets.push_back(fallbackTs);
        GAME_WARN("MAP", "No tilesets found in JSON. Using fallback tileset key: '%s'", defaultTilesetKey.c_str());
    }

    // 3. Автоматичний сканер ВСІХ тайлових шарів та їх сортування (підлога / дах)
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
            abovePlayerLayers.push_back(layerData);
            GAME_DEBUG("MAP", "Layer '%s' auto-sorted as ABOVE player", layerName.c_str());
        }
        else {
            belowPlayerLayers.push_back(layerData);
            GAME_DEBUG("MAP", "Layer '%s' auto-sorted as BELOW player", layerName.c_str());
        }

        searchPos = typePos + 18;
    }

    // 4. Сканер шарів об'єктів (колізії, тригери та візуальні об'єкти для Y-сортування)
    searchPos = 0;
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
            ParseObjectLayer(content, layerName, collisionRects, nullptr);
            GAME_DEBUG("MAP", "Auto-detected COLLISION object layer: '%s'", layerName.c_str());
        }
        else if (lowerLayerName.find("trig") != std::string::npos ||
            lowerLayerName.find("door") != std::string::npos ||
            lowerLayerName.find("interact") != std::string::npos ||
            lowerLayerName.find("двер") != std::string::npos)
        {
            ParseObjectLayer(content, layerName, doorTriggers, &doorNumbers);
            GAME_DEBUG("MAP", "Auto-detected TRIGGER/DOOR object layer: '%s'", layerName.c_str());
        }
        else {
            ParseVisualObjectLayer(content, layerName, visualLayerIndex++);
            GAME_DEBUG("MAP", "Auto-detected VISUAL object layer: '%s'", layerName.c_str());
        }

        searchPos = objArrayPos + 10;
    }

    GAME_INFO("MAP", "Map loaded successfully: %s (Size: %dx%d, Tilesets: %zu, Visual Objects: %zu)",
        mapPath.c_str(), mapWidth, mapHeight, tilesets.size(), visualObjects.size());
    return true;
}


// ФУНКЦІЯ ОЧИЩЕННЯ
void GameMap::Unload() {
    tilesets.clear();
    belowPlayerLayers.clear();
    abovePlayerLayers.clear();
    collisionRects.clear();
    doorTriggers.clear();
    doorNumbers.clear();
    visualObjects.clear();
    currentDoorMessage = "";
}


// ФУНКЦІЯ МАЛЮВАННЯ ТАЙЛІВ ПІД ГРАВЦЕМ
void GameMap::DrawBelowPlayer() {
    for (const auto& layerData : belowPlayerLayers) {
        DrawSingleLayer(layerData);
    }

    /* for (const auto& rect : collisionRects) {
        DrawRectangleRec(rect, Fade(RED, 0.4f));
    }

    for (const auto& trigger : doorTriggers) {
        DrawRectangleLinesEx(trigger, 2, GREEN);
    }*/
}


// ФУНКЦІЯ МАЛЮВАННЯ ТАЙЛІВ НАД ГРАВЦЕМ
void GameMap::DrawAbovePlayer() {
    for (const auto& layerData : abovePlayerLayers) {
        DrawSingleLayer(layerData);
    }
}


// ВНУТРІШНЯ ФУНКЦІЯ МАЛЮВАННЯ ОДНОГО ТАЙЛОВОГО ШАРУ (ПО СІТЦІ)
void GameMap::DrawSingleLayer(const std::vector<int>& layerData) {
    if (layerData.empty() || tilesets.empty()) return;

    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
    constexpr uint32_t ALL_FLIP_FLAGS = (FLIPPED_HORIZONTALLY_FLAG |
        FLIPPED_VERTICALLY_FLAG |
        FLIPPED_DIAGONALLY_FLAG);

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int idx = x + y * mapWidth;
            if (idx < 0 || idx >= (int)layerData.size()) continue;

            uint32_t rawGlobalTileId = static_cast<uint32_t>(layerData[idx]);
            if (rawGlobalTileId == 0) continue;

            uint32_t globalTileId = rawGlobalTileId & ~ALL_FLIP_FLAGS;
            if (globalTileId == 0) continue;

            const TilesetInfo* activeTileset = nullptr;
            int localTileId = 0;

            for (int i = (int)tilesets.size() - 1; i >= 0; --i) {
                if (globalTileId >= (uint32_t)tilesets[i].firstgid) {
                    activeTileset = &tilesets[i];
                    localTileId = globalTileId - tilesets[i].firstgid;
                    break;
                }
            }

            if (!activeTileset || activeTileset->texture.id == 0) continue;

            int tsCols = activeTileset->texture.width / activeTileset->tileWidth;
            if (tsCols <= 0) continue;

            int tsX = (localTileId % tsCols) * activeTileset->tileWidth;
            int tsY = (localTileId / tsCols) * activeTileset->tileHeight;

            Rectangle srcRec = { (float)tsX, (float)tsY, (float)activeTileset->tileWidth, (float)activeTileset->tileHeight };

            float posX = (float)(x * tileWidth);
            float posY = (float)(y * tileHeight);

            bool flipH = (rawGlobalTileId & FLIPPED_HORIZONTALLY_FLAG) != 0;
            bool flipV = (rawGlobalTileId & FLIPPED_VERTICALLY_FLAG) != 0;
            bool flipD = (rawGlobalTileId & FLIPPED_DIAGONALLY_FLAG) != 0;

            if (!flipH && !flipV && !flipD) {
                Vector2 pos = { posX, posY };
                DrawTextureRec(activeTileset->texture, srcRec, pos, WHITE);
            }
            else {
                Rectangle destRec = {
                    posX + (float)tileWidth / 2.0f,
                    posY + (float)tileHeight / 2.0f,
                    (float)activeTileset->tileWidth,
                    (float)activeTileset->tileHeight
                };
                Vector2 origin = { (float)activeTileset->tileWidth / 2.0f, (float)activeTileset->tileHeight / 2.0f };

                float rotation = 0.0f;

                if (flipD) {
                    if (flipH && !flipV) { rotation = 90.0f; flipH = false; flipV = false; }
                    else if (!flipH && flipV) { rotation = 270.0f; flipH = false; flipV = false; }
                    else if (flipH && flipV) { rotation = 90.0f; flipH = false; flipV = true; }
                    else { rotation = 270.0f; flipH = true; flipV = false; }
                }

                if (flipH) srcRec.width = -srcRec.width;
                if (flipV) srcRec.height = -srcRec.height;

                DrawTexturePro(activeTileset->texture, srcRec, destRec, origin, rotation, WHITE);
            }
        }
    }
}


// МАЛЮВАННЯ ОДНОГО ВІЗУАЛЬНОГО ОБ'ЄКТА (ДЛЯ Y-СОРТУВАННЯ)
void GameMap::DrawVisualObject(const MapVisualObject& obj) const {
    constexpr uint32_t FLIP_H = 0x80000000;
    constexpr uint32_t FLIP_V = 0x40000000;
    constexpr uint32_t FLIP_D = 0x20000000;
    constexpr uint32_t ALL_FLAGS = FLIP_H | FLIP_V | FLIP_D;

    uint32_t gid = obj.rawGid & ~ALL_FLAGS;
    if (gid == 0) return;

    const TilesetInfo* activeTs = nullptr;
    int localId = 0;
    for (int i = (int)tilesets.size() - 1; i >= 0; --i) {
        if (gid >= (uint32_t)tilesets[i].firstgid) {
            activeTs = &tilesets[i];
            localId = gid - tilesets[i].firstgid;
            break;
        }
    }
    if (!activeTs) return;

    Texture2D tex = { 0 };
    Rectangle srcRec = { 0, 0, 0, 0 };

    // 1. Якщо це Collection of Images — беремо індивідуальну текстуру за localId
    if (!activeTs->individualTextures.empty()) {
        auto it = activeTs->individualTextures.find(localId);
        if (it == activeTs->individualTextures.end() || it->second.id == 0) return;
        tex = it->second;
        srcRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    }
    // 2. Якщо це звичайний спрайтшит — вирізаємо кадр по сітці
    else if (activeTs->texture.id != 0) {
        tex = activeTs->texture;
        int tsCols = tex.width / activeTs->tileWidth;
        if (tsCols <= 0) return;
        srcRec = {
            (float)((localId % tsCols) * activeTs->tileWidth),
            (float)((localId / tsCols) * activeTs->tileHeight),
            (float)activeTs->tileWidth,
            (float)activeTs->tileHeight
        };
    }
    else {
        return;
    }

    bool flipH = (obj.rawGid & FLIP_H) != 0;
    bool flipV = (obj.rawGid & FLIP_V) != 0;
    bool flipD = (obj.rawGid & FLIP_D) != 0;

    float rotation = 0.0f;
    if (flipD) {
        if (flipH && !flipV) { rotation = 90.0f; flipH = false; flipV = false; }
        else if (!flipH && flipV) { rotation = 270.0f; flipH = false; flipV = false; }
        else if (flipH && flipV) { rotation = 90.0f; flipH = false; flipV = true; }
        else { rotation = 270.0f; flipH = true; flipV = false; }
    }

    if (flipH) srcRec.width = -srcRec.width;
    if (flipV) srcRec.height = -srcRec.height;

    // У Tiled (x, y) для тайл-об'єктів — це НИЖНІЙ лівий кут!
    // Зміщуємо точку рендеру в центр об'єкта для DrawTexturePro
    Rectangle destRec = {
        obj.x + obj.width * 0.5f,
        obj.y - obj.height * 0.5f,
        obj.width,
        obj.height
    };
    Vector2 origin = { obj.width * 0.5f, obj.height * 0.5f };

    DrawTexturePro(tex, srcRec, destRec, origin, rotation, WHITE);
}


// ПАРСИНГ ШАРУ ВІЗУАЛЬНИХ ОБ'ЄКТІВ (З GID)
void GameMap::ParseVisualObjectLayer(const std::string& content, const std::string& layerName, int layerOrder) {
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
                try {
                    return std::stof(obj.substr(startValue, endValue - startValue));
                }
                catch (...) {
                    return 0.0f;
                }
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
                visualObjects.push_back(vObj);
            }
        }

        objStart = objectsText.find("{", objEnd);
    }

    GAME_DEBUG("MAP", "Visual object layer '%s' loaded. Total visual objects: %zu", layerName.c_str(), visualObjects.size());
}


// ФУНКЦІЯ ПАРСИНГУ ОБ'ЄКТІВ КОЛІЗІЙ ТА ТРИГЕРІВ
void GameMap::ParseObjectLayer(const std::string& content, const std::string& layerName, std::vector<Rectangle>& rects, std::vector<std::string>* names) {
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
            try {
                return std::stof(obj.substr(startValue, endValue - startValue));
            }
            catch (...) {
                return 0.0f;
            }
            };

        float x = getFloat("x");
        float y = getFloat("y");
        float w = getFloat("width");
        float h = getFloat("height");

        rects.push_back({ x, y, w, h });

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

    GAME_DEBUG("MAP", "Object layer '%s' loaded: %zu rectangles", layerName.c_str(), rects.size());
}


// ПЕРЕВІРКА ЗІТКНЕНЬ ЗІ СТІНАМИ
bool GameMap::CheckWallCollision(Rectangle playerHitbox) const {
    for (const auto& rect : collisionRects) {
        if (CheckCollisionRecs(playerHitbox, rect)) {
            return true;
        }
    }
    return false;
}


// ОНОВЛЕННЯ ТРИГЕРІВ ДВЕРЕЙ
void GameMap::UpdateDoorTriggers(Rectangle playerHitbox) {
    currentDoorMessage = "";
    for (size_t i = 0; i < doorTriggers.size(); ++i) {
        if (CheckCollisionRecs(playerHitbox, doorTriggers[i])) {
            std::string triggerName = "unknown";
            if (i < doorNumbers.size()) {
                triggerName = doorNumbers[i];
            }
            currentDoorMessage = triggerName;
            break;
        }
    }
}