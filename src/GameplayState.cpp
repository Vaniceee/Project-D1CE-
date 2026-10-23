#include"GameplayState.h"

void GameplayState::Init() {
	targetTexture = LoadRenderTexture(virtualWidth, virtualHeight);

	player = new Player({ 400.0f,300.0f }, 9.0f, 40.0f, 60.0f);
}
void GameplayState::Cleanup() {
	if (targetTexture.id != 0) {
		UnloadRenderTexture(targetTexture);
	}


	if (player) {
		delete player;
		player = nullptr;
	}
}

void GameplayState::Update() {
	if (player) {
		player->Update();
	}
}
void GameplayState::Draw() {

	BeginTextureMode(targetTexture);
	ClearBackground(MAROON);
	if (player) {
		player->Draw();
	}

	EndTextureMode();

	///-------------------SCALING LOGIC--------------///

    float windowWidth = (float)GetScreenWidth();
    float windowHeight = (float)GetScreenHeight();

   
    float scaleX = windowWidth / (float)virtualWidth;
    float scaleY = windowHeight / (float)virtualHeight;

 
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    float drawWidth = (float)virtualWidth * scale;
    float drawHeight = (float)virtualHeight * scale;

   
    float destX = (windowWidth - drawWidth) * 0.5f;
    float destY = (windowHeight - drawHeight) * 0.5f;

  
    ClearBackground(BLACK);

  
    Rectangle sourceRec = { 0.0f, 0.0f, (float)targetTexture.texture.width, (float)-targetTexture.texture.height };
    Rectangle destRec = { destX, destY, drawWidth, drawHeight };
    Vector2 origin = { 0.0f, 0.0f };

    
    DrawTexturePro(targetTexture.texture, sourceRec, destRec, origin, 0.0f, WHITE);
}
