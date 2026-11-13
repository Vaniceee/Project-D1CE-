#include "RenderSystem.h"
#include <algorithm>
#include <cmath>

void RenderSystem::RenderWorld(const GameMap& gameMap, Player* player, Guard* guard) {
    // 1. Шари під гравцем (підлога, килими)
    gameMap.DrawBelowPlayer();

    // 2. Збір об'єктів для Y-сортування (очищаємо без звільнення виділеної пам'яті)
    renderQueue.clear();
    const auto& mapObjects = gameMap.GetVisualObjects();
    renderQueue.reserve(mapObjects.size() + 2);

    for (const auto& obj : mapObjects) {
        renderQueue.push_back({ obj.y, obj.layerOrder, RenderItem::Type::MAP_OBJECT, &obj });
    }

    if (player) {
        Rectangle foot = player->GetFootCollider();
        renderQueue.push_back({ foot.y + foot.height, 100, RenderItem::Type::PLAYER, nullptr });
    }

    if (guard) {
        Rectangle guardFoot = guard->GetFootCollider();
        renderQueue.push_back({ guardFoot.y + guardFoot.height, 100, RenderItem::Type::GUARD, nullptr });
    }

    // 3. Сортування за координатою Y (глибина сцени)
    std::sort(renderQueue.begin(), renderQueue.end(), [](const RenderItem& a, const RenderItem& b) {
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