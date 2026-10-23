#include "AssetManager.h"

void AssetManager::LoadTexture(const std::string& name, const std::string& filePath) {
    if (textures.find(name) == textures.end()) {
        textures[name] = ::LoadTexture(filePath.c_str());
    }
}

Texture2D AssetManager::GetTexture(const std::string& name) {
    return textures.count(name) ? textures[name] : Texture2D{ 0 };
}

void AssetManager::UnloadAll() {
    for (auto& pair : textures) {
        ::UnloadTexture(pair.second);
    }
    textures.clear();
}