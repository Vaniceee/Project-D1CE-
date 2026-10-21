// ProjectD1CE_CMake.cpp : Defines the entry point for the application.
//

#include "ProjectD1CE_CMake.h"
#include "raylib.h"
using namespace std;

int main()
{
	

	const int screenWidth = 800;
	const int screenHeight = 600;
	InitWindow(screenWidth, screenHeight, "Test");
	ToggleBorderlessWindowed();
	SetTargetFPS(60);

	while (!WindowShouldClose()) {

		BeginDrawing();
		ClearBackground(WHITE);

		Vector2 mousePosition = GetMousePosition();
		DrawCircleV(mousePosition, 10, BLACK);
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
