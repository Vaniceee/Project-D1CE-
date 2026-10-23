#pragma once
#include "GameState.h"
#include "Player.h"
#include "raylib.h"

class GameplayState : public GameState {
public:
	void Init() override;
	void Cleanup() override;
	void Update() override;
	void Draw() override;

private:
	Player* player = nullptr;

	const int virtualWidth = 1280;
	const int virtualHeight = 720;

	RenderTexture2D targetTexture = { 0 };


};
