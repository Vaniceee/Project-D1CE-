#pragma once
#include "Entity.h"

class Player : public Entity {
public:
	Player(Vector2 startPos, float startSpeed, float width, float height);

		void Update() override;
        void Draw() override;

};