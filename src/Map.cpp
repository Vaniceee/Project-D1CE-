#include "Map.h"
#include "MapParser.h"

bool GameMap::Load(const std::string& mapPath, const std::string& defaultTilesetKey) {
    Unload();
    return MapParser::Parse(mapPath, defaultTilesetKey, data);
}

void GameMap::Unload() {
    data = MapData{}; // Скидає всі вектори та параметри до дефолтних значень
    currentDoorMessage = "";
}

void GameMap::DrawBelowPlayer() const {
    for (const auto& layerData : data.belowPlayerLayers) {
        DrawSingleLayer(layerData);
    }
}

void GameMap::DrawAbovePlayer() const {
    for (const auto& layerData : data.abovePlayerLayers) {
        DrawSingleLayer(layerData);
    }
}

void GameMap::DrawSingleLayer(const std::vector<int>& layerData) const {
    if (layerData.empty() || data.tilesets.empty()) return;

    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
    constexpr uint32_t ALL_FLIP_FLAGS = (FLIPPED_HORIZONTALLY_FLAG |
        FLIPPED_VERTICALLY_FLAG |
        FLIPPED_DIAGONALLY_FLAG);

    for (int y = 0; y < data.mapHeight; ++y) {
        for (int x = 0; x < data.mapWidth; ++x) {
            int idx = x + y * data.mapWidth;
            if (idx < 0 || idx >= (int)layerData.size()) continue;

            uint32_t rawGlobalTileId = static_cast<uint32_t>(layerData[idx]);
            if (rawGlobalTileId == 0) continue;

            uint32_t globalTileId = rawGlobalTileId & ~ALL_FLIP_FLAGS;
            if (globalTileId == 0) continue;

            const TilesetInfo* activeTileset = nullptr;
            int localTileId = 0;

            for (int i = (int)data.tilesets.size() - 1; i >= 0; --i) {
                if (globalTileId >= (uint32_t)data.tilesets[i].firstgid) {
                    activeTileset = &data.tilesets[i];
                    localTileId = globalTileId - data.tilesets[i].firstgid;
                    break;
                }
            }

            if (!activeTileset || activeTileset->texture.id == 0) continue;

            int tsCols = activeTileset->texture.width / activeTileset->tileWidth;
            if (tsCols <= 0) continue;

            int tsX = (localTileId % tsCols) * activeTileset->tileWidth;
            int tsY = (localTileId / tsCols) * activeTileset->tileHeight;

            Rectangle srcRec = { (float)tsX, (float)tsY, (float)activeTileset->tileWidth, (float)activeTileset->tileHeight };

            float posX = (float)(x * data.tileWidth);
            float posY = (float)(y * data.tileHeight);

            bool flipH = (rawGlobalTileId & FLIPPED_HORIZONTALLY_FLAG) != 0;
            bool flipV = (rawGlobalTileId & FLIPPED_VERTICALLY_FLAG) != 0;
            bool flipD = (rawGlobalTileId & FLIPPED_DIAGONALLY_FLAG) != 0;

            if (!flipH && !flipV && !flipD) {
                DrawTextureRec(activeTileset->texture, srcRec, { posX, posY }, WHITE);
            }
            else {
                Rectangle destRec = {
                    posX + (float)data.tileWidth / 2.0f,
                    posY + (float)data.tileHeight / 2.0f,
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

void GameMap::DrawVisualObject(const MapVisualObject& obj) const {
    constexpr uint32_t FLIP_H = 0x80000000;
    constexpr uint32_t FLIP_V = 0x40000000;
    constexpr uint32_t FLIP_D = 0x20000000;
    constexpr uint32_t ALL_FLAGS = FLIP_H | FLIP_V | FLIP_D;

    uint32_t gid = obj.rawGid & ~ALL_FLAGS;
    if (gid == 0) return;

    const TilesetInfo* activeTs = nullptr;
    int localId = 0;
    for (int i = (int)data.tilesets.size() - 1; i >= 0; --i) {
        if (gid >= (uint32_t)data.tilesets[i].firstgid) {
            activeTs = &data.tilesets[i];
            localId = gid - data.tilesets[i].firstgid;
            break;
        }
    }
    if (!activeTs) return;

    Texture2D tex = { 0 };
    Rectangle srcRec = { 0, 0, 0, 0 };

    if (!activeTs->individualTextures.empty()) {
        auto it = activeTs->individualTextures.find(localId);
        if (it == activeTs->individualTextures.end() || it->second.id == 0) return;
        tex = it->second;
        srcRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    }
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

    Rectangle destRec = {
        obj.x + obj.width * 0.5f,
        obj.y - obj.height * 0.5f,
        obj.width,
        obj.height
    };
    Vector2 origin = { obj.width * 0.5f, obj.height * 0.5f };

    DrawTexturePro(tex, srcRec, destRec, origin, rotation, WHITE);
}

bool GameMap::CheckWallCollision(Rectangle playerHitbox) const {
    for (const auto& rect : data.collisionRects) {
        if (CheckCollisionRecs(playerHitbox, rect)) {
            return true;
        }
    }
    return false;
}

void GameMap::UpdateDoorTriggers(Rectangle playerHitbox) {
    currentDoorMessage = "";
    for (size_t i = 0; i < data.doorTriggers.size(); ++i) {
        if (CheckCollisionRecs(playerHitbox, data.doorTriggers[i])) {
            currentDoorMessage = (i < data.doorNumbers.size()) ? data.doorNumbers[i] : "unknown";
            break;
        }
    }
}