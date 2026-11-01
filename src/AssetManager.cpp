#include "AssetManager.h"
#include <iostream>
#include "Logger.h"

AssetManager& AssetManager::Instance() {
    static AssetManager instance;
    return instance;
}

void AssetManager::LoadTexture(const std::string& name, const std::string& path) {
    if (textures.find(name) == textures.end()) {
        Texture2D tex = ::LoadTexture(path.c_str());

        if (tex.id > 0) {
            textures[name] = tex;
            GAME_DEBUG("ASSET", "Texture loaded: '%s' from %s", name.c_str(), path.c_str());
        }
        else {
            GAME_ERROR("ASSET", "Failed to load texture '%s' from %s", name.c_str(), path.c_str());
        }
    }
}

Texture2D AssetManager::GetTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second;
    }
    return Texture2D{ 0 };
}

void AssetManager::LoadSound(const std::string& name, const std::string& path) {
    if (sounds.find(name) == sounds.end()) {
        Sound snd = ::LoadSound(path.c_str());
        if (snd.stream.buffer != nullptr) {
            sounds[name] = snd;
            GAME_DEBUG("ASSET", "Sound loaded: '%s' from %s", name.c_str(), path.c_str());
        }
        else {
            GAME_ERROR("ASSET", "Failed to load sound '%s' from %s", name.c_str(), path.c_str());
        }
    }
}

Sound AssetManager::GetSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) return it->second;
    return Sound{ 0 };
}

void AssetManager::LoadMusic(const std::string& name, const std::string& path) {
    if (musics.find(name) == musics.end()) {
        Music mus = ::LoadMusicStream(path.c_str());
        if (mus.stream.buffer != nullptr) {
            musics[name] = mus;
            GAME_DEBUG("ASSET", "Music stream loaded: '%s' from %s", name.c_str(), path.c_str());
        }
        else {
            GAME_ERROR("ASSET", "Failed to load music stream '%s' from %s", name.c_str(), path.c_str());
        }
    }
}

Music AssetManager::GetMusic(const std::string& name) {
    auto it = musics.find(name);
    if (it != musics.end()) return it->second;
    return Music{ 0 };
}

void AssetManager::UnloadAll() {
    GAME_INFO("ASSET", "Unloading all cached textures, sounds, and music streams...");
    // 1. Очищення текстур
    for (std::map<std::string, Texture2D>::iterator it = textures.begin(); it != textures.end(); ++it) {
        ::UnloadTexture(it->second);
    }
    textures.clear();

    // 2. Очищення звуків
    for (std::map<std::string, Sound>::iterator it = sounds.begin(); it != sounds.end(); ++it) {
        ::UnloadSound(it->second);
    }
    sounds.clear();

    // 3. Очищення музичних потоків
    for (std::map<std::string, Music>::iterator it = musics.begin(); it != musics.end(); ++it) {
        ::UnloadMusicStream(it->second);
    }
    musics.clear();
}