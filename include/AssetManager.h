#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>

class AssetManager {
public:
    static void LoadTexture(const std::string& name, const std::string& filePath);
    static Texture2D GetTexture(const std::string& name);
    static void UnloadAll();

private:
    inline static std::unordered_map<std::string, Texture2D> textures;
};
