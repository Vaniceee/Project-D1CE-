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
    ~Guard();

    void InitPathfinding(const GameMap& gameMap);
    void SetPatrolWaypoints(const std::vector<Vector2>& waypoints);
    void Update() override {}
    void Update(Player& player, const GameMap& gameMap, float dt);
    void Draw() override;
    void DrawDebug() const;

    void OnPlayerTeleported(Vector2 enteredTriggerCenter);
    void SetAudioPaused(bool paused);
    void StopAudio();

    GuardState GetCurrentState() const { return currentState; }
    Rectangle GetFootCollider() const;

private:
    GuardState currentState = GuardState::PATROL;
    float scale = 1.0f;

    // --- «≤–, —À”’ “¿ ¿“¿ ¿ ---
    float viewDistance = 230.0f;
    float fovPatrolAngleDeg = 80.0f;
    float fovChaseAngleDeg = 150.0f;
    float proximityRadius = 38.0f;
    float hearingWalkRadius = 115.0f;
    float hearingSprintRadius = 175.0f;
    float attackRadius = 42.0f;

    float facingAngle = 0.0f;
    float baseLookAngle = 0.0f;

    // --- œ¿Ã'ﬂ“‹ œŒ√ŒÕ≤ “¿  ƒ ¿“¿ » ---
    float chaseMemoryDuration = 1.5f;
    float chaseMemoryTimer = 0.0f;

    float attackCooldownTimer = 0.0f;
    float attackCooldownDuration = 1.8f;
    float slowdownFactor = 0.25f;

    // --- ¿”ƒ≤Œ œŒ√ŒÕ≤, ƒ»’¿ÕÕﬂ “¿ ¬¿∆ »’  –Œ ≤¬ ---
    Music chaseMusic = { 0 };
    bool hasChaseMusic = false;
    bool isChaseAudioEngaged = false;
    float chaseIntroTimer = 0.0f;
    float chaseIntroDuration = 1.6f;
    float chaseAudioSustainTimer = 0.0f;
    float chaseAudioSustainDuration = 3.0f;
    float currentChaseVolume = 0.0f;
    float maxChaseAudioDistance = 360.0f;
    float chaseFadeInSpeed = 4.0f;
    float chaseFadeOutSpeed = 1.1f;

    Sound breathSoundLeft = { 0 };
    Sound breathSoundRight = { 0 };
    bool hasBreathSound = false;
    float maxBreathHearDistance = 390.0f;
    float currentBreathVolume = 0.0f;

    Sound stepSoundsLeft[4] = { { 0 } };
    Sound stepSoundsRight[4] = { { 0 } };
    int stepSoundCount = 0;
    float footstepTimer = 0.0f;
    int lastStepIndex = -1;
    float maxFootstepHearDistance = 480.0f;

    // --- ¬≤«”¿À‹Õ¿ “≈À≈√–¿‘≤ﬂ (Œ◊≤ “¿ ÿÀ≈…‘ ” œŒ√ŒÕ≤) ---
    struct EyeTrailPoint {
        Vector2 leftEye;
        Vector2 rightEye;
        float alpha;
    };
    std::vector<EyeTrailPoint> eyeTrail;
    float eyeTrailSpawnTimer = 0.0f;
    float visualPulseTime = 0.0f;

    // --- Õ¿¬≤√¿÷≤ﬂ A* “¿ “≈À≈œŒ–“» ---
    AStar pathfinder;
    std::vector<Vector2> currentPath;
    Vector2 spawnFootPos = { 0.0f, 0.0f };
    Vector2 lastKnownPlayerPos = { 0.0f, 0.0f };
    float pathUpdateTimer = 0.0f;
    float teleportCooldown = 0.0f;
    bool pursuingTeleport = false;
    bool returningViaTeleport = false;
    bool patrolTeleporting = false;
    Vector2 returnTriggerPos = { 0.0f, 0.0f };

    // «‡ÚËÏÍ‡ ÚÂÎÂÔÓÚ‡ˆ≥ø Õœ—
    bool isTeleporting = false;
    float teleportTimer = 0.0f;
    float teleportDuration = 1.2f;
    Vector2 pendingTeleportPos = { 0.0f, 0.0f };

    // --- œ¿“–”À‹ “¿ Œ«»–¿ÕÕﬂ ---
    std::vector<Vector2> patrolWaypoints;
    size_t currentWaypointIndex = 0;
    bool isScanning = false;
    float scanTimer = 0.0f;
    float scanDuration = 3.0f;
    float waypointStepDuration = 1.0f;

    // --- ‘≤«» ¿  ŒÀ≤«≤… (NUDGE) ---
    int activeNudgeY = 0;
    int activeNudgeX = 0;

    float footBoxWidth;
    float footBoxHeight;
    float footOffsetX;
    float footOffsetY;

    void RecalculateFootCollider();
    Vector2 GetFootCenter() const;
    float GetActiveFovAngle() const;

    // «≥, ÒÎÛı, ÔÓÏÂÌ≥ Ú‡ ‡Û‰≥Ó
    bool CanDetectPlayer(const Player& player, const GameMap& gameMap) const;
    bool HasClearLineOfSight(Vector2 start, Vector2 end, const GameMap& gameMap) const;
    bool CanMoveDirectlyTo(Vector2 start, Vector2 end, const GameMap& gameMap) const;
    void RequestPathTo(Vector2 target, const GameMap& gameMap);
    bool FindBestTeleportToward(Vector2 fromPos, Vector2 goalPos, const GameMap& gameMap, Vector2& outTriggerPos, std::vector<Vector2>& outPathToTrigger);
    void UpdateScanningLook(float dt);
    void UpdateFourWayPatrolLook(float dt);
    void UpdateChaseAudio(float distToPlayer, float dt);
    void UpdateBreathAudio(Vector2 guardCenter, Vector2 playerCenter, const GameMap& gameMap, float dt);
    void UpdateFootstepAudio(Vector2 guardCenter, Vector2 playerCenter, bool movedThisFrame, const GameMap& gameMap, float dt);

    // –Ûı
    void MoveWithCollisions(Vector2 moveDir, float moveStep, const GameMap& gameMap);
    void FollowPath(float moveSpeed, const GameMap& gameMap, float dt);

    // œÓ‚Â‰≥ÌÍ‡ ÒÚ‡Ì≥‚ (FSM)
    void UpdatePatrolState(const GameMap& gameMap, float dt);
    void UpdateInvestigateState(const GameMap& gameMap, float dt);
    void UpdateChaseState(Player& player, const GameMap& gameMap, float dt);
};