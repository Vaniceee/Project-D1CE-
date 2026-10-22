#pragma once
#include "Player.h"

class GameManager {
public:
	GameManager();
	~GameManager();

	void Init();
	void Run();
	void Shutdown();

	void ChangeState(GameState* newState);

private:
	const int 
};