#include "CutsceneState.h"
#include "AssetManager.h"
#include "GameManager.h"
#include "GameplayState.h"
#include "CustomCursor.h"
#include "config.h"
#include <algorithm>
#include <cmath>

void CutsceneState::Init()
{
    auto& assets = AssetManager::Instance();

    // Використовуємо тільки існуючі константи з config.h
    assets.LoadTexture("cs_s1_base", Config::CUTSCENE_S1_BASE);
    assets.LoadTexture("cs_s1_overlay", Config::CUTSCENE_S1_OVERLAY);
    assets.LoadMusic("cs_s1_audio", Config::CUTSCENE_S1_AUDIO);

    pages.clear();

    // --- СТОРІНКА 1 (готова, синхронізована зі звуком на 6.0с) ---
    CutscenePageData page1;
    page1.isPlaceholder = false;
    page1.audioKey = "cs_s1_audio";
    page1.layers = {
        { "cs_s1_base",{ 0.0f, 0.0f }, false, 0.0f, 0.45f },
        // Якщо scene1b.png має ширину 1920px (повний кадр), зміни true на false
        { "cs_s1_overlay",{ 0.0f, 0.0f }, true,  6.0f, 0.35f }
    };
    page1.subtitles = {
        { 0.2f, TEXT_TYPING_SPEED, "Empty wallet. Heavy interest. Deadline: dawn." },
        { 6.0f, TEXT_TYPING_SPEED, "House of cards, stamped in red. Fold—or go all in." }
    };
    page1.minCompletionTime = ComputeRequiredPageDuration(page1);
    pages.push_back(page1);

    // --- СТОРІНКИ 2–5 (поки не заповнені — об'єднуються в один спільний рікрол) ---
    for (int i = 0; i < 4; ++i) {
        CutscenePageData stub;
        stub.isPlaceholder = true;
        pages.push_back(stub);
    }

    // Завантажуємо GIF + WAV для спільного рікролу ОДИН раз
    bool hasAnyPlaceholder = false;
    for (const auto& p : pages) {
        if (p.isPlaceholder) { hasAnyPlaceholder = true; break; }
    }

    if (hasAnyPlaceholder && FileExists(Config::CUTSCENE_RICKROLL_GIF)) {
        rickrollImage = LoadImageAnim(Config::CUTSCENE_RICKROLL_GIF, &rickrollTotalFrames);
        if (rickrollImage.data != nullptr && rickrollTotalFrames > 0) {
            rickrollTexture = LoadTextureFromImage(rickrollImage);
            assets.LoadMusic("cs_rickroll_wav", Config::CUTSCENE_RICKROLL_WAV);
            rickrollMusic = assets.GetMusic("cs_rickroll_wav");
            rickrollLoaded = true;
        }
    }

    // Ініціалізуємо кнопку дії
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    actionButton.bounds = { sw - 215.0f, sh - 96.0f, 175.0f, 68.0f };
    actionButton.text = "CONTINUE";
    actionButton.isHovered = false;
    actionButton.wasHovered = false;
    actionButton.hoverProgress = 0.0f;
    actionButton.isDanger = false;

    StartPage(0);
}

float CutsceneState::ComputeRequiredPageDuration(const CutscenePageData& page) const
{
    float maxTime = 6.5f;

    for (const auto& layer : page.layers) {
        maxTime = std::max(maxTime, layer.appearDelay + layer.fadeDuration + 0.5f);
    }

    for (const auto& sub : page.subtitles) {
        float speed = (sub.charsPerSec > 0.1f) ? sub.charsPerSec : TEXT_TYPING_SPEED;
        float endTextTime = sub.startTime + ((float)sub.fullText.size() / speed) + 0.6f;
        maxTime = std::max(maxTime, endTextTime);
    }

    return maxTime;
}

void CutsceneState::UnloadRickrollResources()
{
    if (!rickrollLoaded) return;

    if (rickrollMusic.stream.buffer != nullptr) {
        StopMusicStream(rickrollMusic);
    }
    if (rickrollTexture.id > 0) {
        UnloadTexture(rickrollTexture);
        rickrollTexture = {};
    }
    if (rickrollImage.data != nullptr) {
        UnloadImage(rickrollImage);
        rickrollImage = {};
    }
    rickrollLoaded = false;
}

void CutsceneState::Cleanup()
{
    if (hasActiveMusic) {
        StopMusicStream(currentMusic);
        hasActiveMusic = false;
    }

    UnloadRickrollResources();
    AssetManager::Instance().UnloadAll();
}

void CutsceneState::StartPage(int index)
{
    if (index >= (int)pages.size()) {
        BeginExitToGameplay();
        return;
    }

    currentPageIndex = index;
    pageTimer = 0.0f;
    pageSkipped = false;

    if (pages[index].isPlaceholder) {
        StartRickrollPlaceholder(index);
        return;
    }

    playingPlaceholder = false;
    actionButton.text = (index + 1 == (int)pages.size()) ? "PLAY" : "CONTINUE";

    if (hasActiveMusic) {
        StopMusicStream(currentMusic);
        hasActiveMusic = false;
    }

    const auto& page = pages[currentPageIndex];
    if (!page.audioKey.empty()) {
        currentMusic = AssetManager::Instance().GetMusic(page.audioKey);
        if (currentMusic.stream.buffer != nullptr) {
            currentMusic.looping = false;
            StopMusicStream(currentMusic);
            PlayMusicStream(currentMusic);
            hasActiveMusic = true;
        }
    }
}

void CutsceneState::StartRickrollPlaceholder(int pageIndex)
{
    if (hasActiveMusic) {
        StopMusicStream(currentMusic);
        hasActiveMusic = false;
    }

    int nextReadyPage = (int)pages.size();
    for (int i = pageIndex + 1; i < (int)pages.size(); ++i) {
        if (!pages[i].isPlaceholder) {
            nextReadyPage = i;
            break;
        }
    }

    if (!rickrollLoaded) {
        StartPage(nextReadyPage);
        return;
    }

    playingPlaceholder = true;
    pageTimer = 0.0f;
    rickrollCurrentFrame = -1;
    actionButton.text = (nextReadyPage >= (int)pages.size()) ? "SKIP" : "NEXT";

    if (rickrollMusic.stream.buffer != nullptr) {
        rickrollMusic.looping = true;
        StopMusicStream(rickrollMusic);
        PlayMusicStream(rickrollMusic);
    }
}

void CutsceneState::BeginExitToGameplay()
{
    if (!exitingToGameplay) {
        exitingToGameplay = true;
        fadeOutAlpha = 0.0f;
        UnloadRickrollResources();
    }
}

void CutsceneState::Update()
{
    const float dt = GetFrameTime();
    animTime += dt;

    if (exitingToGameplay) {
        if (hasActiveMusic) UpdateMusicStream(currentMusic);

        fadeOutAlpha += dt * 2.2f;
        if (fadeOutAlpha >= 1.0f) {
            GameManager::Instance().ChangeState(new GameplayState());
        }
        return;
    }

    Vector2 mousePos = GetMousePosition();
    actionButton.wasHovered = actionButton.isHovered;
    bool isHover = CheckCollisionPointRec(mousePos, actionButton.bounds);
    actionButton.isHovered = isHover;
    wasHoveringButton = isHover;

    float hoverBlend = 1.0f - expf(-12.0f * dt);
    actionButton.hoverProgress = Lerp(actionButton.hoverProgress, isHover ? 1.0f : 0.0f, hoverBlend);
    CustomCursor::Update(dt, isHover);

    // --- ЄДИНИЙ РІКРОЛ ДЛЯ ОБ'ЄДНАНИХ СТОРІНОК ---
    if (playingPlaceholder) {
        if (rickrollMusic.stream.buffer != nullptr) {
            UpdateMusicStream(rickrollMusic);
        }
        pageTimer += dt;

        const float gifFps = 12.0f;
        int targetFrame = (rickrollTotalFrames > 0)
            ? ((int)(pageTimer * gifFps) % rickrollTotalFrames)
            : 0;

        if (targetFrame != rickrollCurrentFrame && targetFrame >= 0 && rickrollImage.data != nullptr) {
            rickrollCurrentFrame = targetFrame;
            size_t frameBytes = (size_t)rickrollImage.width * (size_t)rickrollImage.height * 4;
            unsigned char* framePtr = (unsigned char*)rickrollImage.data + (frameBytes * (size_t)rickrollCurrentFrame);
            UpdateTexture(rickrollTexture, framePtr);
        }

        bool skipClicked = isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (skipClicked || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (rickrollMusic.stream.buffer != nullptr) {
                StopMusicStream(rickrollMusic);
            }
            int nextReadyPage = (int)pages.size();
            for (int i = currentPageIndex + 1; i < (int)pages.size(); ++i) {
                if (!pages[i].isPlaceholder) {
                    nextReadyPage = i;
                    break;
                }
            }
            StartPage(nextReadyPage);
        }
        return;
    }

    // --- ЗВИЧАЙНА СТОРІНКА КОМІКСУ (ЖОРСТКА СИНХРОНІЗАЦІЯ ЗІ ЗВУКОМ) ---
    if (hasActiveMusic) {
        UpdateMusicStream(currentMusic);
        float audioPos = GetMusicTimePlayed(currentMusic);
        if (IsMusicStreamPlaying(currentMusic) && audioPos > 0.001f) {
            pageTimer = audioPos;
        }
        else {
            pageTimer += dt;
        }
    }
    else {
        pageTimer += dt;
    }

    const auto& page = pages[currentPageIndex];
    bool pageFinished = pageSkipped || (pageTimer >= page.minCompletionTime);

    if (!pageFinished) {
        bool clickAnywhere = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isHover;
        if (clickAnywhere || IsKeyPressed(KEY_SPACE)) {
            pageSkipped = true;
            pageTimer = page.minCompletionTime + 1.0f;
        }
    }
    else {
        bool continueClicked = isHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (continueClicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            StartPage(currentPageIndex + 1);
        }
    }
}

std::string CutsceneState::ComputeTypedSubtitle() const
{
    if (playingPlaceholder) {
        int lastPlaceholder = currentPageIndex;
        while (lastPlaceholder + 1 < (int)pages.size() && pages[lastPlaceholder + 1].isPlaceholder) {
            lastPlaceholder++;
        }

        std::string msg = (currentPageIndex == lastPlaceholder)
            ? TextFormat("SCENE %d // WORK IN PROGRESS...", currentPageIndex + 1)
            : TextFormat("SCENES %d-%d // WORK IN PROGRESS...", currentPageIndex + 1, lastPlaceholder + 1);

        size_t charCount = (size_t)(pageTimer * TEXT_TYPING_SPEED);
        return (charCount >= msg.size()) ? msg : msg.substr(0, charCount);
    }

    const auto& page = pages[currentPageIndex];
    if (page.subtitles.empty()) return "";

    const CutsceneSubtitleSegment* activeSeg = nullptr;
    for (const auto& seg : page.subtitles) {
        if (pageTimer >= seg.startTime) {
            activeSeg = &seg;
        }
    }

    if (!activeSeg) return "";
    if (pageSkipped) return activeSeg->fullText;

    float localTime = pageTimer - activeSeg->startTime;
    float speed = (activeSeg->charsPerSec > 0.1f) ? activeSeg->charsPerSec : TEXT_TYPING_SPEED;
    size_t charCount = (size_t)(localTime * speed);
    if (charCount >= activeSeg->fullText.size()) {
        return activeSeg->fullText;
    }
    return activeSeg->fullText.substr(0, charCount);
}

void CutsceneState::Draw()
{
    std::string typedText = ComputeTypedSubtitle();

    if (playingPlaceholder) {
        int lastPlaceholder = currentPageIndex;
        while (lastPlaceholder + 1 < (int)pages.size() && pages[lastPlaceholder + 1].isPlaceholder) {
            lastPlaceholder++;
        }

        CutsceneUI::DrawPlaceholderVideo(
            rickrollTexture,
            0.0f,
            typedText,
            actionButton,
            currentPageIndex + 1,
            lastPlaceholder + 1,
            animTime,
            fadeOutAlpha
        );
        return;
    }

    const auto& page = pages[currentPageIndex];
    auto& assets = AssetManager::Instance();

    std::vector<CutsceneVisualLayer> visualLayers;
    visualLayers.reserve(page.layers.size());

    for (const auto& cfg : page.layers) {
        CutsceneVisualLayer vl;
        vl.texture = assets.GetTexture(cfg.textureKey);
        vl.designPos = cfg.designPos;

        if (cfg.alignRight && vl.texture.id > 0) {
            vl.designPos.x = CutsceneUI::DESIGN_WIDTH - (float)vl.texture.width;
        }

        if (pageSkipped) {
            vl.alpha = 1.0f;
        }
        else if (pageTimer < cfg.appearDelay) {
            vl.alpha = 0.0f;
        }
        else if (cfg.fadeDuration <= 0.001f) {
            vl.alpha = 1.0f;
        }
        else {
            vl.alpha = std::clamp((pageTimer - cfg.appearDelay) / cfg.fadeDuration, 0.0f, 1.0f);
        }
        visualLayers.push_back(vl);
    }

    bool showContinueBtn = pageSkipped || (pageTimer >= page.minCompletionTime);

    CutsceneUI::DrawComicPage(
        visualLayers,
        typedText,
        showContinueBtn,
        actionButton,
        currentPageIndex + 1,
        (int)pages.size(),
        animTime,
        fadeOutAlpha
    );
}