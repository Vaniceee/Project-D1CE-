#include "Map.h"
#include "MapParser.h"
#include "Logger.h"
#include "Keys.h"

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
                float rotation = 0.0f;
                bool outFlipH = false;
                bool outFlipV = false;

                int flags = (flipD ? 4 : 0) | (flipH ? 2 : 0) | (flipV ? 1 : 0);
                switch (flags) {
                case 0: rotation = 0.0f;   outFlipH = false; outFlipV = false; break;
                case 1: rotation = 0.0f;   outFlipH = false; outFlipV = true;  break;
                case 2: rotation = 0.0f;   outFlipH = true;  outFlipV = false; break;
                case 3: rotation = 180.0f; outFlipH = false; outFlipV = false; break;
                case 4: rotation = 90.0f;  outFlipH = false; outFlipV = true;  break;
                case 5: rotation = 270.0f; outFlipH = false; outFlipV = false; break;
                case 6: rotation = 90.0f;  outFlipH = false; outFlipV = false; break;
                case 7: rotation = 90.0f;  outFlipH = true;  outFlipV = false; break;
                }

                if (outFlipH) srcRec.width = -srcRec.width;
                if (outFlipV) srcRec.height = -srcRec.height;

                Rectangle destRec = {
                    posX + (float)data.tileWidth * 0.5f,
                    posY + (float)data.tileHeight * 0.5f,
                    (float)activeTileset->tileWidth,
                    (float)activeTileset->tileHeight
                };
                Vector2 origin = {
                    (float)activeTileset->tileWidth * 0.5f,
                    (float)activeTileset->tileHeight * 0.5f
                };

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
    bool outFlipH = false;
    bool outFlipV = false;

    int flags = (flipD ? 4 : 0) | (flipH ? 2 : 0) | (flipV ? 1 : 0);
    switch (flags) {
    case 0: rotation = 0.0f;   outFlipH = false; outFlipV = false; break;
    case 1: rotation = 0.0f;   outFlipH = false; outFlipV = true;  break;
    case 2: rotation = 0.0f;   outFlipH = true;  outFlipV = false; break;
    case 3: rotation = 180.0f; outFlipH = false; outFlipV = false; break;
    case 4: rotation = 90.0f;  outFlipH = false; outFlipV = true;  break;
    case 5: rotation = 270.0f; outFlipH = false; outFlipV = false; break;
    case 6: rotation = 90.0f;  outFlipH = false; outFlipV = false; break;
    case 7: rotation = 90.0f;  outFlipH = true;  outFlipV = false; break;
    }

    if (outFlipH) srcRec.width = -srcRec.width;
    if (outFlipV) srcRec.height = -srcRec.height;

    float preRotWidth = flipD ? obj.height : obj.width;
    float preRotHeight = flipD ? obj.width : obj.height;

    Rectangle destRec = {
        obj.x + obj.width * 0.5f,
        obj.y - obj.height * 0.5f,
        preRotWidth,
        preRotHeight
    };
    Vector2 origin = { preRotWidth * 0.5f, preRotHeight * 0.5f };

    DrawTexturePro(tex, srcRec, destRec, origin, rotation, WHITE);
}

void GameMap::DrawDebug() const {
    // 1. Ручні колізії з окремого шару — темніший червоний (MAROON)
    for (const auto& rect : data.collisionRects) {
        DrawRectangleRec(rect, ColorAlpha(MAROON, 0.45f));
        DrawRectangleLinesEx(rect, 1.2f, Color{ 165, 20, 35, 255 });
    }

    // 1.5 Автоматичні колізії всередині тайлів/об'єктів — світліший яскраво-червоний
    for (const auto& rect : data.autoCollisionRects) {
        DrawRectangleRec(rect, ColorAlpha(Color{ 255, 85, 85, 255 }, 0.35f));
        DrawRectangleLinesEx(rect, 1.0f, Color{ 255, 110, 110, 255 });
    }

    // 2. Тригери переходів/кімнат — напівпрозорий зелений
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

    // 4. Інтерактивні об'єкти (_i) — напівпрозорий рожевий (PINK)
    for (const auto& obj : data.interactables) {
        float fillAlpha = obj.isUsed ? 0.15f : 0.40f;
        Color borderCol = obj.isUsed ? GRAY : PINK;
        DrawRectangleRec(obj.bounds, ColorAlpha(PINK, fillAlpha));
        DrawRectangleLinesEx(obj.bounds, 1.5f, borderCol);

        const char* label = TextFormat("%s [%s]%s", obj.fullId.c_str(), obj.type.c_str(), obj.isUsed ? " (USED)" : "");
        DrawText(label, (int)obj.bounds.x, (int)obj.bounds.y - 12, 10, borderCol);
    }

    // 5. Спавни об'єктів (точки) — напівпрозорий фіолетовий
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
    for (const auto& rect : data.autoCollisionRects) {
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


//---------- Розподіл спавну ключів --------------
void GameMap::DistributeLoot() {
    std::vector<InteractableObject*> outerLockers;
    std::vector<InteractableObject*> guardRoomLockers;
    InteractableObject* guardSafe = nullptr;

    for (auto& obj : data.interactables) {
        if (obj.type == "lock") {
            if (obj.fullId == "lock_8" || obj.fullId == "lock_9") {
                guardRoomLockers.push_back(&obj);
            }
            else {
                outerLockers.push_back(&obj);
            }
        }
        else if (obj.type == "safe") {
            guardSafe = &obj;
        }
    }

    // 1. Ключ від кімнати охоронця — у зовнішніх шафках
    if (!outerLockers.empty()) {
        int idx = GetRandomValue(0, (int)outerLockers.size() - 1);
        InteractableObject* chosen = outerLockers[idx];
        Vector2 pos = { chosen->bounds.x + chosen->bounds.width * 0.5f, chosen->bounds.y + chosen->bounds.height * 0.5f };
        chosen->storedItem = std::make_shared<RoomKey>(pos);
        GAME_INFO("LOOT", "Spawned '%s' inside '%s'", chosen->storedItem->GetName().c_str(), chosen->fullId.c_str());
    }
    else {
        GAME_WARN("LOOT", "No outer lockers found for RoomKey!");
    }

    // 2. Картка від сейфа — у шафках кімнати охоронця (lock_8 або lock_9)
    if (!guardRoomLockers.empty()) {
        int idx = GetRandomValue(0, (int)guardRoomLockers.size() - 1);
        InteractableObject* chosen = guardRoomLockers[idx];
        Vector2 pos = { chosen->bounds.x + chosen->bounds.width * 0.5f, chosen->bounds.y + chosen->bounds.height * 0.5f };
        chosen->storedItem = std::make_shared<SafeKeyCard>(pos);
        GAME_INFO("LOOT", "Spawned '%s' inside '%s'", chosen->storedItem->GetName().c_str(), chosen->fullId.c_str());
    }
    else {
        GAME_WARN("LOOT", "No guard room lockers (lock_8/lock_9) found for SafeKeyCard!");
    }

    // 3. Головний ключ від виходу — у сейфі
    if (guardSafe != nullptr) {
        Vector2 pos = { guardSafe->bounds.x + guardSafe->bounds.width * 0.5f, guardSafe->bounds.y + guardSafe->bounds.height * 0.5f };
        guardSafe->storedItem = std::make_shared<MasterKey>(pos);
        GAME_INFO("LOOT", "Spawned '%s' inside '%s'", guardSafe->storedItem->GetName().c_str(), guardSafe->fullId.c_str());
    }
    else {
        GAME_WARN("LOOT", "No safe found for MasterKey!");
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

                    float footOffsetX = playerFootHitbox.x - playerPos.x;
                    float footOffsetY = playerFootHitbox.y - playerPos.y;

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

InteractableObject* GameMap::GetInteractableAt(Rectangle footHitbox) {
    for (auto& obj : data.interactables) {
        if (!obj.isUsed && CheckCollisionRecs(footHitbox, obj.bounds)) {
            return &obj;
        }
    }
    return nullptr;
}