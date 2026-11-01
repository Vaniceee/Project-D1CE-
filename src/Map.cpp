#include "Map.h"
#include "AssetManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Допоміжна функція для переведення рядка в нижній регістр
std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
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
        // Захист від зайвих пробілів та переносів рядків всередині токена
        token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) {
            return std::isspace(c);
            }), token.end());

        if (!token.empty()) {
            try {
                // Безпечне перетворення із захистом від падінь на сміттєвих токенах
                result.push_back(std::stoi(token));
            }
            catch (...) {
                result.push_back(0); // Якщо токен пошкоджений — ставимо пустий тайл замість крашу
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
    tilesets.clear();
    belowPlayerLayers.clear();
    abovePlayerLayers.clear();
    collisionRects.clear();
    doorTriggers.clear();
    doorNumbers.clear();

    // 2. ПАРСИНГ ТАЙЛСЕТІВ (З динамічним зчитуванням їхніх власних розмірів з JSON)
    size_t tsSearchPos = 0;
    while (true) {
        size_t tsPos = content.find("\"firstgid\":", tsSearchPos);
        if (tsPos == std::string::npos) break;

        TilesetInfo ts;
        // Витягуємо firstgid
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

        // Лямбда для точного пошуку числових полів у межах поточного блоку тайлсету
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

        // Завантажуємо текстуру через AssetManager
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

    // 3. Автоматичний сканер ВСІХ тайлових шарів та їх сортування
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

    // 4. Сканер шарів об'єктів (колізії та тригери)
    searchPos = 0;
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

        searchPos = objArrayPos + 10;
    }

    GAME_INFO("MAP", "Map loaded successfully: %s (Size: %dx%d, Tilesets count: %zu)",
        mapPath.c_str(), mapWidth, mapHeight, tilesets.size());
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
}


// ФУНКЦІЯ МАЛЮВАННЯ ПІД ГРАВЦЕМ
void GameMap::DrawBelowPlayer() {
    for (const auto& layerData : belowPlayerLayers) {
        DrawSingleLayer(layerData);
    }

    for (const auto& rect : collisionRects) {
        DrawRectangleRec(rect, Fade(RED, 0.4f));
    }

    for (const auto& trigger : doorTriggers) {
        DrawRectangleLinesEx(trigger, 2, GREEN);
    }
}


// ФУНКЦІЯ МАЛЮВАННЯ НАД ГРАВЦЕМ
void GameMap::DrawAbovePlayer() {
    for (const auto& layerData : abovePlayerLayers) {
        DrawSingleLayer(layerData);
    }
}


// ВНУТРІШНЯ ФУНКЦІЯ МАЛЮВАННЯ ОДНОГО ШАРУ (З ПІДТРИМКОЮ РІЗНИХ РОЗМІРІВ ТАЙЛІВ ТА ЗМІЩЕНЬ)
void GameMap::DrawSingleLayer(const std::vector<int>& layerData) {
    if (layerData.empty() || tilesets.empty()) return;

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            int idx = x + y * mapWidth;
            if (idx < 0 || idx >= (int)layerData.size()) continue; // Захист від виходу за межі пам'яті

            int globalTileId = layerData[idx];
            if (globalTileId == 0) continue; // 0 — пуста клітинка

            // Шукаємо, якому саме тайлсету належить globalTileId
            const TilesetInfo* activeTileset = nullptr;
            int localTileId = 0;

            for (int i = (int)tilesets.size() - 1; i >= 0; --i) {
                if (globalTileId >= tilesets[i].firstgid) {
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

            // Базова позиція плитки на сітці карти
            float posX = (float)(x * tileWidth);
            float posY = (float)(y * tileHeight);

            // Якщо тайл більший за стандартну сітку карти (наприклад, 96x96 проти 32x32),
            // зміщуємо його вгору відносно нижнього лівого кута клітинки
            if (activeTileset->tileHeight > tileHeight) {
                posY -= (activeTileset->tileHeight - tileHeight);
            }

            Vector2 pos = { posX, posY };
            DrawTextureRec(activeTileset->texture, srcRec, pos, WHITE);
        }
    }
}


// ФУНКЦІЯ ПАРСИНГУ ОБ'ЄКТІВ
void GameMap::ParseObjectLayer(const std::string& content, const std::string& layerName, std::vector<Rectangle>& rects, std::vector<std::string>* names) {
    size_t layerPos = content.find("\"name\":\"" + layerName + "\"");
    if (layerPos == std::string::npos) return;

    size_t objectsPos = content.find("\"objects\":[", layerPos);
    if (objectsPos == std::string::npos) return;

    size_t endObjects = content.find("]", objectsPos);
    if (endObjects == std::string::npos || endObjects <= objectsPos) return;

    std::string objectsText = content.substr(objectsPos, endObjects - objectsPos);

    size_t objStart = objectsText.find("{");

    while (objStart != std::string::npos) {
        size_t objEnd = objectsText.find("}", objStart);
        if (objEnd == std::string::npos) break;
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
bool GameMap::CheckWallCollision(Rectangle playerHitbox) {
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