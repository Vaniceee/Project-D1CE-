#include "Map.h"
#include "MapParser.h"
#include "Logger.h"

bool GameMap::Load(const std::string& mapPath, const std::string& defaultTilesetKey) {
    Unload();
    return MapParser::Parse(mapPath, defaultTilesetKey, data);
}

void GameMap::Unload() {
    data = MapData{};
    currentTriggerMessage = "";
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

void GameMap::DrawDebug() const {
    // 1. Колізії — напівпрозорий червоний
    for (const auto& rect : data.collisionRects) {
        DrawRectangleRec(rect, ColorAlpha(RED, 0.35f));
        DrawRectangleLinesEx(rect, 1.0f, RED);
    }

    // 2. Тригери взаємодій — напівпрозорий зелений
    for (size_t i = 0; i < data.triggers.size(); ++i) {
        DrawRectangleRec(data.triggers[i], ColorAlpha(GREEN, 0.35f));
        DrawRectangleLinesEx(data.triggers[i], 1.0f, GREEN);
        if (i < data.triggerNames.size()) {
            DrawText(data.triggerNames[i].c_str(), (int)data.triggers[i].x, (int)data.triggers[i].y - 12, 10, GREEN);
        }
    }

    // 3. Таргети телепортів (_t) — напівпрозорий блакитний
    for (const auto& [name, rect] : data.teleportTargets) {
        DrawRectangleRec(rect, ColorAlpha(SKYBLUE, 0.45f));
        DrawRectangleLinesEx(rect, 1.0f, SKYBLUE);
        DrawText(name.c_str(), (int)rect.x, (int)rect.y - 12, 10, SKYBLUE);
    }

    // 4. Спавни об'єктів (точки) — напівпрозорий фіолетовий
    DrawCircleV(data.playerSpawn, 10.0f, ColorAlpha(PURPLE, 0.6f));
    DrawCircleLinesV(data.playerSpawn, 10.0f, PURPLE);

    for (const auto& spawn : data.guardSpawns) {
        DrawCircleV(spawn, 10.0f, ColorAlpha(PURPLE, 0.6f));
        DrawCircleLinesV(spawn, 10.0f, PURPLE);
    }
}

bool GameMap::CheckWallCollision(Rectangle playerHitbox) const {
    for (const auto& rect : data.collisionRects) {
        if (CheckCollisionRecs(playerHitbox, rect)) {
            return true;
        }
    }
    return false;
}

void GameMap::UpdateTriggers(Rectangle playerHitbox) {
    currentTriggerMessage = "";
    for (size_t i = 0; i < data.triggers.size(); ++i) {
        if (CheckCollisionRecs(playerHitbox, data.triggers[i])) {
            currentTriggerMessage = (i < data.triggerNames.size()) ? data.triggerNames[i] : "unknown";
            break;
        }
    }
}

bool GameMap::TryGetTeleportDestination(Rectangle playerFootHitbox, Vector2 playerPos, Vector2& outNewPlayerPos) const {
    for (size_t i = 0; i < data.triggers.size(); ++i) {
        if (CheckCollisionRecs(playerFootHitbox, data.triggers[i])) {
            if (i < data.triggerNames.size()) {
                std::string targetKey = data.triggerNames[i] + "_t";
                auto it = data.teleportTargets.find(targetKey);
                if (it != data.teleportTargets.end()) {
                    const Rectangle& targetRect = it->second;

                    // Зміщення ніг відносно верхнього лівого кута спрайта гравця
                    float footOffsetX = playerFootHitbox.x - playerPos.x;
                    float footOffsetY = playerFootHitbox.y - playerPos.y;

                    // Центруємо ноги гравця по центру прямокутника _t
                    float targetFootX = targetRect.x + (targetRect.width - playerFootHitbox.width) * 0.5f;
                    float targetFootY = targetRect.y + (targetRect.height - playerFootHitbox.height) * 0.5f;

                    outNewPlayerPos = {
                        targetFootX - footOffsetX,
                        targetFootY - footOffsetY
                    };

                    GAME_INFO("TELEPORT", "Triggered '%s' -> moving to '%s' (%.1f, %.1f)",
                        data.triggerNames[i].c_str(), targetKey.c_str(), outNewPlayerPos.x, outNewPlayerPos.y);

                    return true;
                }
            }
        }
    }
    return false;
}


bool GameMap::TryGetActiveTriggerCenter(Rectangle footHitbox, Vector2& outTriggerCenter) const {
    for (size_t i = 0; i < data.triggers.size(); ++i) {
        if (CheckCollisionRecs(footHitbox, data.triggers[i])) {
            outTriggerCenter = {
                data.triggers[i].x + data.triggers[i].width * 0.5f,
                data.triggers[i].y + data.triggers[i].height * 0.5f
            };
            return true;
        }
    }
    return false;
}
//------------ПОШУК ТРИГЕРУ ДВЕРЕЙ В КІМНАТІ ДЛЯ НПС------------------
std::vector<Vector2> GameMap::GetAllTriggerCenters() const {
    std::vector<Vector2> centers;
    centers.reserve(data.triggers.size());
    for (const auto& trig : data.triggers) {
        centers.push_back({
            trig.x + trig.width * 0.5f,
            trig.y + trig.height * 0.5f
            });
    }
    return centers;
}