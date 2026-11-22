#include "Guard.h"
#include "Map.h"
#include "AssetManager.h"
#include "Config.h"
#include "Logger.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

namespace {
    float SmoothRotateAngle(float currentRad, float targetRad, float speed, float dt) {
        float diff = targetRad - currentRad;
        while (diff > PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        float alpha = 1.0f - expf(-speed * dt);
        return currentRad + diff * alpha;
    }
}

Guard::Guard(Vector2 startPos, float startSpeed, float width, float height)
    : Entity(startPos, startSpeed, width, height) {
    RecalculateFootCollider();
    spawnFootPos = GetFootCenter();
}

Guard::~Guard() {
    for (int i = 0; i < stepSoundCount; ++i) {
        if (stepSoundsLeft[i].frameCount > 0) UnloadSound(stepSoundsLeft[i]);
        if (stepSoundsRight[i].frameCount > 0) UnloadSound(stepSoundsRight[i]);
    }
    stepSoundCount = 0;

    if (hasBreathSound) {
        if (breathSoundLeft.frameCount > 0) UnloadSound(breathSoundLeft);
        if (breathSoundRight.frameCount > 0) UnloadSound(breathSoundRight);
        hasBreathSound = false;
    }
}

void Guard::InitPathfinding(const GameMap& gameMap) {
    pathfinder.BuildGrid(gameMap);
    spawnFootPos = GetFootCenter();

    AssetManager& am = AssetManager::Instance();
    chaseMusic = am.GetMusic("guard_chase");
    hasChaseMusic = (chaseMusic.stream.buffer != nullptr);
    if (hasChaseMusic) {
        chaseMusic.looping = true;
        SetMusicVolume(chaseMusic, 0.0f);
    }

    // 1. Ініціалізація розділених стерео-кроків
    for (int i = 0; i < stepSoundCount; ++i) {
        if (stepSoundsLeft[i].frameCount > 0) UnloadSound(stepSoundsLeft[i]);
        if (stepSoundsRight[i].frameCount > 0) UnloadSound(stepSoundsRight[i]);
    }
    stepSoundCount = 0;

    const char* stepPaths[4] = {
        Config::PATH_STEP_WOOD_1,
        Config::PATH_STEP_WOOD_2,
        Config::PATH_STEP_WOOD_3,
        Config::PATH_STEP_WOOD_4
    };

    for (int i = 0; i < 4; ++i) {
        Wave src = LoadWave(stepPaths[i]);
        if (src.frameCount == 0) continue;

        float* samples = LoadWaveSamples(src);
        unsigned int frames = src.frameCount;
        unsigned int channels = src.channels;

        Wave leftWave = { 0 };
        leftWave.frameCount = frames;
        leftWave.sampleRate = src.sampleRate;
        leftWave.sampleSize = 16;
        leftWave.channels = 2;
        leftWave.data = RL_CALLOC(frames * 2, sizeof(short));

        Wave rightWave = { 0 };
        rightWave.frameCount = frames;
        rightWave.sampleRate = src.sampleRate;
        rightWave.sampleSize = 16;
        rightWave.channels = 2;
        rightWave.data = RL_CALLOC(frames * 2, sizeof(short));

        short* leftData = (short*)leftWave.data;
        short* rightData = (short*)rightWave.data;

        for (unsigned int f = 0; f < frames; ++f) {
            float mono = 0.0f;
            for (unsigned int c = 0; c < channels; ++c) {
                mono += samples[f * channels + c];
            }
            mono /= (float)channels;
            short s = (short)(Clamp(mono, -1.0f, 1.0f) * 32767.0f);

            leftData[f * 2 + 0] = s;
            leftData[f * 2 + 1] = 0;

            rightData[f * 2 + 0] = 0;
            rightData[f * 2 + 1] = s;
        }

        stepSoundsLeft[stepSoundCount] = LoadSoundFromWave(leftWave);
        stepSoundsRight[stepSoundCount] = LoadSoundFromWave(rightWave);
        stepSoundCount++;

        UnloadWaveSamples(samples);
        UnloadWave(leftWave);
        UnloadWave(rightWave);
        UnloadWave(src);
    }

    // 2. Ініціалізація безшовного зацикленого дихання охоронця (Equal-Power Crossfade на стику)
    if (hasBreathSound) {
        if (breathSoundLeft.frameCount > 0) UnloadSound(breathSoundLeft);
        if (breathSoundRight.frameCount > 0) UnloadSound(breathSoundRight);
        hasBreathSound = false;
    }

    Wave breathSrc = LoadWave(Config::PATH_AUDIO_GUARD_BREATH);
    if (breathSrc.frameCount > 0) {
        float* rawSamples = LoadWaveSamples(breathSrc);
        unsigned int totalFrames = breathSrc.frameCount;
        unsigned int channels = breathSrc.channels;
        unsigned int sampleRate = breathSrc.sampleRate;

        // Зводимо у моно-буфер
        std::vector<float> monoSamples(totalFrames, 0.0f);
        for (unsigned int f = 0; f < totalFrames; ++f) {
            float sum = 0.0f;
            for (unsigned int c = 0; c < channels; ++c) {
                sum += rawSamples[f * channels + c];
            }
            monoSamples[f] = sum / (float)channels;
        }
        UnloadWaveSamples(rawSamples);

        // 1.5-секундний кросфейд: накладаємо хвіст на початок і обрізаємо довжину кросфейду,
        // щоб перехід з кінця на початок був абсолютно непомітним
        unsigned int fadeFrames = (unsigned int)(sampleRate * 1.5f);
        if (fadeFrames > totalFrames / 3) {
            fadeFrames = totalFrames / 3;
        }
        unsigned int loopFrames = totalFrames - fadeFrames;

        if (loopFrames > 0) {
            Wave leftBreathWave = { 0 };
            leftBreathWave.frameCount = loopFrames;
            leftBreathWave.sampleRate = sampleRate;
            leftBreathWave.sampleSize = 16;
            leftBreathWave.channels = 2;
            leftBreathWave.data = RL_CALLOC(loopFrames * 2, sizeof(short));

            Wave rightBreathWave = { 0 };
            rightBreathWave.frameCount = loopFrames;
            rightBreathWave.sampleRate = sampleRate;
            rightBreathWave.sampleSize = 16;
            rightBreathWave.channels = 2;
            rightBreathWave.data = RL_CALLOC(loopFrames * 2, sizeof(short));

            short* leftData = (short*)leftBreathWave.data;
            short* rightData = (short*)rightBreathWave.data;

            for (unsigned int f = 0; f < loopFrames; ++f) {
                float val = monoSamples[f];
                if (f < fadeFrames) {
                    float t = (float)f / (float)fadeFrames;
                    // Equal-power крива (sin/cos), щоб на стику не просідала гучність
                    float fadeIn = sinf(t * PI * 0.5f);
                    float fadeOut = cosf(t * PI * 0.5f);
                    float tailVal = monoSamples[loopFrames + f];
                    val = val * fadeIn + tailVal * fadeOut;
                }

                short s = (short)(Clamp(val, -1.0f, 1.0f) * 32767.0f);
                leftData[f * 2 + 0] = s;
                leftData[f * 2 + 1] = 0;

                rightData[f * 2 + 0] = 0;
                rightData[f * 2 + 1] = s;
            }

            breathSoundLeft = LoadSoundFromWave(leftBreathWave);
            breathSoundRight = LoadSoundFromWave(rightBreathWave);
            hasBreathSound = true;

            UnloadWave(leftBreathWave);
            UnloadWave(rightBreathWave);
        }

        UnloadWave(breathSrc);
    }
}

void Guard::SetPatrolWaypoints(const std::vector<Vector2>& waypoints) {
    patrolWaypoints = waypoints;
    currentWaypointIndex = 0;

    if (patrolWaypoints.size() > 1 && Vector2Distance(spawnFootPos, patrolWaypoints[0]) < 24.0f) {
        currentWaypointIndex = 1;
    }

    GAME_INFO("GUARD", "Loaded %zu patrol waypoints (starting at index %zu)",
        patrolWaypoints.size(), currentWaypointIndex);
}

bool Guard::FindBestTeleportToward(Vector2 fromPos, Vector2 goalPos, const GameMap& gameMap, Vector2& outTriggerPos, std::vector<Vector2>& outPathToTrigger) {
    float bestScore = 1e9f;
    bool found = false;

    for (const auto& link : gameMap.GetAllTeleportLinks()) {
        std::vector<Vector2> pathToEntry;
        bool canReachEntry = CanMoveDirectlyTo(fromPos, link.triggerCenter, gameMap);
        if (!canReachEntry) {
            pathToEntry = pathfinder.FindPath(fromPos, link.triggerCenter);
            canReachEntry = !pathToEntry.empty();
        }
        if (!canReachEntry) continue;

        bool exitReachesGoal = CanMoveDirectlyTo(link.destinationCenter, goalPos, gameMap);
        float exitToGoalDist = Vector2Distance(link.destinationCenter, goalPos);

        if (!exitReachesGoal) {
            auto pathFromExit = pathfinder.FindPath(link.destinationCenter, goalPos);
            if (!pathFromExit.empty()) {
                exitReachesGoal = true;
                exitToGoalDist = 0.0f;
                Vector2 prev = link.destinationCenter;
                for (const Vector2& wp : pathFromExit) {
                    exitToGoalDist += Vector2Distance(prev, wp);
                    prev = wp;
                }
            }
        }

        float entryDist = Vector2Distance(fromPos, link.triggerCenter);
        float score = exitReachesGoal ? (entryDist + exitToGoalDist) : (100000.0f + entryDist + exitToGoalDist);

        if (score < bestScore) {
            bestScore = score;
            outTriggerPos = link.triggerCenter;
            outPathToTrigger = std::move(pathToEntry);
            found = true;
        }
    }

    return found;
}

void Guard::SetAudioPaused(bool paused) {
    if (hasChaseMusic) {
        if (paused) {
            PauseMusicStream(chaseMusic);
        }
        else if (currentChaseVolume > 0.005f) {
            ResumeMusicStream(chaseMusic);
        }
    }

    if (hasBreathSound) {
        if (paused) {
            PauseSound(breathSoundLeft);
            PauseSound(breathSoundRight);
        }
        else if (currentBreathVolume > 0.005f) {
            ResumeSound(breathSoundLeft);
            ResumeSound(breathSoundRight);
        }
    }
}

void Guard::StopAudio() {
    if (hasChaseMusic) {
        StopMusicStream(chaseMusic);
    }
    if (hasBreathSound) {
        StopSound(breathSoundLeft);
        StopSound(breathSoundRight);
    }
    currentChaseVolume = 0.0f;
    currentBreathVolume = 0.0f;
    isChaseAudioEngaged = false;
    chaseIntroTimer = 0.0f;
    chaseAudioSustainTimer = 0.0f;
    footstepTimer = 0.0f;
}

void Guard::UpdateBreathAudio(Vector2 guardCenter, Vector2 playerCenter, const GameMap& gameMap, float dt) {
    if (!hasBreathSound) return;

    float dist = Vector2Distance(guardCenter, playerCenter);
    float targetVolume = 0.0f;

    if (!isTeleporting && dist < maxBreathHearDistance) {
        float normDist = Clamp(dist / maxBreathHearDistance, 0.0f, 1.0f);
        // Чим ближче НПС, тим виразніше чути важке дихання
        float distAttenuation = powf(1.0f - normDist, 1.35f);
        bool hasLOS = HasClearLineOfSight(guardCenter, playerCenter, gameMap);
        float wallFactor = hasLOS ? 1.0f : 0.58f;

        float stateMaxVol = 0.72f;
        if (currentState == GuardState::CHASE) {
            stateMaxVol = 0.95f;
        }
        else if (currentState == GuardState::INVESTIGATE) {
            stateMaxVol = 0.82f;
        }

        targetVolume = Clamp(stateMaxVol * distAttenuation * wallFactor, 0.0f, 1.0f);
    }

    float blendAlpha = 1.0f - expf(-6.5f * dt);
    currentBreathVolume = Lerp(currentBreathVolume, targetVolume, blendAlpha);

    if (currentBreathVolume > 0.004f) {
        // Тримаємо обидва канали синхронно зацикленими
        if (!IsSoundPlaying(breathSoundLeft)) {
            PlaySound(breathSoundLeft);
        }
        if (!IsSoundPlaying(breathSoundRight)) {
            PlaySound(breathSoundRight);
        }

        // Прискорюємо ритм дихання у погоні / пошуку
        float targetPitch = 1.0f;
        if (currentState == GuardState::CHASE) targetPitch = 1.18f;
        else if (currentState == GuardState::INVESTIGATE) targetPitch = 1.08f;

        float dx = guardCenter.x - playerCenter.x;
        float panOffset = Clamp(dx / 220.0f, -1.0f, 1.0f);

        float angle = (panOffset + 1.0f) * 0.25f * PI;
        float leftGain = cosf(angle);
        float rightGain = sinf(angle);

        SetSoundVolume(breathSoundLeft, currentBreathVolume * leftGain);
        SetSoundPitch(breathSoundLeft, targetPitch);

        SetSoundVolume(breathSoundRight, currentBreathVolume * rightGain);
        SetSoundPitch(breathSoundRight, targetPitch);
    }
    else {
        currentBreathVolume = 0.0f;
        if (IsSoundPlaying(breathSoundLeft)) StopSound(breathSoundLeft);
        if (IsSoundPlaying(breathSoundRight)) StopSound(breathSoundRight);
    }
}

void Guard::UpdateFootstepAudio(Vector2 guardCenter, Vector2 playerCenter, bool movedThisFrame, const GameMap& gameMap, float dt) {
    if (stepSoundCount == 0 || isTeleporting || !movedThisFrame) {
        if (!movedThisFrame && footstepTimer > 0.14f) {
            footstepTimer = 0.14f;
        }
        return;
    }

    footstepTimer -= dt;
    if (footstepTimer > 0.0f) return;

    float stepInterval = 0.64f;
    if (currentState == GuardState::CHASE) {
        stepInterval = 0.34f;
    }
    else if (currentState == GuardState::INVESTIGATE) {
        stepInterval = 0.48f;
    }
    footstepTimer = stepInterval;

    float dist = Vector2Distance(guardCenter, playerCenter);
    if (dist > maxFootstepHearDistance) return;

    int idx = GetRandomValue(0, stepSoundCount - 1);
    if (stepSoundCount > 1 && idx == lastStepIndex) {
        idx = (idx + 1) % stepSoundCount;
    }
    lastStepIndex = idx;

    float normDist = Clamp(dist / maxFootstepHearDistance, 0.0f, 1.0f);
    float distAttenuation = powf(1.0f - normDist, 1.45f);
    bool hasLOS = HasClearLineOfSight(guardCenter, playerCenter, gameMap);
    float wallFactor = hasLOS ? 1.0f : 0.62f;

    float baseVolume = (currentState == GuardState::CHASE) ? 0.92f : 0.78f;
    float finalVolume = Clamp(baseVolume * distAttenuation * wallFactor, 0.0f, 1.0f);
    if (finalVolume < 0.02f) return;

    float pitch = (currentState == GuardState::CHASE)
        ? ((float)GetRandomValue(72, 80) / 100.0f)
        : ((float)GetRandomValue(60, 68) / 100.0f);

    float dx = guardCenter.x - playerCenter.x;
    float panOffset = Clamp(dx / 220.0f, -1.0f, 1.0f);

    float angle = (panOffset + 1.0f) * 0.25f * PI;
    float leftGain = cosf(angle);
    float rightGain = sinf(angle);

    float leftVol = finalVolume * leftGain;
    float rightVol = finalVolume * rightGain;

    if (leftVol > 0.005f) {
        SetSoundVolume(stepSoundsLeft[idx], leftVol);
        SetSoundPitch(stepSoundsLeft[idx], pitch);
        PlaySound(stepSoundsLeft[idx]);
    }
    if (rightVol > 0.005f) {
        SetSoundVolume(stepSoundsRight[idx], rightVol);
        SetSoundPitch(stepSoundsRight[idx], pitch);
        PlaySound(stepSoundsRight[idx]);
    }
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
        targetVolume = Lerp(0.07f, 0.40f, proximity);

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
        patrolTeleporting = false;
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

void Guard::UpdateFourWayPatrolLook(float dt) {
    scanTimer += dt;

    int stepIndex = (int)(scanTimer / waypointStepDuration);
    if (stepIndex > 3) stepIndex = 3;

    float stepLocalTime = fmodf(scanTimer, waypointStepDuration);
    const float turnTransition = 0.45f;

    float prevOffsetDeg = (stepIndex > 0) ? ((float)(stepIndex - 1) * 90.0f) : 0.0f;
    float targetOffsetDeg = (float)stepIndex * 90.0f;

    float t = (stepIndex == 0) ? 1.0f : Clamp(stepLocalTime / turnTransition, 0.0f, 1.0f);
    float smoothT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

    float currentOffsetDeg = Lerp(prevOffsetDeg, targetOffsetDeg, smoothT);
    facingAngle = baseLookAngle + currentOffsetDeg * DEG2RAD;
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
        float targetAngle = atan2f(dir.y, dir.x);
        float turnSpeed = (currentState == GuardState::CHASE) ? 14.0f : 7.5f;
        facingAngle = SmoothRotateAngle(facingAngle, targetAngle, turnSpeed, dt);
        MoveWithCollisions(dir, moveSpeed * dt, gameMap);
    }
}

void Guard::Update(Player& player, const GameMap& gameMap, float dt) {
    visualPulseTime += dt;
    for (size_t i = 0; i < eyeTrail.size(); ) {
        eyeTrail[i].alpha -= dt * 3.4f;
        if (eyeTrail[i].alpha <= 0.0f) {
            eyeTrail.erase(eyeTrail.begin() + i);
        }
        else {
            ++i;
        }
    }

    Rectangle playerFoot = player.GetFootCollider();
    Vector2 playerFootCenter = { playerFoot.x + playerFoot.width / 2.0f, playerFoot.y + playerFoot.height / 2.0f };

    if (isTeleporting) {
        teleportTimer -= dt;
        UpdateBreathAudio(GetFootCenter(), playerFootCenter, gameMap, dt);
        UpdateChaseAudio(9999.0f, dt);
        if (teleportTimer <= 0.0f) {
            isTeleporting = false;
            SetPosition(pendingTeleportPos);
            teleportCooldown = 1.5f;
            chaseMemoryTimer = 0.0f;
            pursuingTeleport = false;
            currentPath.clear();
            eyeTrail.clear();

            if (returningViaTeleport) {
                returningViaTeleport = false;
                currentState = GuardState::PATROL;
                isScanning = false;
                if (patrolTeleporting) {
                    patrolTeleporting = false;
                    pathUpdateTimer = 0.0f;
                }
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

    Vector2 posBeforeMove = position;

    bool detectedNow = CanDetectPlayer(player, gameMap);

    if (pursuingTeleport && Vector2Distance(playerFootCenter, lastKnownPlayerPos) < 48.0f) {
        detectedNow = false;
    }

    if (detectedNow) {
        pursuingTeleport = false;
        returningViaTeleport = false;
        patrolTeleporting = false;
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

    bool movedThisFrame = (Vector2Distance(posBeforeMove, position) > 0.08f);
    Vector2 updatedGuardCenter = GetFootCenter();
    float updatedDistToPlayer = Vector2Distance(updatedGuardCenter, playerFootCenter);

    Vector2 newGuardPos;
    if (teleportCooldown <= 0.0f && gameMap.TryGetTeleportDestination(GetFootCollider(), position, newGuardPos)) {
        isTeleporting = true;
        teleportTimer = teleportDuration;
        pendingTeleportPos = newGuardPos;
        currentPath.clear();
        UpdateBreathAudio(updatedGuardCenter, playerFootCenter, gameMap, dt);
        UpdateChaseAudio(9999.0f, dt);
        GAME_INFO("GUARD", "Guard entering teleport (delay: %.2fs)...", teleportDuration);
        return;
    }

    if (updatedDistToPlayer <= attackRadius && attackCooldownTimer <= 0.0f) {
        if (!player.IsInvincible()) {
            player.TakeDamage(1);
            attackCooldownTimer = attackCooldownDuration;
        }
    }

    UpdateBreathAudio(updatedGuardCenter, playerFootCenter, gameMap, dt);
    UpdateFootstepAudio(updatedGuardCenter, playerFootCenter, movedThisFrame, gameMap, dt);
    UpdateChaseAudio(updatedDistToPlayer, dt);

    if (currentState == GuardState::CHASE && !isTeleporting) {
        eyeTrailSpawnTimer -= dt;
        if (eyeTrailSpawnTimer <= 0.0f) {
            eyeTrailSpawnTimer = 0.032f;

            float w = collider.width * scale;
            Vector2 headCenter = { position.x + w * 0.5f, position.y + 13.0f };
            Vector2 lookDir = { cosf(facingAngle), sinf(facingAngle) };
            Vector2 perpDir = { -lookDir.y, lookDir.x };

            Vector2 eyeBase = Vector2Add(headCenter, Vector2Scale(lookDir, 4.5f));
            EyeTrailPoint pt;
            pt.leftEye = Vector2Add(eyeBase, Vector2Scale(perpDir, -4.0f));
            pt.rightEye = Vector2Add(eyeBase, Vector2Scale(perpDir, 4.0f));
            pt.alpha = 0.72f;
            eyeTrail.push_back(pt);
        }
    }

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
            facingAngle = SmoothRotateAngle(facingAngle, atan2f(dir.y, dir.x), 7.5f, dt);
            MoveWithCollisions(dir, speed * 0.75f * dt, gameMap);
            return;
        }

        if (!currentPath.empty()) {
            FollowPath(speed * 0.75f, gameMap, dt);
        }
        else if (distToDoor > 3.0f) {
            Vector2 dir = Vector2Normalize(Vector2Subtract(returnTriggerPos, guardCenter));
            facingAngle = SmoothRotateAngle(facingAngle, atan2f(dir.y, dir.x), 7.5f, dt);
            MoveWithCollisions(dir, speed * 0.75f * dt, gameMap);
        }
        return;
    }

    bool hasWaypoints = !patrolWaypoints.empty();
    Vector2 targetPoint = hasWaypoints ? patrolWaypoints[currentWaypointIndex] : spawnFootPos;

    if (Vector2Distance(guardCenter, targetPoint) > 14.0f) {
        isScanning = false;
        if (CanMoveDirectlyTo(guardCenter, targetPoint, gameMap)) {
            currentPath.clear();
            Vector2 dir = Vector2Normalize(Vector2Subtract(targetPoint, guardCenter));
            facingAngle = SmoothRotateAngle(facingAngle, atan2f(dir.y, dir.x), 7.5f, dt);
            MoveWithCollisions(dir, speed * 0.75f * dt, gameMap);
            return;
        }

        if (currentPath.empty() && pathUpdateTimer <= 0.0f) {
            RequestPathTo(targetPoint, gameMap);

            if (currentPath.empty()) {
                Vector2 chosenTrigger = { 0.0f, 0.0f };
                std::vector<Vector2> pathToTrig;

                if (FindBestTeleportToward(guardCenter, targetPoint, gameMap, chosenTrigger, pathToTrig)) {
                    returnTriggerPos = chosenTrigger;
                    currentPath = std::move(pathToTrig);
                    returningViaTeleport = true;
                    patrolTeleporting = hasWaypoints;
                    GAME_INFO("GUARD", "Patrol target %zu in another zone. Moving via trigger at (%.1f, %.1f)",
                        currentWaypointIndex, returnTriggerPos.x, returnTriggerPos.y);
                }
                else {
                    pathUpdateTimer = 3.0f;
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

        if (hasWaypoints) {
            UpdateFourWayPatrolLook(dt);

            if (scanTimer >= waypointStepDuration * 4.0f) {
                isScanning = false;
                scanTimer = 0.0f;
                currentWaypointIndex = (currentWaypointIndex + 1) % patrolWaypoints.size();
                currentPath.clear();
                pathUpdateTimer = 0.0f;
            }
        }
        else {
            UpdateScanningLook(dt);
        }
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
        facingAngle = SmoothRotateAngle(facingAngle, atan2f(dir.y, dir.x), 10.0f, dt);
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
        facingAngle = SmoothRotateAngle(facingAngle, atan2f(dir.y, dir.x), 10.0f, dt);
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
            GAME_INFO("GUARD", "Nothing found. State -> PATROL");
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
            facingAngle = SmoothRotateAngle(facingAngle, atan2f(dir.y, dir.x), 14.0f, dt);
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
    float alphaMult = isTeleporting ? 0.35f : 1.0f;

    Color guardColor = RED;
    if (attackCooldownTimer > 0.0f) guardColor = MAROON;
    else if (currentState == GuardState::INVESTIGATE) guardColor = ORANGE;
    else if (currentState == GuardState::PATROL) guardColor = BLUE;

    float w = collider.width * scale;
    float h = collider.height * scale;

    // 1. Примарний шлейф червоних очей під час погоні
    for (const auto& pt : eyeTrail) {
        float a = pt.alpha * alphaMult;
        DrawCircleV(pt.leftEye, 3.2f, ColorAlpha(RED, a * 0.45f));
        DrawCircleV(pt.rightEye, 3.2f, ColorAlpha(RED, a * 0.45f));
    }

    // 2. Силует охоронця 
    DrawRectangleRec({ position.x, position.y, w, h }, ColorAlpha(guardColor, alphaMult));
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

    // 2. Зони слуху, чутності кроків (480 px), дихання (390 px), дотику (38 px) та атаки (42 px)
    DrawCircleLinesV(center, maxFootstepHearDistance, ColorAlpha(GREEN, 0.14f));
    DrawCircleLinesV(center, maxBreathHearDistance, ColorAlpha(LIME, 0.18f));
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