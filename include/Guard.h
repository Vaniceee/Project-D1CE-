#pragma once
#include "Entity.h"
#include "Player.h"
#include "AStar.h"
#include "raylib.h"
#include <vector>

class GameMap;

enum class GuardState {
    PATROL,
    INVESTIGATE,
    CHASE
};

class Guard : public Entity {
public:
    Guard(Vector2 startPos, float startSpeed = 120.0f, float width = 32.0f, float height = 64.0f);

    void InitPathfinding(const GameMap& gameMap);
    void Update() override {}
    void Update(Player& player, const GameMap& gameMap, float dt);
    void Draw() override;

    // Викликається з GameplayState, коли гравець заходить у телепорт
    void OnPlayerTeleported(Vector2 enteredTriggerCenter);

    GuardState GetCurrentState() const { return currentState; }
    Rectangle GetFootCollider() const;

private:
    GuardState currentState = GuardState::PATROL;
    float scale = 1.0f;

    // --- ЗІР, СЛУХ ТА АТАКА ---
    float viewDistance = 230.0f;        // Дальність конуса зору
    float fovPatrolAngleDeg = 80.0f;    // Кут огляду в спокійному стані
    float fovChaseAngleDeg = 150.0f;    // Широкий периферійний кут у погоні
    float proximityRadius = 38.0f;      // Мікро-зона дотику зі спини
    float hearingWalkRadius = 115.0f;   // Радіус слуху при звичайній ходьбі
    float hearingSprintRadius = 175.0f; // Радіус слуху при спринті
    float attackRadius = 42.0f;

    float facingAngle = 0.0f;           // Поточний кут погляду (в радіанах)
    float baseLookAngle = 0.0f;         // Базовий напрямок перед початком озирання

    // --- ПАМ'ЯТЬ ПОГОНІ ТА КД АТАКИ ---
    float chaseMemoryDuration = 1.5f;   // Скільки секунд НПС «пам'ятає» гравця за спиною
    float chaseMemoryTimer = 0.0f;

    float attackCooldownTimer = 0.0f;
    float attackCooldownDuration = 1.8f;
    float slowdownFactor = 0.25f;

    // --- НАВІГАЦІЯ A* ТА ТЕЛЕПОРТИ ---
    AStar pathfinder;
    std::vector<Vector2> currentPath;
    Vector2 spawnFootPos = { 0.0f, 0.0f };
    Vector2 lastKnownPlayerPos = { 0.0f, 0.0f };
    float pathUpdateTimer = 0.0f;
    float teleportCooldown = 0.0f;
    bool pursuingTeleport = false;
    bool returningViaTeleport = false;
    Vector2 returnTriggerPos = { 0.0f, 0.0f };

    // Затримка телепортації НПС
    bool isTeleporting = false;
    float teleportTimer = 0.0f;
    float teleportDuration = 1.2f;
    Vector2 pendingTeleportPos = { 0.0f, 0.0f };

    // --- ШАБЛОН ДЛЯ МАЙБУТНЬОГО ПАТРУЛЯ ТА ОЗИРАННЯ ---
    std::vector<Vector2> patrolWaypoints;
    size_t currentWaypointIndex = 0;
    bool isScanning = false;
    float scanTimer = 0.0f;
    float scanDuration = 3.0f;

    // --- ФІЗИКА КОЛІЗІЙ (NUDGE) ---
    int activeNudgeY = 0;
    int activeNudgeX = 0;

    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;

    void RecalculateFootCollider();
    Vector2 GetFootCenter() const;
    float GetActiveFovAngle() const;

    // Зір, слух та промені
    bool CanDetectPlayer(const Player& player, const GameMap& gameMap) const;
    bool HasClearLineOfSight(Vector2 start, Vector2 end, const GameMap& gameMap) const;
    bool CanMoveDirectlyTo(Vector2 start, Vector2 end, const GameMap& gameMap) const;
    void RequestPathTo(Vector2 target, const GameMap& gameMap);
    void UpdateScanningLook(float dt);

    // Рух
    void MoveWithCollisions(Vector2 moveDir, float moveStep, const GameMap& gameMap);
    void FollowPath(float moveSpeed, const GameMap& gameMap, float dt);

    // Поведінка станів (FSM)
    void UpdatePatrolState(const GameMap& gameMap, float dt);
    void UpdateInvestigateState(const GameMap& gameMap, float dt);
    void UpdateChaseState(Player& player, const GameMap& gameMap, float dt);
};