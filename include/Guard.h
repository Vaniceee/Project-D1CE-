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

    void OnPlayerTeleported(Vector2 enteredTriggerCenter);
    void SetAudioPaused(bool paused);
    void StopAudio();

    GuardState GetCurrentState() const { return currentState; }
    Rectangle GetFootCollider() const;

private:
    GuardState currentState = GuardState::PATROL;
    float scale = 1.0f;

    // --- ЗІР, СЛУХ ТА АТАКА ---
    float viewDistance = 230.0f;
    float fovPatrolAngleDeg = 80.0f;
    float fovChaseAngleDeg = 150.0f;
    float proximityRadius = 38.0f;
    float hearingWalkRadius = 115.0f;
    float hearingSprintRadius = 175.0f;
    float attackRadius = 42.0f;

    float facingAngle = 0.0f;
    float baseLookAngle = 0.0f;

    // --- ПАМ'ЯТЬ ПОГОНІ ТА КД АТАКИ ---
    float chaseMemoryDuration = 1.5f;
    float chaseMemoryTimer = 0.0f;

    float attackCooldownTimer = 0.0f;
    float attackCooldownDuration = 1.8f;
    float slowdownFactor = 0.25f;

    // --- АУДІО ПОГОНІ ---
    Music chaseMusic = { 0 };
    bool hasChaseMusic = false;
    bool isChaseAudioEngaged = false;
    float chaseIntroTimer = 0.0f;
    float chaseIntroDuration = 1.6f;       // Плавне наростання гучності на старті погоні (сек)
    float chaseAudioSustainTimer = 0.0f;
    float chaseAudioSustainDuration = 3.0f; // Скільки секунд музика грає після переходу в жовтий стан
    float currentChaseVolume = 0.0f;
    float maxChaseAudioDistance = 360.0f;
    float chaseFadeInSpeed = 4.0f;
    float chaseFadeOutSpeed = 1.1f;        // Повільне згасання після завершення

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

    // --- ПАТРУЛЬ ТА ОЗИРАННЯ ---
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

    // Зір, слух, промені та аудіо
    bool CanDetectPlayer(const Player& player, const GameMap& gameMap) const;
    bool HasClearLineOfSight(Vector2 start, Vector2 end, const GameMap& gameMap) const;
    bool CanMoveDirectlyTo(Vector2 start, Vector2 end, const GameMap& gameMap) const;
    void RequestPathTo(Vector2 target, const GameMap& gameMap);
    void UpdateScanningLook(float dt);
    void UpdateChaseAudio(float distToPlayer, float dt);

    // Рух
    void MoveWithCollisions(Vector2 moveDir, float moveStep, const GameMap& gameMap);
    void FollowPath(float moveSpeed, const GameMap& gameMap, float dt);

    // Поведінка станів (FSM)
    void UpdatePatrolState(const GameMap& gameMap, float dt);
    void UpdateInvestigateState(const GameMap& gameMap, float dt);
    void UpdateChaseState(Player& player, const GameMap& gameMap, float dt);
};