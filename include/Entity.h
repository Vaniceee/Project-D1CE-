#pragma once
#include "raylib.h"

class Entity {
protected:
	Vector2 position;
	Vector2 velocity;
	float speed;
	Rectangle collider;

public:
	Entity(Vector2 startPos, float startSpeed, float width, float height);
	virtual ~Entity() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;

	//Getter and setter
	Vector2 GetPosition() const;
	void SetPosition(Vector2 newPos);
	Rectangle GetCollider() const;
};