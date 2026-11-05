#pragma once
#include "Player.h"
#include "GameState.h"

class GameManager {
public:
   
    static GameManager& Instance();

    void Init();
    void Run();
    void Shutdown();

    void ChangeState(GameState* newState);
    void SetResolution(int width, int height);

    void Quit() { isRunning = false; }

    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }

private:
    
    GameManager();
    ~GameManager();

    int screenWidth = 1920;
    int screenHeight = 1080;

    GameState* currentState = nullptr;
    bool isRunning = true;
};