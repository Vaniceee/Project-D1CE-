#pragma once
#include "raylib.h"
#include <vector>

class GameMap;

class AStar {
public:
    void BuildGrid(const GameMap& gameMap);
    std::vector<Vector2> FindPath(Vector2 startWorld, Vector2 goalWorld) const;

private:
    int width = 0;
    int height = 0;
    int tileWidth = 32;
    int tileHeight = 32;
    std::vector<bool> walkable;

    bool IsValid(int x, int y) const;
    bool IsWalkable(int x, int y) const;
    void FindNearestWalkable(int& x, int& y) const;
};
