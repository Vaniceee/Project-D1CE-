#pragma once
#include "raylib.h"
#include <string>
#include <map>

class AssetManager {
public:
    static AssetManager& Instance();                                            // Повертає єдиний екземпляр менеджера (Singleton)

    void LoadTexture(const std::string& name, const std::string& path);         // Завантаження текстури та збереження її під іменем 'name'
    Texture2D GetTexture(const std::string& name);                              // Отримання вже завантаженої текстури

    // Звуки (короткі FX)
    void LoadSound(const std::string& name, const std::string& path);
    Sound GetSound(const std::string& name);

    // Музика (потокове аудіо)
    void LoadMusic(const std::string& name, const std::string& path);
    Music GetMusic(const std::string& name);

    void UnloadAll();                                                           // Очищення всієї пам'яті (викликати при закритті гри)

private:
    AssetManager() {}                                                           // Приватний конструктор, щоб ніхто не створив інший менеджер
    std::map<std::string, Texture2D> textures;                                  // Словник для зберігання текстур
    std::map<std::string, Sound> sounds;
    std::map<std::string, Music> musics;
};

