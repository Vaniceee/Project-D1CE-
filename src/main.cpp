// ProjectD1CE_CMake.cpp : Defines the entry point for the application.
//

#include "ProjectD1CE_CMake.h"
#include "raylib.h"
using namespace std;

int main()
{
	

	const int screenWidth = 1280;
	const int screenHeight = 720;
	InitWindow(screenWidth, screenHeight, "Test");
	//ToggleBorderlessWindowed();
	SetTargetFPS(300);
	HideCursor();
	IsCursorOnScreen();

	while (!WindowShouldClose()) {

		BeginDrawing();
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
