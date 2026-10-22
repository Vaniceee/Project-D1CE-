// ProjectD1CE_CMake.cpp : Defines the entry point for the application.
//

#include "ProjectD1CE_CMake.h"
#include "raylib.h"
#include "Player.h"
using namespace std;

int main()
{
	Player player({ 500,500 }, 4.0f, 40.0f, 40.0f);

	const int screenWidth = 1280;
	const int screenHeight = 720;
	InitWindow(screenWidth, screenHeight, "Test");
	//ToggleBorderlessWindowed();
	SetTargetFPS(300);
	HideCursor();
	IsCursorOnScreen();

	while (!WindowShouldClose()) {
		player.Update();

		BeginDrawing();
		player.Draw();
		DrawText("Hello", 300, 300, 13, BLACK);
		

		if (IsCursorOnScreen()) {
			ClearBackground(WHITE);

		}
		else ClearBackground(BLACK);
			
			

		



		Vector2 mousePosition = GetMousePosition();
		DrawCircleV(mousePosition, 30, BLACK);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
