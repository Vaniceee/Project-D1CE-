#pragma once
#include "GameState.h"
#include "CutsceneUI.h"
#include "raylib.h"
#include <string>
#include <vector>

struct CutsceneLayerConfig {
    std::string textureKey;
    Vector2 designPos = { 0.0f, 0.0f }; // Позиція на базовому полотні 1920x1080
    bool alignRight = false;            // true = автоматично притискає до правого краю (1920 - width)
    float appearDelay = 0.0f;           // Коли починає з'являтися (сек)
    float fadeDuration = 0.5f;          // Тривалість плавної появи (сек)
};

struct CutsceneSubtitleSegment {
    float startTime = 0.0f;         // Коли починає друкуватися репліка (сек)
    float charsPerSec = 9.0f;       // Швидкість друку: 9 символів/сек
    std::string fullText;
};

struct CutscenePageData {
    bool isPlaceholder = false;
    std::string audioKey;
    float minCompletionTime = 0.0f;
    std::vector<CutsceneLayerConfig> layers;
    std::vector<CutsceneSubtitleSegment> subtitles;
};

class CutsceneState : public GameState {
public:
    void Init() override;
    void Cleanup() override;
    void Update() override;
    void Draw() override;

private:
    static constexpr float TEXT_TYPING_SPEED = 9.0f;

    void StartPage(int index);
    void StartRickrollPlaceholder(int pageIndex);
    void UnloadRickrollResources();
    void BeginExitToGameplay();
    float ComputeRequiredPageDuration(const CutscenePageData& page) const;
    std::string ComputeTypedSubtitle() const;

    std::vector<CutscenePageData> pages;
    int currentPageIndex = 0;

    float pageTimer = 0.0f;
    float animTime = 0.0f;
    bool pageSkipped = false;

    Music currentMusic{};
    bool hasActiveMusic = false;

    // Дані для спільного GIF-рікролу
    bool playingPlaceholder = false;
    Image rickrollImage{};
    Texture2D rickrollTexture{};
    int rickrollTotalFrames = 0;
    int rickrollCurrentFrame = -1;
    Music rickrollMusic{};
    bool rickrollLoaded = false;

    UiButton actionButton{};
    bool wasHoveringButton = false;

    bool exitingToGameplay = false;
    float fadeOutAlpha = 0.0f;
};