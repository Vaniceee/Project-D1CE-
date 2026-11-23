#include "Map.h"
#include "MapParser.h"
#include "Logger.h"
#include "Keys.h"
#include <algorithm>
#include <cmath>

bool GameMap::Load(const std::string& mapPath, const std::string& defaultTilesetKey) {
    Unload();
    return MapParser::Parse(mapPath, defaultTilesetKey, data);
}

void GameMap::Unload() {
    data = MapData{};
    currentTriggerMessage = "";
}

int GameMap::GetRoomIndexAtPoint(Vector2 worldPos) const {
    for (size_t i = 0; i < data.rooms.size(); ++i) {
        if (CheckCollisionPointRec(worldPos, data.rooms[i].bounds)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int GameMap::GetRoomIndexAtRect(Rectangle rect) const {
    Vector2 center = { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
    int idx = GetRoomIndexAtPoint(center);
    if (idx != -1) return idx;

    for (size_t i = 0; i < data.rooms.size(); ++i) {
        if (CheckCollisionRecs(rect, data.rooms[i].bounds)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void GameMap::DrawBelowPlayer(Rectangle viewRect, int activeRoomIndex) const {
    for (const auto& layerData : data.belowPlayerLayers) {
        DrawSingleLayer(layerData, viewRect, activeRoomIndex);
    }
}

void GameMap::DrawAbovePlayer(Rectangle viewRect, int activeRoomIndex) const {
    for (const auto& layerData : data.abovePlayerLayers) {
        DrawSingleLayer(layerData, viewRect, activeRoomIndex);
    }
}

void GameMap::DrawSingleLayer(const std::vector<int>& layerData, Rectangle viewRect, int activeRoomIndex) const {
    if (layerData.empty() || data.tilesets.empty() || data.tileWidth <= 0 || data.tileHeight <= 0) return;

    // 1. Відсікання тайлів за межами прямокутника видимості камери (Frustum Culling)
    int startX = (std::max)(0, static_cast<int>(std::floor(viewRect.x / static_cast<float>(data.tileWidth))));
    int startY = (std::max)(0, static_cast<int>(std::floor(viewRect.y / static_cast<float>(data.tileHeight))));
    int endX = (std::min)(data.mapWidth - 1, static_cast<int>(std::floor((viewRect.x + viewRect.width) / static_cast<float>(data.tileWidth))));
    int endY = (std::min)(data.mapHeight - 1, static_cast<int>(std::floor((viewRect.y + viewRect.height) / static_cast<float>(data.tileHeight))));

    if (startX > endX || startY > endY) return;

    // 2. Якщо на карті є шар кімнат і гравець знаходиться у конкретній кімнаті —
    // додатково звужуємо цикл по тайлах до меж цієї кімнати
    bool restrictToRoom = (!data.rooms.empty() && activeRoomIndex >= 0 && activeRoomIndex < static_cast<int>(data.rooms.size()));
    Rectangle activeRoomBounds = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (restrictToRoom) {
        activeRoomBounds = data.rooms[activeRoomIndex].bounds;
        int roomStartX = (std::max)(0, static_cast<int>(std::floor(activeRoomBounds.x / static_cast<float>(data.tileWidth))));
        int roomStartY = (std::max)(0, static_cast<int>(std::floor(activeRoomBounds.y / static_cast<float>(data.tileHeight))));
        int roomEndX = (std::min)(data.mapWidth - 1, static_cast<int>(std::floor((activeRoomBounds.x + activeRoomBounds.width - 0.01f) / static_cast<float>(data.tileWidth))));
        int roomEndY = (std::min)(data.mapHeight - 1, static_cast<int>(std::floor((activeRoomBounds.y + activeRoomBounds.height - 0.01f) / static_cast<float>(data.tileHeight))));

        startX = (std::max)(startX, roomStartX);
        startY = (std::max)(startY, roomStartY);
        endX = (std::min)(endX, roomEndX);
        endY = (std::min)(endY, roomEndY);

        if (startX > endX || startY > endY) return;
    }

    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
    constexpr uint32_t ALL_FLIP_FLAGS = (FLIPPED_HORIZONTALLY_FLAG |
        FLIPPED_VERTICALLY_FLAG |
        FLIPPED_DIAGONALLY_FLAG);

    for (int y = startY; y <= endY; ++y) {
        float posY = static_cast<float>(y * data.tileHeight);
        float tileCenterY = posY + static_cast<float>(data.tileHeight) * 0.5f;

        for (int x = startX; x <= endX; ++x) {
            float posX = static_cast<float>(x * data.tileWidth);

            // Перевіряємо, щоб центр тайла точно належав активній кімнаті
            if (restrictToRoom) {
                float tileCenterX = posX + static_cast<float>(data.tileWidth) * 0.5f;
                if (!CheckCollisionPointRec({ tileCenterX, tileCenterY }, activeRoomBounds)) {
                    continue;
                }
            }

            int idx = x + y * data.mapWidth;
            if (idx < 0 || idx >= static_cast<int>(layerData.size())) continue;

            uint32_t rawGlobalTileId = static_cast<uint32_t>(layerData[idx]);
            if (rawGlobalTileId == 0) continue;

            uint32_t globalTileId = rawGlobalTileId & ~ALL_FLIP_FLAGS;
            if (globalTileId == 0) continue;

            const TilesetInfo* activeTileset = nullptr;
            int localTileId = 0;

            for (int i = static_cast<int>(data.tilesets.size()) - 1; i >= 0; --i) {
                if (globalTileId >= static_cast<uint32_t>(data.tilesets[i].firstgid)) {
                    activeTileset = &data.tilesets[i];
                    localTileId = static_cast<int>(globalTileId) - data.tilesets[i].firstgid;
                    break;
                }
            }

            if (!activeTileset || activeTileset->texture.id == 0) continue;

            int tsCols = activeTileset->texture.width / activeTileset->tileWidth;
            if (tsCols <= 0) continue;

            int tsX = (localTileId % tsCols) * activeTileset->tileWidth;
            int tsY = (localTileId / tsCols) * activeTileset->tileHeight;

            Rectangle srcRec = {
                static_cast<float>(tsX),
                static_cast<float>(tsY),
                static_cast<float>(activeTileset->tileWidth),
                static_cast<float>(activeTileset->tileHeight)
            };

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
                    posX + static_cast<float>(data.tileWidth) * 0.5f,
                    posY + static_cast<float>(data.tileHeight) * 0.5f,
                    static_cast<float>(activeTileset->tileWidth),
                    static_cast<float>(activeTileset->tileHeight)
                };
                Vector2 origin = {
                    static_cast<float>(activeTileset->tileWidth) * 0.5f,
                    static_cast<float>(activeTileset->tileHeight) * 0.5f
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
    for (int i = static_cast<int>(data.tilesets.size()) - 1; i >= 0; --i) {
        if (gid >= static_cast<uint32_t>(data.tilesets[i].firstgid)) {
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
        srcRec = { 0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height) };
    }
    else if (activeTs->texture.id != 0) {
        tex = activeTs->texture;
        int tsCols = tex.width / activeTs->tileWidth;
        if (tsCols <= 0) return;
        srcRec = {
            static_cast<float>((localId % tsCols) * activeTs->tileWidth),
            static_cast<float>((localId / tsCols) * activeTs->tileHeight),
            static_cast<float>(activeTs->tileWidth),
            static_cast<float>(activeTs->tileHeight)
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

void GameMap::DrawDebug(int activeRoomIndex) const {
    // 0. Межі кімнат із шару "rooms" — бірюзовий контур
    for (size_t i = 0; i < data.rooms.size(); ++i) {
        bool isCurrent = (static_cast<int>(i) == activeRoomIndex);
        Color roomCol = isCurrent ? Color{ 0, 235, 210, 255 } : Color{ 70, 130, 150, 180 };
        DrawRectangleLinesEx(data.rooms[i].bounds, isCurrent ? 2.5f : 1.2f, roomCol);
        const char* roomLabel = isCurrent
            ? TextFormat("ROOM: %s [ACTIVE]", data.rooms[i].name.c_str())
            : TextFormat("ROOM: %s", data.rooms[i].name.c_str());
        DrawText(roomLabel, static_cast<int>(data.rooms[i].bounds.x) + 6, static_cast<int>(data.rooms[i].bounds.y) + 6, 12, roomCol);
    }

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
            DrawText(data.triggerNames[i].c_str(), static_cast<int>(data.triggers[i].x), static_cast<int>(data.triggers[i].y) - 12, 10, GREEN);
        }
    }

    // 3. Таргети телепортів (_t) — напівпрозорий блакитний
    for (const auto& [name, rect] : data.teleportTargets) {
        DrawRectangleRec(rect, ColorAlpha(SKYBLUE, 0.45f));
        DrawRectangleLinesEx(rect, 1.0f, SKYBLUE);
        DrawText(name.c_str(), static_cast<int>(rect.x), static_cast<int>(rect.y) - 12, 10, SKYBLUE);
    }

    // 4. Інтерактивні об'єкти (_i) — напівпрозорий рожевий (PINK)
    for (const auto& obj : data.interactables) {
        float fillAlpha = obj.isUsed ? 0.15f : 0.40f;
        Color borderCol = obj.isUsed ? GRAY : PINK;
        DrawRectangleRec(obj.bounds, ColorAlpha(PINK, fillAlpha));
        DrawRectangleLinesEx(obj.bounds, 1.5f, borderCol);

        const char* label = TextFormat("%s [%s]%s", obj.fullId.c_str(), obj.type.c_str(), obj.isUsed ? " (USED)" : "");
        DrawText(label, static_cast<int>(obj.bounds.x), static_cast<int>(obj.bounds.y) - 12, 10, borderCol);
    }

    // 5. Спавни об'єктів (точки) — напівпрозорий фіолетовий
    DrawCircleV(data.playerSpawn, 10.0f, ColorAlpha(PURPLE, 0.6f));
    DrawCircleLinesV(data.playerSpawn, 10.0f, PURPLE);

    for (const auto& spawn : data.guardSpawns) {
        DrawCircleV(spawn, 10.0f, ColorAlpha(PURPLE, 0.6f));
        DrawCircleLinesV(spawn, 10.0f, PURPLE);
    }

    // 6. Точки маршруту патрулювання охоронця — бурштинові ромби з номерами
    for (size_t i = 0; i < data.guardPatrolPoints.size(); ++i) {
        Vector2 pt = data.guardPatrolPoints[i];
        DrawPoly(pt, 4, 8.0f, 0.0f, ColorAlpha(ORANGE, 0.75f));
        DrawPolyLinesEx(pt, 4, 8.0f, 0.0f, 1.5f, GOLD);
        DrawText(TextFormat("P%d", static_cast<int>(i)), static_cast<int>(pt.x) + 10, static_cast<int>(pt.y) - 6, 12, GOLD);

        if (i + 1 < data.guardPatrolPoints.size()) {
            DrawLineEx(pt, data.guardPatrolPoints[i + 1], 1.2f, ColorAlpha(ORANGE, 0.45f));
        }
        else if (data.guardPatrolPoints.size() > 1) {
            DrawLineEx(pt, data.guardPatrolPoints[0], 1.2f, ColorAlpha(ORANGE, 0.25f));
        }
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
        int idx = GetRandomValue(0, static_cast<int>(outerLockers.size()) - 1);
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
        int idx = GetRandomValue(0, static_cast<int>(guardRoomLockers.size()) - 1);
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

std::vector<GameMap::TeleportLink> GameMap::GetAllTeleportLinks() const {
    std::vector<TeleportLink> links;
    for (size_t i = 0; i < data.triggers.size(); ++i) {
        if (i >= data.triggerNames.size()) continue;
        std::string targetKey = data.triggerNames[i] + "_t";
        auto it = data.teleportTargets.find(targetKey);
        if (it != data.teleportTargets.end()) {
            TeleportLink link;
            link.triggerCenter = {
                data.triggers[i].x + data.triggers[i].width * 0.5f,
                data.triggers[i].y + data.triggers[i].height * 0.5f
            };
            link.destinationCenter = {
                it->second.x + it->second.width * 0.5f,
                it->second.y + it->second.height * 0.5f
            };
            links.push_back(link);
        }
    }
    return links;
}

InteractableObject* GameMap::GetInteractableAt(Rectangle footHitbox) {
    for (auto& obj : data.interactables) {
        if (!obj.isUsed && CheckCollisionRecs(footHitbox, obj.bounds)) {
            return &obj;
        }
    }
    return nullptr;
}