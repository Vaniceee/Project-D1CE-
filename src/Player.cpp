#include "Player.h"
#include "Map.h"
#include "Collisions.h"
#include "AssetManager.h"
#include "raylib.h"
#include "raymath.h"
#include <string>

Player::Player(Vector2 startPos, float startSpeed, float width, float height, Texture2D tex, float playerScale)
    : Entity(startPos, startSpeed, width, height), currentSpeed(startSpeed), texture(tex), scale(playerScale) {

    RecalculateFootCollider();
}

void Player::TakeDamage(int damage) {
    if (isInvincible || lives <= 0) return;

    lives -= damage;
    isInvincible = true;
    invincibilityTimer = invincibilityDuration;

    Sound ripSound = AssetManager::Instance().GetSound("card_rip");
    if (ripSound.stream.buffer != nullptr) {
        SetSoundVolume(ripSound, 0.55f);
        PlaySound(ripSound);
    }

    TraceLog(LOG_INFO, "PLAYER DAMAGE! Remaining lives: %d", lives);
}

void Player::UpdateBreathingSound() {
    Sound breathSound = AssetManager::Instance().GetSound("Breath");
    if (breathSound.stream.buffer == nullptr) return;

    float dt = GetFrameTime();
    float staminaRatio = currentStamina / maxStamina;
    bool shouldPlay = isExhausted || (staminaRatio < 0.35f);
    float maxVolume = 0.140f;

    if (shouldPlay) {
        if (!IsSoundPlaying(breathSound)) {
            currentBreathVolume = maxVolume;
            SetSoundVolume(breathSound, currentBreathVolume);
            PlaySound(breathSound);
        }
        else {
            currentBreathVolume = maxVolume;
            SetSoundVolume(breathSound, currentBreathVolume);
        }
    }
    else {
        if (currentBreathVolume > 0.01f) {
            float blendAlpha = 1.0f - expf(-3.5f * dt);
            currentBreathVolume = Lerp(currentBreathVolume, 0.0f, blendAlpha);
            SetSoundVolume(breathSound, currentBreathVolume);
        }
        else {
            if (IsSoundPlaying(breathSound)) {
                StopSound(breathSound);
            }
            currentBreathVolume = 0.0f;
        }
    }
}

void Player::RecalculateFootCollider() {
    float scaledWidth = frameWidth * scale;
    float scaledHeight = frameHeight * scale;

    footBoxWidth = scaledWidth * 0.55f;
    footBoxHeight = scaledHeight * 0.25f;

    footOffsetX = (scaledWidth - footBoxWidth) * 0.5f;
    footOffsetY = scaledHeight - footBoxHeight;

    collider = { position.x + footOffsetX, position.y + footOffsetY, footBoxWidth, footBoxHeight };
}

void Player::SetScale(float newScale) {
    scale = newScale;
    RecalculateFootCollider();
}

Rectangle Player::GetFootCollider() const {
    return { position.x + footOffsetX, position.y + footOffsetY, footBoxWidth, footBoxHeight };
}

void Player::PlayFootstepSound(bool isSprinting) {
    AssetManager& am = AssetManager::Instance();

    int soundIndex = GetRandomValue(1, 4);
    std::string soundKey = "step_wood_" + std::to_string(soundIndex);
    Sound stepSound = am.GetSound(soundKey);

    if (stepSound.stream.buffer != nullptr) {
        float randomPitch = 0.82f + ((float)GetRandomValue(-4, 4) / 100.0f);
        float baseVolume = isSprinting ? 0.65f : 0.45f;
        float randomVolume = baseVolume + ((float)GetRandomValue(-3, 3) / 100.0f);

        SetSoundPitch(stepSound, randomPitch);
        SetSoundVolume(stepSound, randomVolume);
        PlaySound(stepSound);
    }
}

void Player::Update(const GameMap& gameMap) {
    Vector2 moveDir = InputManager::GetMovementDirection();
    float deltaTime = GetFrameTime();

    if (isInvincible) {
        invincibilityTimer -= deltaTime;
        if (invincibilityTimer <= 0.0f) {
            isInvincible = false;
            invincibilityTimer = 0.0f;
        }
    }

    if (isMoving) {
        frameSpeed = isSprinting ? 0.075f : 0.135f;

        animTimer += deltaTime;
        if (animTimer >= frameSpeed) {
            animTimer = 0.0f;
            int prevFrame = currentFrame;
            currentFrame = (currentFrame + 1) % totalFrames;

            if ((currentFrame == 2 || currentFrame == 6) && prevFrame != currentFrame) {
                PlayFootstepSound(isSprinting);
            }
        }
    }
    else {
        frameSpeed = 0.16f;
        animTimer += deltaTime;
        if (animTimer >= frameSpeed) {
            animTimer = 0.0f;
            currentFrame = (currentFrame + 1) % totalFrames;
        }
    }

    isMoving = (moveDir.x != 0.0f or moveDir.y != 0.0f);
    float currentMoveSpeed = speed;
    isSprinting = InputManager::IsSprintPressed() && isMoving && !isExhausted;

    if (isSprinting) {
        currentMoveSpeed *= 1.68f;
        currentStamina -= staminaDrainRate * deltaTime;
        if (currentStamina <= 0.0f) {
            currentStamina = 0.0f;
            isExhausted = true;
        }
    }
    else {
        currentStamina += staminaRegenRate * deltaTime;
        if (currentStamina >= maxStamina) {
            currentStamina = maxStamina;
        }
        float minStaminaToSprint = maxStamina * 0.4f;
        if (isExhausted && currentStamina >= minStaminaToSprint) {
            isExhausted = false;
        }
    }

    float frameSpeed = currentMoveSpeed * deltaTime;
    const float nudgeTolerance = 13.0f;

    if (moveDir.x != 0.0f) {
        position.x += moveDir.x * frameSpeed;

        if (gameMap.CheckWallCollision(GetFootCollider())) {
            position.x -= moveDir.x * frameSpeed;

            if (activeNudgeY == 0) {
                Rectangle testUp = GetFootCollider();
                testUp.y -= nudgeTolerance;
                testUp.x += moveDir.x * frameSpeed;

                Rectangle testDown = GetFootCollider();
                testDown.y += nudgeTolerance;
                testDown.x += moveDir.x * frameSpeed;

                bool canPassUp = !gameMap.CheckWallCollision(testUp);
                bool canPassDown = !gameMap.CheckWallCollision(testDown);

                if (canPassUp && !canPassDown) {
                    activeNudgeY = -1;
                }
                else if (canPassDown && !canPassUp) {
                    activeNudgeY = 1;
                }
            }

            if (activeNudgeY == -1) {
                position.y -= frameSpeed;
                moveDir.y = 0.0f;
            }
            else if (activeNudgeY == 1) {
                position.y += frameSpeed;
                moveDir.y = 0.0f;
            }
        }
        else {
            activeNudgeY = 0;
        }
    }
    else {
        activeNudgeY = 0;
    }

    if (moveDir.y != 0.0f) {
        position.y += moveDir.y * frameSpeed;

        if (gameMap.CheckWallCollision(GetFootCollider())) {
            position.y -= moveDir.y * frameSpeed;

            if (activeNudgeX == 0) {
                Rectangle testLeft = GetFootCollider();
                testLeft.x -= nudgeTolerance;
                testLeft.y += moveDir.y * frameSpeed;

                Rectangle testRight = GetFootCollider();
                testRight.x += nudgeTolerance;
                testRight.y += moveDir.y * frameSpeed;

                bool canPassLeft = !gameMap.CheckWallCollision(testLeft);
                bool canPassRight = !gameMap.CheckWallCollision(testRight);

                if (canPassLeft && !canPassRight) {
                    activeNudgeX = -1;
                }
                else if (canPassRight && !canPassLeft) {
                    activeNudgeX = 1;
                }
            }

            if (activeNudgeX == -1) {
                position.x -= frameSpeed;
                moveDir.x = 0.0f;
            }
            else if (activeNudgeX == 1) {
                position.x += frameSpeed;
                moveDir.x = 0.0f;
            }
        }
        else {
            activeNudgeX = 0;
        }
    }
    else {
        activeNudgeX = 0;
    }

    UpdateBreathingSound();

    collider.x = position.x + footOffsetX;
    collider.y = position.y + footOffsetY;

    if (InputManager::IsInteractPressed()) {
        // Дія взаємодії
    }
}

void Player::Draw() {
    Color drawTint = WHITE;
    if (isInvincible) {
        float alpha = (sinf(GetTime() * 20.0f) > 0.0f) ? 0.3f : 0.8f;
        drawTint = ColorAlpha(WHITE, alpha);
    }

    if (texture.id > 0) {
        PlayerDirection dir = GetCurrentDirection();
        int rowIndex = 0;
        bool flipH = false;

        switch (dir) {
        case PlayerDirection::Up:
            rowIndex = isMoving ? 4 : 1;
            break;
        case PlayerDirection::Left:
            rowIndex = isMoving ? 5 : 2;
            flipH = true;
            break;
        case PlayerDirection::Right:
            rowIndex = isMoving ? 5 : 2;
            flipH = false;
            break;
        case PlayerDirection::Down:
        case PlayerDirection::None:
        default:
            rowIndex = isMoving ? 3 : 0;
            break;
        }

        Rectangle sourceRec = {
            (float)currentFrame * frameWidth,
            (float)rowIndex * frameHeight,
            flipH ? -frameWidth : frameWidth,
            frameHeight
        };

        float visualOffsetX = -1.5f * scale;

        Rectangle destRec = {
            position.x + visualOffsetX,
            position.y,
            frameWidth * scale,
            frameHeight * scale,
        };

        DrawTexturePro(texture, sourceRec, destRec, Vector2{ 0.0f, 0.0f }, 0.0f, drawTint);
    }
    else {
        DrawRectangleRec({ position.x, position.y, collider.width / 0.6f, collider.height / 0.3f }, BLUE);
    }
}

void Player::DrawDebug() const {
    Rectangle foot = GetFootCollider();
    DrawRectangleRec(foot, ColorAlpha(RED, 0.25f));
    DrawRectangleLinesEx(foot, 1.0f, RED);
}

PlayerDirection Player::GetCurrentDirection() const {
    return InputManager::GetLastDirection();
}