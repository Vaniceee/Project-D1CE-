#include "GameOverState.h"
#include "GameplayState.h"
#include "MainMenuState.h"
#include "GameManager.h"
#include "CustomCursor.h"
#include <cmath>
#include <vector>

void GameOverState::Init() {
    HideCursor();
    AssetManager& am = AssetManager::Instance();

    am.LoadTexture("card_lives", Config::PATH_PNG_CARD_LIVES);
    cardTex = am.GetTexture("card_lives");
    hasCardTex = (cardTex.id > 0);

    Wave clickWave = { 0 };
    clickWave.sampleRate = 44100;
    clickWave.sampleSize = 16;
    clickWave.channels = 1;
    clickWave.frameCount = 1100;
    short* samples = (short*)RL_MALLOC(clickWave.frameCount * sizeof(short));
    if (samples != nullptr) {
        for (unsigned int i = 0; i < clickWave.frameCount; ++i) {
            float t = (float)i / (float)clickWave.sampleRate;
            float env = expf(-t * 145.0f);
            float sig = sinf(2.0f * PI * 940.0f * t) * 0.65f + sinf(2.0f * PI * 1880.0f * t) * 0.35f;
            samples[i] = (short)(sig * env * 11000.0f);
        }
        clickWave.data = samples;
        hoverSound = LoadSoundFromWave(clickWave);
        UnloadWave(clickWave);
        hasHoverSound = (hoverSound.stream.buffer != nullptr);
        if (hasHoverSound) {
            SetSoundVolume(hoverSound, 0.28f);
        }
    }

    const std::vector<std::string> casinoQuotes = {
        "THE HOUSE ALWAYS WINS.",
        "ALL BETS ARE OFF. YOUR LUCK JUST RAN DRY.",
        "YOU PLAYED AGAINST THE ODDS — AND THE ODDS COLLECTED.",
        "NO MORE CHIPS. NO SECOND DEALS.",
        "THE DICE WERE LOADED FROM THE VERY FIRST STEP.",
        "ANOTHER LOST SOUL CASHED OUT IN THE DARK.",
        "YOU CANNOT BLUFF YOUR WAY OUT OF THIS FLOOR."
    };

    int randomIdx = GetRandomValue(0, (int)casinoQuotes.size() - 1);
    selectedQuote = casinoQuotes[randomIdx];

    animTime = 0.0f;
    fadeInAlpha = 1.0f;
    gameOverUI.Init();

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float btnWidth = 310.0f;
    float btnHeight = 58.0f;
    float btnX = (screenW - btnWidth) * 0.5f;

    tryAgainButton.bounds = { btnX, screenH * 0.65f, btnWidth, btnHeight };
    tryAgainButton.text = "TRY AGAIN";
    tryAgainButton.isHovered = false;
    tryAgainButton.wasHovered = false;
    tryAgainButton.hoverProgress = 0.0f;
    tryAgainButton.isDanger = false;

    mainMenuButton.bounds = { btnX, screenH * 0.65f + 78.0f, btnWidth, btnHeight };
    mainMenuButton.text = "MAIN MENU";
    mainMenuButton.isHovered = false;
    mainMenuButton.wasHovered = false;
    mainMenuButton.hoverProgress = 0.0f;
    mainMenuButton.isDanger = true;
}

void GameOverState::Cleanup() {
    if (hasMusic) {
        StopMusicStream(gameOverMusic);
    }
    if (hasHoverSound) {
        UnloadSound(hoverSound);
        hoverSound = { 0 };
        hasHoverSound = false;
    }
    AssetManager::Instance().UnloadAll();
}

void GameOverState::Update() {
    float dt = GetFrameTime();
    animTime += dt;

    if (fadeInAlpha > 0.0f) {
        fadeInAlpha -= dt * 1.4f;
        if (fadeInAlpha < 0.0f) fadeInAlpha = 0.0f;
    }

    if (hasMusic) {
        UpdateMusicStream(gameOverMusic);
    }

    Vector2 mousePos = GetMousePosition();
    gameOverUI.Update(dt, animTime, mousePos);

    tryAgainButton.wasHovered = tryAgainButton.isHovered;
    mainMenuButton.wasHovered = mainMenuButton.isHovered;

    tryAgainButton.isHovered = CheckCollisionPointRec(mousePos, tryAgainButton.bounds);
    mainMenuButton.isHovered = CheckCollisionPointRec(mousePos, mainMenuButton.bounds);

    if (hasHoverSound) {
        if (tryAgainButton.isHovered && !tryAgainButton.wasHovered) {
            SetSoundPitch(hoverSound, 1.08f);
            PlaySound(hoverSound);
        }
        else if (mainMenuButton.isHovered && !mainMenuButton.wasHovered) {
            SetSoundPitch(hoverSound, 0.86f);
            PlaySound(hoverSound);
        }
    }

    float animSpeed = 1.0f - expf(-12.0f * dt);
    tryAgainButton.hoverProgress = Lerp(tryAgainButton.hoverProgress, tryAgainButton.isHovered ? 1.0f : 0.0f, animSpeed);
    mainMenuButton.hoverProgress = Lerp(mainMenuButton.hoverProgress, mainMenuButton.isHovered ? 1.0f : 0.0f, animSpeed);

    CustomCursor::Update(dt, tryAgainButton.isHovered || mainMenuButton.isHovered);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (tryAgainButton.isHovered) {
            GameManager::Instance().ChangeState(new GameplayState());
            return;
        }
        else if (mainMenuButton.isHovered) {
            GameManager::Instance().ChangeState(new MainMenuState());
            return;
        }
    }
}

void GameOverState::Draw() {
    ClearBackground(Color{ 8, 5, 7, 255 });
    gameOverUI.Draw(
        animTime,
        fadeInAlpha,
        cardTex,
        hasCardTex,
        selectedQuote,
        tryAgainButton,
        mainMenuButton
    );
}