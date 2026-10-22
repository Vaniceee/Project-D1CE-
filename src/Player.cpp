#include "ProjectD1CE_CMake.h"
#include "Player.h"
#include "Entity.h"


Player::Player(Vector2 startPos, float startSpeed, float width, float height) 
	: Entity(startPos, startSpeed, width, height) { }
	
void Player::Update() {

}

void Player::Draw() {
	DrawRectangleRec(collider, BLUE);
}