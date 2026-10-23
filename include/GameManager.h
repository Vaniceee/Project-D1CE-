#pragma once
#include "Player.h"
#include "GameState.h"

class GameManager {
public:
	GameManager();
	~GameManager();

	void Init();
	void Run();
	void Shutdown();

	void ChangeState(GameState* newState);
	void SetResolution(int width, int height);

	int GetScreenWidth() const { return screenWidth; }
	int GetScreenHeight() const { return screenHeight; }


private:
	int screenWidth = 1920;
	int screenHeight = 1080;

	GameState* currentState = nullptr;


};