#include "Guard.h"
#include "Map.h"
#include "AssetManager.h"
#include "Logger.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

Guard::Guard(Vector2 startPos, float startSpeed, float width, float height)
    : Entity(startPos, startSpeed, width, height) {
    RecalculateFootCollider();
    spawnFootPos = GetFootCenter();
}

void Guard::InitPathfinding(const GameMap& gameMap) {
    pathfinder.BuildGrid(gameMap);
    spawnFootPos = GetFootCenter();

    chaseMusic = AssetManager::Instance().GetMusic("guard_chase");
    hasChaseMusic = (chaseMusic.stream.buffer != nullptr);
    if (hasChaseMusic) {
        chaseMusic.looping = true;
        SetMusicVolume(chaseMusic, 0.0f);
    }
}

void Guard::SetAudioPaused(bool paused) {
    if (!hasChaseMusic) return;
    if (paused) {
        PauseMusicStream(chaseMusic);
    }
    else if (currentChaseVolume > 0.005f) {
        ResumeMusicStream(chaseMusic);
    }
}

void Guard::StopAudio() {
    if (!hasChaseMusic) return;
    StopMusicStream(chaseMusic);
    currentChaseVolume = 0.0f;
    isChaseAudioEngaged = false;
    chaseIntroTimer = 0.0f;
    chaseAudioSustainTimer = 0.0f;
}

void Guard::UpdateChaseAudio(float distToPlayer, float dt) {
    if (!hasChaseMusic) return;

    bool shouldPlayAudio = false;

    if (currentState == GuardState::CHASE && !isTeleporting) {
        chaseAudioSustainTimer = chaseAudioSustainDuration;
        shouldPlayAudio = true;
    }
    else if (currentState == GuardState::INVESTIGATE && !isTeleporting && chaseAudioSustainTimer > 0.0f) {
        chaseAudioSustainTimer -= dt;
        if (chaseAudioSustainTimer > 0.0f) {
            shouldPlayAudio = true;
        }
    }
    else if (currentState == GuardState::PATROL) {
        chaseAudioSustainTimer = 0.0f;
    }

    float targetVolume = 0.0f;
    if (shouldPlayAudio) {
        if (!isChaseAudioEngaged) {
            isChaseAudioEngaged = true;
            chaseIntroTimer = 0.0f;
        }

        float range = maxChaseAudioDistance - attackRadius;
        float normalizedDist = (range > 1.0f) ? ((distToPlayer - attackRadius) / range) : 0.0f;
        normalizedDist = Clamp(normalizedDist, 0.0f, 1.0f);

        float proximity = 1.0f - normalizedDist;
        targetVolume = Lerp(0.15f, 0.85f, proximity);

        if (currentState == GuardState::INVESTIGATE) {
            targetVolume *= 0.75f;
        }
    }

    float fadeSpeed = shouldPlayAudio ? chaseFadeInSpeed : chaseFadeOutSpeed;
    float blendAlpha = 1.0f - expf(-fadeSpeed * dt);
    currentChaseVolume = Lerp(currentChaseVolume, targetVolume, blendAlpha);

    if (currentChaseVolume > 0.004f || shouldPlayAudio) {
        if (!IsMusicStreamPlaying(chaseMusic)) {
            PlayMusicStream(chaseMusic);
        }
        UpdateMusicStream(chaseMusic);

        chaseIntroTimer += dt;
        float introT = Clamp(chaseIntroTimer / chaseIntroDuration, 0.0f, 1.0f);
        float introEnvelope = introT * introT * (3.0f - 2.0f * introT);

        float timePlayed = GetMusicTimePlayed(chaseMusic);
        float timeLength = GetMusicTimeLength(chaseMusic);
        float seamEnvelope = 1.0f;
        const float microSeamFade = 0.15f;

        if (timeLength > 1.0f) {
            if (timePlayed < microSeamFade) {
                seamEnvelope = timePlayed / microSeamFade;
            }
            else if (timePlayed > timeLength - microSeamFade) {
                seamEnvelope = (timeLength - timePlayed) / microSeamFade;
            }
            seamEnvelope = Clamp(seamEnvelope, 0.0f, 1.0f);
        }

        SetMusicVolume(chaseMusic, currentChaseVolume * introEnvelope * seamEnvelope);
    }
    else {
        currentChaseVolume = 0.0f;
        isChaseAudioEngaged = false;
        chaseIntroTimer = 0.0f;
        if (IsMusicStreamPlaying(chaseMusic)) {
            StopMusicStream(chaseMusic);
        }
    }
}

void Guard::RecalculateFootCollider() {
    float scaledWidth = collider.width * scale;
    float scaledHeight = collider.height * scale;

    footBoxWidth = scaledWidth * 0.6f;
    footBoxHeight = scaledHeight * 0.25f;

    footOffsetX = (scaledWidth - footBoxWidth) * 0.5f;
    footOffsetY = scaledHeight - footBoxHeight;
}

Rectangle Guard::GetFootCollider() const {
    return { position.x + footOffsetX, position.y + footOffsetY, footBoxWidth, footBoxHeight };
}

Vector2 Guard::GetFootCenter() const {
    Rectangle foot = GetFootCollider();
    return { foot.x + foot.width / 2.0f, foot.y + foot.height / 2.0f };
}

float Guard::GetActiveFovAngle() const {
    return (currentState == GuardState::CHASE) ? fovChaseAngleDeg : fovPatrolAngleDeg;
}

void Guard::OnPlayerTeleported(Vector2 enteredTriggerCenter) {
    if (currentState == GuardState::CHASE || currentState == GuardState::INVESTIGATE) {
        currentState = GuardState::INVESTIGATE;
        pursuingTeleport = true;
        returningViaTeleport = false;
        chaseMemoryTimer = 0.0f;
        lastKnownPlayerPos = enteredTriggerCenter;
        isScanning = false;
        scanTimer = 0.0f;
        currentPath.clear();
        pathUpdateTimer = 0.0f;
        GAME_INFO("GUARD", "Player teleported! Guard pursuing into trigger at (%.1f, %.1f)",
            enteredTriggerCenter.x, enteredTriggerCenter.y);
    }
}

bool Guard::HasClearLineOfSight(Vector2 start, Vector2 end, const GameMap& gameMap) const {
    Vector2 diff = Vector2Subtract(end, start);
    float dist = Vector2Length(diff);
    if (dist <= 1.0f) return true;

    Vector2 dir = Vector2Scale(diff, 1.0f / dist);
    const float stepSize = 12.0f;

    for (float d = stepSize; d < dist; d += stepSize) {
        Vector2 p = Vector2Add(start, Vector2Scale(dir, d));
        Rectangle rayBox = { p.x - 2.0f, p.y - 2.0f, 4.0f, 4.0f };
        if (gameMap.CheckWallCollision(rayBox)) {
            return false;
        }
    }
    return true;
}

bool Guard::CanMoveDirectlyTo(Vector2 start, Vector2 end, const GameMap& gameMap) const {
    Vector2 diff = Vector2Subtract(end, start);
    float dist = Vector2Length(diff);
    if (dist <= 4.0f) return true;

    Vector2 dir = Vector2Scale(diff, 1.0f / dist);
    const float stepSize = 10.0f;

    for (float d = stepSize; d <= dist; d += stepSize) {
        Vector2 p = Vector2Add(start, Vector2Scale(dir, d));
        Rectangle footCheck = {
            p.x - footBoxWidth * 0.5f,
            p.y - footBoxHeight * 0.5f,
            footBoxWidth,
            footBoxHeight
        };
        if (gameMap.CheckWallCollision(footCheck)) {
            return false;
        }
    }
    return true;
}

void Guard::RequestPathTo(Vector2 target, const GameMap& gameMap) {
    Vector2 guardCenter = GetFootCenter();
    currentPath = pathfinder.FindPath(guardCenter, target);

    if (currentPath.size() > 1) {
        Vector2 toFirst = Vector2Subtract(currentPath[0], guardCenter);
        Vector2 toSecond = Vector2Subtract(currentPath[1], currentPath[0]);
        if (Vector2Length(toFirst) < 18.0f || Vector2DotProduct(toFirst, toSecond) < 0.0f) {
            currentPath.erase(currentPath.begin());
        }
    }
}

bool Guard::CanDetectPlayer(const Player& player, const GameMap& gameMap) const {
    Vector2 guardCenter = GetFootCenter();
    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerCenter = { playerFoot.x + playerFoot.width * 0.5f, playerFoot.y + playerFoot.height * 0.5f };

    float dist = Vector2Distance(guardCenter, playerCenter);
    float maxRange = fmaxf(viewDistance, hearingSprintRadius);
    if (dist > maxRange) return false;

    if (!HasClearLineOfSight(guardCenter, playerCenter, gameMap)) {
        return false;
    }

    if (dist <= proximityRadius) {
        return true;
    }

    if (player.IsSprinting() && dist <= hearingSprintRadius) {
        return true;
    }
    if (player.IsMoving() && dist <= hearingWalkRadius) {
        return true;
    }

    if (dist <= viewDistance) {
        Vector2 forward = { cosf(facingAngle), sinf(facingAngle) };
        Vector2 toPlayer = Vector2Normalize(Vector2Subtract(playerCenter, guardCenter));

        float dot = Vector2DotProduct(forward, toPlayer);
        float halfFovRad = (GetActiveFovAngle() * 0.5f) * DEG2RAD;
        if (dot >= cosf(halfFovRad)) {
            return true;
        }
    }

    return false;
}

void Guard::UpdateScanningLook(float dt) {
    scanTimer += dt;
    float sweepOffset = sinf((scanTimer / scanDuration) * PI * 2.0f) * (50.0f * DEG2RAD);
    facingAngle = baseLookAngle + sweepOffset;
}

void Guard::MoveWithCollisions(Vector2 moveDir, float moveStep, const GameMap& gameMap) {
    const float nudgeTolerance = 13.0f;

    if (moveDir.x != 0.0f) {
        position.x += moveDir.x * moveStep;
        if (gameMap.CheckWallCollision(GetFootCollider())) {
            position.x -= moveDir.x * moveStep;
            if (activeNudgeY == 0) {
                Rectangle testUp = GetFootCollider();
                testUp.y -= nudgeTolerance;
                testUp.x += moveDir.x * moveStep;

                Rectangle testDown = GetFootCollider();
                testDown.y += nudgeTolerance;
                testDown.x += moveDir.x * moveStep;

                bool canPassUp = !gameMap.CheckWallCollision(testUp);
                bool canPassDown = !gameMap.CheckWallCollision(testDown);

                if (canPassUp && !canPassDown) activeNudgeY = -1;
                else if (canPassDown && !canPassUp) activeNudgeY = 1;
            }

            if (activeNudgeY == -1) { position.y -= moveStep; moveDir.y = 0.0f; }
            else if (activeNudgeY == 1) { position.y += moveStep; moveDir.y = 0.0f; }
        }
        else {
            activeNudgeY = 0;
        }
    }
    else {
        activeNudgeY = 0;
    }

    if (moveDir.y != 0.0f) {
        position.y += moveDir.y * moveStep;
        if (gameMap.CheckWallCollision(GetFootCollider())) {
            position.y -= moveDir.y * moveStep;
            if (activeNudgeX == 0) {
                Rectangle testLeft = GetFootCollider();
                testLeft.x -= nudgeTolerance;
                testLeft.y += moveDir.y * moveStep;

                Rectangle testRight = GetFootCollider();
                testRight.x += nudgeTolerance;
                testRight.y += moveDir.y * moveStep;

                bool canPassLeft = !gameMap.CheckWallCollision(testLeft);
                bool canPassRight = !gameMap.CheckWallCollision(testRight);

                if (canPassLeft && !canPassRight) activeNudgeX = -1;
                else if (canPassRight && !canPassLeft) activeNudgeX = 1;
            }

            if (activeNudgeX == -1) { position.x -= moveStep; moveDir.x = 0.0f; }
            else if (activeNudgeX == 1) { position.x += moveStep; moveDir.x = 0.0f; }
        }
        else {
            activeNudgeX = 0;
        }
    }
    else {
        activeNudgeX = 0;
    }
}

void Guard::FollowPath(float moveSpeed, const GameMap& gameMap, float dt) {
    if (currentPath.empty()) return;

    Vector2 guardCenter = GetFootCenter();
    Vector2 targetWaypoint = currentPath.front();

    if (Vector2Distance(guardCenter, targetWaypoint) < 8.0f) {
        currentPath.erase(currentPath.begin());
        if (currentPath.empty()) return;
        targetWaypoint = currentPath.front();
    }

    Vector2 dir = Vector2Subtract(targetWaypoint, guardCenter);
    if (Vector2Length(dir) > 0.0f) {
        dir = Vector2Normalize(dir);
        facingAngle = atan2f(dir.y, dir.x);
        MoveWithCollisions(dir, moveSpeed * dt, gameMap);
    }
}

void Guard::Update(Player& player, const GameMap& gameMap, float dt) {
    if (isTeleporting) {
        teleportTimer -= dt;
        UpdateChaseAudio(9999.0f, dt);
        if (teleportTimer <= 0.0f) {
            isTeleporting = false;
            SetPosition(pendingTeleportPos);
            teleportCooldown = 1.5f;
            chaseMemoryTimer = 0.0f;
            pursuingTeleport = false;
            currentPath.clear();

            if (returningViaTeleport) {
                returningViaTeleport = false;
                currentState = GuardState::PATROL;
                isScanning = false;
            }
            else {
                currentState = GuardState::INVESTIGATE;
                isScanning = true;
                scanTimer = 0.0f;
                baseLookAngle = facingAngle;
            }
            GAME_INFO("GUARD", "Guard finished teleport to (%.1f, %.1f)", pendingTeleportPos.x, pendingTeleportPos.y);
        }
        collider.x = position.x + footOffsetX;
        collider.y = position.y + footOffsetY;
        return;
    }

    if (attackCooldownTimer > 0.0f) {
        attackCooldownTimer -= dt;
        if (attackCooldownTimer < 0.0f) attackCooldownTimer = 0.0f;
    }
    if (teleportCooldown > 0.0f) {
        teleportCooldown -= dt;
        if (teleportCooldown < 0.0f) teleportCooldown = 0.0f;
    }
    if (pathUpdateTimer > 0.0f) {
        pathUpdateTimer -= dt;
    }

    Vector2 guardFootCenter = GetFootCenter();
    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerFootCenter = { playerFoot.x + playerFoot.width / 2.0f, playerFoot.y + playerFoot.height / 2.0f };

    float distToPlayer = Vector2Distance(guardFootCenter, playerFootCenter);
    bool detectedNow = CanDetectPlayer(player, gameMap);

    if (pursuingTeleport && Vector2Distance(playerFootCenter, lastKnownPlayerPos) < 48.0f) {
        detectedNow = false;
    }

    if (detectedNow) {
        pursuingTeleport = false;
        returningViaTeleport = false;
        if (currentState != GuardState::CHASE) {
            currentState = GuardState::CHASE;
            currentPath.clear();
            pathUpdateTimer = 0.0f;
            isScanning = false;
            GAME_INFO("GUARD", "State -> CHASE");
        }
        chaseMemoryTimer = chaseMemoryDuration;
        lastKnownPlayerPos = playerFootCenter;
    }
    else if (currentState == GuardState::CHASE) {
        if (chaseMemoryTimer > 0.0f) {
            chaseMemoryTimer -= dt;
            lastKnownPlayerPos = playerFootCenter;
        }
        else {
            currentState = GuardState::INVESTIGATE;
            isScanning = false;
            scanTimer = 0.0f;
            RequestPathTo(lastKnownPlayerPos, gameMap);
            GAME_INFO("GUARD", "Lost sight! State -> INVESTIGATE");
        }
    }

    switch (currentState) {
    case GuardState::PATROL:
        UpdatePatrolState(gameMap, dt);
        break;
    case GuardState::INVESTIGATE:
        UpdateInvestigateState(gameMap, dt);
        break;
    case GuardState::CHASE:
        UpdateChaseState(player, gameMap, dt);
        break;
    }

    Vector2 newGuardPos;
    if (teleportCooldown <= 0.0f && gameMap.TryGetTeleportDestination(GetFootCollider(), position, newGuardPos)) {
        isTeleporting = true;
        teleportTimer = teleportDuration;
        pendingTeleportPos = newGuardPos;
        currentPath.clear();
        UpdateChaseAudio(9999.0f, dt);
        GAME_INFO("GUARD", "Guard entering teleport (delay: %.2fs)...", teleportDuration);
        return;
    }

    if (distToPlayer <= attackRadius && attackCooldownTimer <= 0.0f) {
        if (!player.IsInvincible()) {
            player.TakeDamage(1);
            attackCooldownTimer = attackCooldownDuration;
        }
    }

    UpdateChaseAudio(distToPlayer, dt);

    collider.x = position.x + footOffsetX;
    collider.y = position.y + footOffsetY;
}

void Guard::UpdatePatrolState(const GameMap& gameMap, float dt) {
    Vector2 guardCenter = GetFootCenter();

    if (returningViaTeleport) {
        float distToDoor = Vector2Distance(guardCenter, returnTriggerPos);
        if (distToDoor > 3.0f && CanMoveDirectlyTo(guardCenter, returnTriggerPos, gameMap)) {
            currentPath.clear();
            Vector2 dir = Vector2Normalize(Vector2Subtract(returnTriggerPos, guardCenter));
            facingAngle = atan2f(dir.y, dir.x);
            MoveWithCollisions(dir, speed * 0.75f * dt, gameMap);
            return;
        }

        if (!currentPath.empty()) {
            FollowPath(speed * 0.75f, gameMap, dt);
        }
        else if (distToDoor > 3.0f) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(returnTriggerPos, guardCenter));
            facingAngle = atan2f(dir.y, dir.x);
            MoveWithCollisions(dir, speed * 0.75f * dt, gameMap);
        }
        return;
    }

    Vector2 targetPoint = spawnFootPos;

    if (Vector2Distance(guardCenter, targetPoint) > 12.0f) {
        isScanning = false;
        if (CanMoveDirectlyTo(guardCenter, targetPoint, gameMap)) {
            currentPath.clear();
            Vector2 dir = Vector2Normalize(Vector2Subtract(targetPoint, guardCenter));
            facingAngle = atan2f(dir.y, dir.x);
            MoveWithCollisions(dir, speed * 0.75f * dt, gameMap);
            return;
        }

        if (currentPath.empty() && pathUpdateTimer <= 0.0f) {
            RequestPathTo(targetPoint, gameMap);

            if (currentPath.empty()) {
                float bestDist = 1e9f;
                bool foundExitTrigger = false;

                for (const Vector2& trigCenter : gameMap.GetAllTriggerCenters()) {
                    auto pathToTrig = pathfinder.FindPath(guardCenter, trigCenter);
                    if (!pathToTrig.empty() || CanMoveDirectlyTo(guardCenter, trigCenter, gameMap)) {
                        float d = Vector2Distance(guardCenter, trigCenter);
                        if (d < bestDist) {
                            bestDist = d;
                            returnTriggerPos = trigCenter;
                            currentPath = std::move(pathToTrig);
                            foundExitTrigger = true;
                        }
                    }
                }

                if (foundExitTrigger) {
                    returningViaTeleport = true;
                    GAME_INFO("GUARD", "Spawn unreachable directly. Returning via trigger at (%.1f, %.1f)",
                        returnTriggerPos.x, returnTriggerPos.y);
                }
                else {
                    pathUpdateTimer = 5.0f;
                }
                return;
            }

            pathUpdateTimer = 0.5f;
        }
        FollowPath(speed * 0.75f, gameMap, dt);
    }
    else {
        if (!isScanning) {
            isScanning = true;
            scanTimer = 0.0f;
            baseLookAngle = facingAngle;
        }
        UpdateScanningLook(dt);
    }
}

void Guard::UpdateInvestigateState(const GameMap& gameMap, float dt) {
    float currentMoveSpeed = (attackCooldownTimer > 0.0f) ? (speed * slowdownFactor) : (speed * 0.9f);
    float stopThreshold = pursuingTeleport ? 3.0f : 12.0f;
    float distToTarget = Vector2Distance(GetFootCenter(), lastKnownPlayerPos);

    if (distToTarget > stopThreshold &&
        CanMoveDirectlyTo(GetFootCenter(), lastKnownPlayerPos, gameMap)) {
        currentPath.clear();
        Vector2 dir = Vector2Normalize(Vector2Subtract(lastKnownPlayerPos, GetFootCenter()));
        facingAngle = atan2f(dir.y, dir.x);
        MoveWithCollisions(dir, currentMoveSpeed * dt, gameMap);
        return;
    }

    if (distToTarget > stopThreshold && currentPath.empty() && pathUpdateTimer <= 0.0f) {
        RequestPathTo(lastKnownPlayerPos, gameMap);
        pathUpdateTimer = 0.25f;
    }

    if (!currentPath.empty()) {
        FollowPath(currentMoveSpeed, gameMap, dt);
    }
    else if (pursuingTeleport && distToTarget > stopThreshold) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(lastKnownPlayerPos, GetFootCenter()));
        facingAngle = atan2f(dir.y, dir.x);
        MoveWithCollisions(dir, currentMoveSpeed * dt, gameMap);
    }
    else {
        pursuingTeleport = false;
        if (!isScanning) {
            isScanning = true;
            scanTimer = 0.0f;
            baseLookAngle = facingAngle;
        }

        UpdateScanningLook(dt);

        if (scanTimer >= scanDuration) {
            isScanning = false;
            currentState = GuardState::PATROL;
            currentPath.clear();
            GAME_INFO("GUARD", "Nothing found. State -> PATROL (returning to spawn)");
        }
    }
}

void Guard::UpdateChaseState(Player& player, const GameMap& gameMap, float dt) {
    Vector2 guardCenter = GetFootCenter();
    float currentMoveSpeed = (attackCooldownTimer > 0.0f) ? (speed * slowdownFactor) : speed;

    if (CanMoveDirectlyTo(guardCenter, lastKnownPlayerPos, gameMap)) {
        currentPath.clear();
        Vector2 diff = Vector2Subtract(lastKnownPlayerPos, guardCenter);
        if (Vector2Length(diff) > 2.0f) {
            Vector2 dir = Vector2Normalize(diff);
            facingAngle = atan2f(dir.y, dir.x);
            MoveWithCollisions(dir, currentMoveSpeed * dt, gameMap);
        }
        return;
    }

    if (pathUpdateTimer <= 0.0f || currentPath.empty()) {
        RequestPathTo(lastKnownPlayerPos, gameMap);
        pathUpdateTimer = 0.25f;
    }

    FollowPath(currentMoveSpeed, gameMap, dt);
}

void Guard::Draw() {
    Color guardColor = RED;
    if (attackCooldownTimer > 0.0f) guardColor = MAROON;
    else if (currentState == GuardState::INVESTIGATE) guardColor = ORANGE;
    else if (currentState == GuardState::PATROL) guardColor = BLUE;

    if (isTeleporting) {
        guardColor = ColorAlpha(guardColor, 0.35f);
    }

    DrawRectangleRec({ position.x, position.y, collider.width * scale, collider.height * scale }, guardColor);
}

void Guard::DrawDebug() const {
    Vector2 center = GetFootCenter();

    // 1. Конус огляду (динамічно розширюється з 80° до 150° у погоні)
    float activeFov = GetActiveFovAngle();
    float startAngleDeg = (facingAngle * RAD2DEG) - (activeFov * 0.5f);
    float endAngleDeg = (facingAngle * RAD2DEG) + (activeFov * 0.5f);
    Color coneColor = (currentState == GuardState::CHASE) ? ColorAlpha(RED, 0.22f) : ColorAlpha(YELLOW, 0.18f);

    DrawCircleSector(center, viewDistance, startAngleDeg, endAngleDeg, 24, coneColor);
    DrawCircleSectorLines(center, viewDistance, startAngleDeg, endAngleDeg, 24, ColorAlpha(YELLOW, 0.45f));

    // 2. Зони слуху кроків (115 px ходьба, 175 px біг), дотику (38 px) та атаки (42 px)
    DrawCircleLinesV(center, hearingSprintRadius, ColorAlpha(VIOLET, 0.22f));
    DrawCircleLinesV(center, hearingWalkRadius, ColorAlpha(SKYBLUE, 0.25f));
    DrawCircleLinesV(center, proximityRadius, ColorAlpha(ORANGE, 0.35f));
    DrawCircleLinesV(center, attackRadius, ColorAlpha(RED, 0.6f));
    DrawRectangleLinesEx(GetFootCollider(), 1.0f, MAROON);

    // 3. Дебаг маршруту A*
    for (size_t i = 0; i < currentPath.size(); ++i) {
        DrawCircleV(currentPath[i], 3.0f, SKYBLUE);
        if (i > 0) DrawLineV(currentPath[i - 1], currentPath[i], ColorAlpha(SKYBLUE, 0.5f));
    }
}