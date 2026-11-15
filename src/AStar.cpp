#include "AStar.h"
#include "Map.h"
#include <queue>
#include <cmath>
#include <algorithm>

void AStar::BuildGrid(const GameMap& gameMap) {
    width = gameMap.GetWidth();
    height = gameMap.GetHeight();
    tileWidth = gameMap.GetTileWidth();
    tileHeight = gameMap.GetTileHeight();

    const int totalNodes = width * height;
    walkable.assign(totalNodes, true);
    zoneIds.assign(totalNodes, -1);

    gScore.resize(totalNodes);
    cameFrom.resize(totalNodes);
    closed.resize(totalNodes);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Rectangle tileCheck = {
                x * (float)tileWidth + tileWidth * 0.25f,
                y * (float)tileHeight + tileHeight * 0.25f,
                tileWidth * 0.5f,
                tileHeight * 0.5f
            };
            if (gameMap.CheckWallCollision(tileCheck)) {
                walkable[y * width + x] = false;
            }
        }
    }

    // Попередня розмітка зв'язних зон (кімнат/коридорів) через BFS
    int currentZone = 0;
    std::queue<int> q;
    const int dirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

    for (int i = 0; i < totalNodes; ++i) {
        if (!walkable[i] || zoneIds[i] != -1) continue;

        zoneIds[i] = currentZone;
        q.push(i);

        while (!q.empty()) {
            int curr = q.front();
            q.pop();

            int cx = curr % width;
            int cy = curr / width;

            for (const auto& d : dirs) {
                int nx = cx + d[0];
                int ny = cy + d[1];
                if (!IsWalkable(nx, ny)) continue;

                int nIdx = ny * width + nx;
                if (zoneIds[nIdx] == -1) {
                    zoneIds[nIdx] = currentZone;
                    q.push(nIdx);
                }
            }
        }
        currentZone++;
    }
}

bool AStar::IsValid(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

bool AStar::IsWalkable(int x, int y) const {
    return IsValid(x, y) && walkable[y * width + x];
}

void AStar::FindNearestWalkable(int& x, int& y) const {
    if (IsWalkable(x, y)) return;
    for (int r = 1; r <= 2; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (IsWalkable(x + dx, y + dy)) {
                    x += dx;
                    y += dy;
                    return;
                }
            }
        }
    }
}

std::vector<Vector2> AStar::FindPath(Vector2 startWorld, Vector2 goalWorld) const {
    std::vector<Vector2> path;
    if (width <= 0 || height <= 0 || walkable.empty()) return path;

    int startX = (int)(startWorld.x / tileWidth);
    int startY = (int)(startWorld.y / tileHeight);
    int goalX = (int)(goalWorld.x / tileWidth);
    int goalY = (int)(goalWorld.y / tileHeight);

    FindNearestWalkable(startX, startY);
    FindNearestWalkable(goalX, goalY);

    if (!IsWalkable(startX, startY) || !IsWalkable(goalX, goalY)) return path;

    int startIdx = startY * width + startX;
    int goalIdx = goalY * width + goalX;

    // Миттєвий вихід O(1), якщо старт і фініш у різних ізольованих кімнатах
    if (zoneIds[startIdx] != zoneIds[goalIdx]) {
        return path;
    }

    if (startIdx == goalIdx) {
        path.push_back(goalWorld);
        return path;
    }

    std::fill(gScore.begin(), gScore.end(), 1e9f);
    std::fill(cameFrom.begin(), cameFrom.end(), -1);
    std::fill(closed.begin(), closed.end(), 0);

    struct Node {
        int idx;
        float fScore;
        bool operator>(const Node& other) const { return fScore > other.fScore; }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;

    auto heuristic = [&](int x1, int y1, int x2, int y2) -> float {
        float dx = (float)std::abs(x2 - x1);
        float dy = (float)std::abs(y2 - y1);
        return (dx + dy) + (1.4142f - 2.0f) * fminf(dx, dy);
        };

    gScore[startIdx] = 0.0f;
    openSet.push({ startIdx, heuristic(startX, startY, goalX, goalY) });

    const int dirs[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    while (!openSet.empty()) {
        int current = openSet.top().idx;
        openSet.pop();

        if (current == goalIdx) {
            int curr = goalIdx;
            while (curr != -1 && curr != startIdx) {
                int cx = curr % width;
                int cy = curr / width;
                path.push_back({
                    cx * (float)tileWidth + tileWidth * 0.5f,
                    cy * (float)tileHeight + tileHeight * 0.5f
                    });
                curr = cameFrom[curr];
            }
            std::reverse(path.begin(), path.end());
            if (!path.empty()) {
                path.back() = goalWorld;
            }
            return path;
        }

        if (closed[current]) continue;
        closed[current] = 1;

        int cx = current % width;
        int cy = current / width;

        for (int i = 0; i < 8; ++i) {
            int dx = dirs[i][0];
            int dy = dirs[i][1];
            int nx = cx + dx;
            int ny = cy + dy;

            if (!IsWalkable(nx, ny)) continue;

            bool isDiagonal = (dx != 0 && dy != 0);
            if (isDiagonal) {
                if (!IsWalkable(cx + dx, cy) || !IsWalkable(cx, cy + dy)) {
                    continue;
                }
            }

            int neighborIdx = ny * width + nx;
            if (closed[neighborIdx]) continue;

            float stepCost = isDiagonal ? 1.4142f : 1.0f;
            float tentativeG = gScore[current] + stepCost;

            if (tentativeG < gScore[neighborIdx]) {
                cameFrom[neighborIdx] = current;
                gScore[neighborIdx] = tentativeG;
                openSet.push({ neighborIdx, tentativeG + heuristic(nx, ny, goalX, goalY) });
            }
        }
    }

    return path;
}