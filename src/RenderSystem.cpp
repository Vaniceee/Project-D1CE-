#include "RenderSystem.h"
#include <algorithm>
#include <cmath>

namespace {
    void DrawFloorKeyCap(float x, float y, float w, float h, const char* label, float alpha, bool isWide = false) {
        Rectangle glowRect = { x - 2.5f, y - 2.5f, w + 5.0f, h + 7.5f };
        DrawRectangleRounded(glowRect, 0.24f, 6, ColorAlpha(GOLD, 0.14f * alpha));

        float bevelDepth = 3.2f;
        Rectangle shadowRect = { x + 1.5f, y + bevelDepth + 1.5f, w, h };
        DrawRectangleRounded(shadowRect, 0.20f, 6, ColorAlpha(BLACK, 0.65f * alpha));

        Rectangle baseRect = { x, y + bevelDepth, w, h };
        DrawRectangleRounded(baseRect, 0.20f, 6, ColorAlpha(Color{ 85, 64, 24, 255 }, 0.88f * alpha));
        DrawRectangleRoundedLinesEx(baseRect, 0.20f, 6, 1.0f, ColorAlpha(GOLD, 0.55f * alpha));

        Rectangle topRect = { x, y, w, h };
        DrawRectangleRounded(topRect, 0.20f, 6, ColorAlpha(Color{ 14, 15, 19, 255 }, 0.84f * alpha));

        DrawLineEx(
            { x + 4.0f, y + 1.8f },
            { x + w - 4.0f, y + 1.8f },
            1.0f,
            ColorAlpha(Color{ 255, 236, 175, 255 }, 0.35f * alpha)
        );

        DrawRectangleRoundedLinesEx(topRect, 0.20f, 6, 1.3f, ColorAlpha(GOLD, 0.88f * alpha));
        Rectangle innerRect = { x + 2.5f, y + 2.5f, w - 5.0f, h - 5.0f };
        DrawRectangleRoundedLinesEx(innerRect, 0.16f, 4, 0.7f, ColorAlpha(GOLD, 0.28f * alpha));

        int fontSize = isWide ? 11 : 12;
        int tw = MeasureText(label, fontSize);
        int tx = (int)roundf(x + (w - (float)tw) * 0.5f);
        int ty = (int)roundf(y + (h - (float)fontSize) * 0.5f);

        DrawText(label, tx + 1, ty + 1, fontSize, ColorAlpha(BLACK, 0.85f * alpha));
        DrawText(label, tx, ty, fontSize, ColorAlpha(Color{ 255, 235, 165, 255 }, 0.96f * alpha));
    }

    void DrawFloorHintCaption(float centerX, float y, const char* text, float alpha) {
        int fontSize = 11;
        int tw = MeasureText(text, fontSize);
        float textX = centerX - (float)tw * 0.5f;

        float lineSpan = 13.0f;
        Color decoCol = ColorAlpha(GOLD, 0.52f * alpha);
        DrawLineEx({ textX - 6.0f - lineSpan, y + fontSize * 0.5f }, { textX - 6.0f, y + fontSize * 0.5f }, 1.0f, decoCol);
        DrawLineEx({ textX + (float)tw + 6.0f, y + fontSize * 0.5f }, { textX + (float)tw + 6.0f + lineSpan, y + fontSize * 0.5f }, 1.0f, decoCol);

        DrawText(text, (int)roundf(textX) + 1, (int)roundf(y) + 1, fontSize, ColorAlpha(BLACK, 0.85f * alpha));
        DrawText(text, (int)roundf(textX), (int)roundf(y), fontSize, ColorAlpha(Color{ 245, 222, 148, 255 }, 0.95f * alpha));
    }
}

void RenderSystem::DrawFloorTutorialHint(const FloorTutorialHint& hint) const {
    if (hint.moveAlpha <= 0.01f && hint.sprintAlpha <= 0.01f) return;

    float cx = hint.anchorPos.x;
    float topY = hint.anchorPos.y - 144.0f;
    float pulse = 0.88f + 0.12f * sinf(hint.time * 2.8f);

    float keySize = 21.0f;
    float keyGap = 4.0f;
    float wasdBlockW = keySize * 3.0f + keyGap * 2.0f;
    float shiftW = 68.0f;
    float shiftH = 21.0f;
    float groupGap = 28.0f;

    float totalW = wasdBlockW + groupGap + shiftW;
    float leftGroupX = cx - totalW * 0.5f;
    float rightGroupX = leftGroupX + wasdBlockW + groupGap;

    float row1Y = topY;
    float row2Y = topY + keySize + keyGap + 2.5f;
    float captionY = row2Y + keySize + 9.5f;

    float combinedAlpha = fmaxf(hint.moveAlpha, hint.sprintAlpha) * pulse;
    if (combinedAlpha > 0.02f) {
        float sepX = leftGroupX + wasdBlockW + groupGap * 0.5f;
        float sepTop = row1Y + 3.0f;
        float sepBot = captionY + 10.0f;
        float sepMid = (sepTop + sepBot) * 0.5f;

        DrawLineEx({ sepX, sepTop }, { sepX, sepMid - 5.5f }, 1.0f, ColorAlpha(GOLD, 0.38f * combinedAlpha));
        DrawLineEx({ sepX, sepMid + 5.5f }, { sepX, sepBot }, 1.0f, ColorAlpha(GOLD, 0.38f * combinedAlpha));
        DrawPoly({ sepX, sepMid }, 4, 3.6f, 0.0f, ColorAlpha(GOLD, 0.78f * combinedAlpha));
    }

    if (hint.moveAlpha > 0.01f) {
        float a = hint.moveAlpha * pulse;
        float wasdCenterX = leftGroupX + wasdBlockW * 0.5f;

        DrawFloorKeyCap(leftGroupX + keySize + keyGap, row1Y, keySize, keySize, "W", a);
        DrawFloorKeyCap(leftGroupX, row2Y, keySize, keySize, "A", a);
        DrawFloorKeyCap(leftGroupX + keySize + keyGap, row2Y, keySize, keySize, "S", a);
        DrawFloorKeyCap(leftGroupX + (keySize + keyGap) * 2.0f, row2Y, keySize, keySize, "D", a);

        DrawFloorHintCaption(wasdCenterX, captionY, "MOVE", a);
    }

    if (hint.sprintAlpha > 0.01f) {
        float a = hint.sprintAlpha * pulse;
        float shiftCenterX = rightGroupX + shiftW * 0.5f;

        float badgeY = row1Y + keySize * 0.5f + 1.5f;
        DrawPolyLinesEx({ shiftCenterX, badgeY }, 4, 6.4f, 0.0f, 1.0f, ColorAlpha(GOLD, 0.45f * a));
        DrawPoly({ shiftCenterX, badgeY }, 4, 2.4f, 0.0f, ColorAlpha(GOLD, 0.85f * a));

        DrawFloorKeyCap(rightGroupX, row2Y, shiftW, shiftH, "SHIFT", a, true);
        DrawFloorHintCaption(shiftCenterX, captionY, "SPRINT", a);
    }
}

void RenderSystem::DrawObjectShadows(const std::vector<MapVisualObject>& mapObjects) const {
    for (const auto& obj : mapObjects) {
        // Пропускаємо підлоговий декор (килими тощо мають layerOrder = -1)
        if (obj.layerOrder < 0) continue;

        // Пропускаємо дрібні предмети, що стоять зверху на столах (у них sortY змінено відносно obj.y)
        if (std::abs(obj.sortY - obj.y) > 0.1f) continue;

        // У Tiled obj.x та obj.y — це нижній лівий кут об'єкта
        int shadowCenterX = (int)roundf(obj.x + obj.width * 0.5f);
        int shadowCenterY = (int)roundf(obj.y - 3.0f);

        // Видовжений горизонтальний еліпс під основою об'єкта
        float shadowRadiusX = obj.width * 0.48f;
        float shadowRadiusY = fmaxf(5.0f, fminf(obj.height * 0.18f, obj.width * 0.20f));

        // Двошарова тінь у тому ж стилі, що й у гравця
        DrawEllipse(shadowCenterX, shadowCenterY, shadowRadiusX * 1.18f, shadowRadiusY * 1.22f, ColorAlpha(BLACK, 0.20f));
        DrawEllipse(shadowCenterX, shadowCenterY, shadowRadiusX, shadowRadiusY, ColorAlpha(BLACK, 0.40f));
    }
}

void RenderSystem::RenderWorld(const GameMap& gameMap, Player* player, Guard* guard, const FloorTutorialHint* floorHint) {
    // 1. Шари під гравцем (підлога, килими)
    gameMap.DrawBelowPlayer();

    // 1.5 Підказки керування на підлозі
    if (floorHint != nullptr) {
        DrawFloorTutorialHint(*floorHint);
    }

    // 1.8 Контактні тіні об'єктів на підлозі
    const auto& mapObjects = gameMap.GetVisualObjects();
    DrawObjectShadows(mapObjects);

    // 2. Збір об'єктів для сортування
    renderQueue.clear();
    renderQueue.reserve(mapObjects.size() + 2);

    for (const auto& obj : mapObjects) {
        renderQueue.push_back({ obj.sortY, obj.layerOrder, RenderItem::Type::MAP_OBJECT, &obj });
    }

    if (player) {
        Rectangle foot = player->GetFootCollider();
        renderQueue.push_back({ foot.y + foot.height, 0, RenderItem::Type::PLAYER, nullptr });
    }

    if (guard) {
        Rectangle guardFoot = guard->GetFootCollider();
        renderQueue.push_back({ guardFoot.y + guardFoot.height, 0, RenderItem::Type::GUARD, nullptr });
    }

    // 3. Сортування:
    // - Підлогові об'єкти (order < 0) завжди під ногами.
    // - Решта сортується по sortY (при збігу — за природним порядком шарів у Tiled).
    std::sort(renderQueue.begin(), renderQueue.end(), [](const RenderItem& a, const RenderItem& b) {
        bool aIsFloor = (a.order < 0);
        bool bIsFloor = (b.order < 0);
        if (aIsFloor != bIsFloor) {
            return aIsFloor;
        }

        if (std::abs(a.sortY - b.sortY) < 0.01f) {
            return a.order < b.order;
        }
        return a.sortY < b.sortY;
        });

    // 4. Рендеринг відсортованої черги
    for (const auto& item : renderQueue) {
        switch (item.type) {
        case RenderItem::Type::MAP_OBJECT:
            gameMap.DrawVisualObject(*item.mapObj);
            break;
        case RenderItem::Type::PLAYER:
            player->Draw();
            break;
        case RenderItem::Type::GUARD:
            guard->Draw();
            break;
        }
    }

    // 5. Шари над гравцем (дах, верхні частини стін)
    gameMap.DrawAbovePlayer();
}