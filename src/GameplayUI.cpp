#include "GameplayUI.h"
#include "AssetManager.h"
#include "CustomCursor.h"
#include <cmath>

namespace {
    void DrawArtDecoDieFace(Rectangle rect, int value, Color borderColor, Color pipColor, float alpha) {
        DrawRectangle((int)rect.x + 2, (int)rect.y + 3, (int)rect.width, (int)rect.height, Fade(BLACK, 0.72f * alpha));
        DrawRectangleRounded(rect, 0.22f, 6, ColorAlpha(Color{ 14, 15, 19, 255 }, 0.94f * alpha));
        DrawRectangleRoundedLinesEx(rect, 0.22f, 6, 1.5f, ColorAlpha(borderColor, 0.92f * alpha));

        Rectangle inner = { rect.x + 2.5f, rect.y + 2.5f, rect.width - 5.0f, rect.height - 5.0f };
        DrawRectangleRoundedLinesEx(inner, 0.16f, 4, 0.8f, ColorAlpha(borderColor, 0.32f * alpha));

        float cx = rect.x + rect.width * 0.5f;
        float cy = rect.y + rect.height * 0.5f;
        float off = rect.width * 0.25f;
        float r = rect.width * 0.085f;
        Color col = ColorAlpha(pipColor, alpha);

        auto drawPip = [&](float px, float py) {
            DrawCircleV({ px, py }, r + 0.8f, Fade(BLACK, 0.6f * alpha));
            DrawCircleV({ px, py }, r, col);
            };

        if (value == 1 || value == 3 || value == 5) drawPip(cx, cy);
        if (value >= 2) {
            drawPip(cx - off, cy - off);
            drawPip(cx + off, cy + off);
        }
        if (value >= 4) {
            drawPip(cx + off, cy - off);
            drawPip(cx - off, cy + off);
        }
        if (value == 6) {
            drawPip(cx - off, cy);
            drawPip(cx + off, cy);
        }
    }

    // Тимчасова векторна заглушка іконки, поки текстура не завантажена
    void DrawPlaceholderKeyGlyph(int slotIndex, Vector2 center, float size, Color col) {
        if (slotIndex == 0) {
            // RoomKey — силует ключа
            DrawCircleLinesV({ center.x - size * 0.22f, center.y }, size * 0.22f, col);
            DrawLineEx({ center.x, center.y }, { center.x + size * 0.36f, center.y }, 2.0f, col);
            DrawLineEx({ center.x + size * 0.22f, center.y }, { center.x + size * 0.22f, center.y + size * 0.18f }, 2.0f, col);
            DrawLineEx({ center.x + size * 0.34f, center.y }, { center.x + size * 0.34f, center.y + size * 0.18f }, 2.0f, col);
        }
        else if (slotIndex == 1) {
            // SafeKeyCard — перфорована картка
            Rectangle card = { center.x - size * 0.34f, center.y - size * 0.24f, size * 0.68f, size * 0.48f };
            DrawRectangleLinesEx(card, 1.6f, col);
            DrawLineEx({ card.x + 3.0f, card.y + card.height * 0.35f }, { card.x + card.width - 3.0f, card.y + card.height * 0.35f }, 2.2f, col);
            DrawPoly({ card.x + card.width * 0.72f, card.y + card.height * 0.68f }, 4, 3.2f, 0.0f, col);
        }
        else {
            // MasterKey — подвійний золотий ромб
            DrawPolyLinesEx(center, 4, size * 0.36f, 0.0f, 1.8f, col);
            DrawPoly(center, 4, size * 0.16f, 0.0f, col);
        }
    }
}

void GameplayUI::Init() {
    // Генерація короткого тактильного клацання при наведенні на кнопки
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
            SetSoundVolume(hoverSound, 0.26f);
        }
    }

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float btnWidth = 310.0f;
    float btnHeight = 56.0f;
    float btnX = (screenW - btnWidth) * 0.5f;
    float startY = (screenH * 0.5f) - 62.0f;
    float gapY = 74.0f;

    resumeButton.bounds = { btnX, startY, btnWidth, btnHeight };
    resumeButton.text = "RESUME";
    resumeButton.isDanger = false;

    restartButton.bounds = { btnX, startY + gapY, btnWidth, btnHeight };
    restartButton.text = "RESTART";
    restartButton.isDanger = false;

    exitToMenuButton.bounds = { btnX, startY + gapY * 2.0f, btnWidth, btnHeight };
    exitToMenuButton.text = "MAIN MENU";
    exitToMenuButton.isDanger = true;

    ResetPauseMenu();
}

void GameplayUI::Cleanup() {
    if (hasHoverSound) {
        UnloadSound(hoverSound);
        hoverSound = { 0 };
        hasHoverSound = false;
    }
}

void GameplayUI::ResetPauseMenu() {
    pauseAnimTime = 0.0f;
    resumeButton.isHovered = false;
    resumeButton.wasHovered = false;
    resumeButton.hoverProgress = 0.0f;

    restartButton.isHovered = false;
    restartButton.wasHovered = false;
    restartButton.hoverProgress = 0.0f;

    exitToMenuButton.isHovered = false;
    exitToMenuButton.wasHovered = false;
    exitToMenuButton.hoverProgress = 0.0f;
}

PauseMenuAction GameplayUI::UpdatePauseMenu(float dt) {
    pauseAnimTime += dt;
    Vector2 mousePos = GetMousePosition();

    resumeButton.wasHovered = resumeButton.isHovered;
    restartButton.wasHovered = restartButton.isHovered;
    exitToMenuButton.wasHovered = exitToMenuButton.isHovered;

    resumeButton.isHovered = CheckCollisionPointRec(mousePos, resumeButton.bounds);
    restartButton.isHovered = CheckCollisionPointRec(mousePos, restartButton.bounds);
    exitToMenuButton.isHovered = CheckCollisionPointRec(mousePos, exitToMenuButton.bounds);

    if (hasHoverSound) {
        if ((resumeButton.isHovered && !resumeButton.wasHovered) ||
            (restartButton.isHovered && !restartButton.wasHovered)) {
            SetSoundPitch(hoverSound, 1.08f);
            PlaySound(hoverSound);
        }
        else if (exitToMenuButton.isHovered && !exitToMenuButton.wasHovered) {
            SetSoundPitch(hoverSound, 0.86f);
            PlaySound(hoverSound);
        }
    }

    float animSpeed = 1.0f - expf(-12.0f * dt);
    resumeButton.hoverProgress = Lerp(resumeButton.hoverProgress, resumeButton.isHovered ? 1.0f : 0.0f, animSpeed);
    restartButton.hoverProgress = Lerp(restartButton.hoverProgress, restartButton.isHovered ? 1.0f : 0.0f, animSpeed);
    exitToMenuButton.hoverProgress = Lerp(exitToMenuButton.hoverProgress, exitToMenuButton.isHovered ? 1.0f : 0.0f, animSpeed);

    CustomCursor::Update(dt, resumeButton.isHovered || restartButton.isHovered || exitToMenuButton.isHovered);

    if (IsKeyPressed(KEY_R)) {
        return PauseMenuAction::RESTART;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (resumeButton.isHovered) {
            return PauseMenuAction::RESUME;
        }
        if (restartButton.isHovered) {
            return PauseMenuAction::RESTART;
        }
        if (exitToMenuButton.isHovered) {
            return PauseMenuAction::MAIN_MENU;
        }
    }

    return PauseMenuAction::NONE;
}

void GameplayUI::DrawPauseMenu() const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // 1. Темне затемнення + сканлайни режиму заморозки камери спостереження
    DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 0.78f));
    for (int y = 0; y < (int)screenH; y += 4) {
        DrawRectangle(0, y, (int)screenW, 1, Fade(WHITE, 0.03f));
    }

    // 2. Кутові візири CCTV
    float margin = 26.0f;
    float arm = 26.0f;
    Color hudCol = ColorAlpha(GOLD, 0.35f);
    DrawLineEx({ margin, margin }, { margin + arm, margin }, 1.5f, hudCol);
    DrawLineEx({ margin, margin }, { margin, margin + arm }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin - arm, margin }, { screenW - margin, margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + arm }, 1.5f, hudCol);
    DrawLineEx({ margin, screenH - margin }, { margin + arm, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ margin, screenH - margin - arm }, { margin, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin - arm, screenH - margin }, { screenW - margin, screenH - margin }, 1.5f, hudCol);
    DrawLineEx({ screenW - margin, screenH - margin - arm }, { screenW - margin, screenH - margin }, 1.5f, hudCol);

    if (fmodf(pauseAnimTime, 1.2f) < 0.75f) {
        DrawRectangle((int)(margin + 14.0f), (int)(margin + 14.0f), 8, 8, GOLD);
    }
    DrawText("PAUSE // CAM-04 FREEZE", (int)(margin + 30.0f), (int)(margin + 11.0f), 15, ColorAlpha(RAYWHITE, 0.7f));

    // 3. Неоновий заголовок PAUSED
    const char* pauseTitle = "PAUSED";
    int fontSize = 58;
    int textWidth = MeasureText(pauseTitle, fontSize);
    int titleX = (int)((screenW - textWidth) * 0.5f);
    int titleY = (int)(screenH * 0.23f);
    float glow = 0.85f + 0.15f * sinf(pauseAnimTime * 3.0f);

    DrawText(pauseTitle, titleX - 2, titleY + 2, fontSize, ColorAlpha(RED, 0.3f * glow));
    DrawText(pauseTitle, titleX - 1, titleY, fontSize, ColorAlpha(GOLD, 0.4f * glow));
    DrawText(pauseTitle, titleX + 1, titleY, fontSize, ColorAlpha(GOLD, 0.4f * glow));
    DrawText(pauseTitle, titleX, titleY, fontSize, Color{ 255, 226, 110, 255 });

    float centerX = screenW * 0.5f;
    float lineY = (float)(titleY + fontSize + 12);
    DrawLineEx({ centerX - 120.0f, lineY }, { centerX - 12.0f, lineY }, 1.5f, ColorAlpha(GOLD, 0.45f));
    DrawLineEx({ centerX + 12.0f, lineY }, { centerX + 120.0f, lineY }, 1.5f, ColorAlpha(GOLD, 0.45f));
    DrawPoly({ centerX, lineY }, 4, 5.0f, 0.0f, GOLD);

    // 4. Кнопки у стилі Art Deco
    DrawArtDecoButton(resumeButton, pauseAnimTime);
    DrawArtDecoButton(restartButton, pauseAnimTime);
    DrawArtDecoButton(exitToMenuButton, pauseAnimTime);

    const char* hint = "[ESC] RESUME   //   [R] QUICK RESTART";
    int hintW = MeasureText(hint, 13);
    DrawText(hint, (int)(centerX - hintW * 0.5f), (int)(screenH - margin - 24.0f), 13, ColorAlpha(LIGHTGRAY, 0.52f));

    CustomCursor::Draw(exitToMenuButton.isHovered);
}

void GameplayUI::DrawArtDecoButton(const UiButton& btn, float animTime) {
    float p = btn.hoverProgress;
    float expandX = p * 12.0f;
    float expandY = p * 2.0f;

    float jitterX = (btn.isDanger && p > 0.05f) ? sinf(animTime * 56.0f) * 1.3f * p : 0.0f;
    float jitterY = (btn.isDanger && p > 0.05f) ? cosf(animTime * 45.0f) * 0.8f * p : 0.0f;

    Rectangle rect = {
        btn.bounds.x - expandX + jitterX,
        btn.bounds.y - expandY + jitterY,
        btn.bounds.width + expandX * 2.0f,
        btn.bounds.height + expandY * 2.0f
    };

    Color accentColor = btn.isDanger ? Color{ 225, 48, 56, 255 } : GOLD;
    Color darkFillBase = btn.isDanger ? Color{ 16, 10, 12, 225 } : Color{ 12, 13, 16, 225 };
    Color hoverTint = btn.isDanger ? ColorAlpha(MAROON, 0.36f * p) : ColorAlpha(GOLD, 0.16f * p);

    DrawRectangle((int)rect.x + 4, (int)rect.y + 5, (int)rect.width, (int)rect.height, Fade(BLACK, 0.7f));
    if (p > 0.01f) {
        Rectangle glowRect = { rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
        DrawRectangleLinesEx(glowRect, 2.0f, ColorAlpha(accentColor, 0.25f * p));
    }

    DrawRectangleRec(rect, darkFillBase);
    if (p > 0.01f) {
        DrawRectangleRec(rect, hoverTint);
    }

    if (p > 0.02f) {
        BeginScissorMode((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height);
        float sweepPhase = fmodf(animTime * 1.35f, 1.8f) - 0.4f;
        float sweepX = rect.x + sweepPhase * rect.width;
        DrawLineEx(
            { sweepX - 18.0f, rect.y + rect.height },
            { sweepX + 18.0f, rect.y },
            14.0f,
            ColorAlpha(accentColor, 0.14f * p)
        );
        EndScissorMode();
    }

    Color outerBorder = ColorLerp(Color{ 95, 95, 102, 200 }, accentColor, p);
    Color innerBorder = ColorAlpha(accentColor, 0.18f + 0.45f * p);

    DrawRectangleLinesEx(rect, 2.0f, outerBorder);
    Rectangle innerRect = { rect.x + 5.0f, rect.y + 5.0f, rect.width - 10.0f, rect.height - 10.0f };
    DrawRectangleLinesEx(innerRect, 1.0f, innerBorder);

    float cornerR = 3.0f + p * 1.0f;
    Color cornerCol = ColorLerp(GRAY, accentColor, p);
    DrawPoly({ innerRect.x, innerRect.y }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x + innerRect.width, innerRect.y }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x, innerRect.y + innerRect.height }, 4, cornerR, 0.0f, cornerCol);
    DrawPoly({ innerRect.x + innerRect.width, innerRect.y + innerRect.height }, 4, cornerR, 0.0f, cornerCol);

    int fontSize = 24;
    int textWidth = MeasureText(btn.text.c_str(), fontSize);
    float textX = rect.x + (rect.width - textWidth) * 0.5f;
    float textY = rect.y + (rect.height - fontSize) * 0.5f;

    Color textColor = ColorLerp(LIGHTGRAY, WHITE, p);
    DrawText(btn.text.c_str(), (int)textX + 1, (int)textY + 2, fontSize, Fade(BLACK, 0.8f));
    DrawText(btn.text.c_str(), (int)textX, (int)textY, fontSize, textColor);

    if (p > 0.05f) {
        float diamondOffset = 18.0f + p * 10.0f;
        float diamondRadius = 4.5f * p;
        float midY = rect.y + rect.height * 0.5f;
        Color sideDiamondCol = ColorAlpha(accentColor, p);

        DrawPoly({ textX - diamondOffset, midY }, 4, diamondRadius, 0.0f, sideDiamondCol);
        DrawPoly({ textX + textWidth + diamondOffset, midY }, 4, diamondRadius, 0.0f, sideDiamondCol);
    }
}

void GameplayUI::DrawInventorySlots(const GameplayHudData& data, float screenW, float screenH) const {
    struct SlotInfo {
        bool owned;
        const char* texKey;
        const char* shortLabel;
        Color activeColor;
    };

    const SlotInfo slots[3] = {
        { data.hasRoomKey,     "icon_room_key",   "ROOM KEY",   GOLD },
        { data.hasSafeKeyCard, "icon_safe_card",  "SAFE CARD",  Color{ 110, 225, 175, 255 } },
        { data.hasMasterKey,   "icon_master_key", "MASTER KEY", Color{ 255, 226, 110, 255 } }
    };

    const float slotSize = 56.0f;
    const float slotGap = 14.0f;
    const float totalW = slotSize * 3.0f + slotGap * 2.0f;
    const float rightEdge = screenW - 46.0f;
    const float startX = rightEdge - totalW;
    const float slotY = screenH - 48.0f - slotSize;

    // Заголовок блоку інвентарю (симетрично до "STAMINA" зліва)
    const char* invHeader = "KEYRING // ACCESS";
    int headerW = MeasureText(invHeader, 13);
    DrawText(invHeader, (int)(rightEdge - headerW - 14.0f), (int)(slotY - 22.0f), 13, ColorAlpha(GOLD, 0.88f));

    // Бокові декоративні ромби лінії
    DrawPoly({ startX - 12.0f, slotY + slotSize * 0.5f }, 4, 5.5f, 0.0f, ColorAlpha(GOLD, 0.75f));
    DrawPoly({ rightEdge + 12.0f, slotY + slotSize * 0.5f }, 4, 7.0f, 0.0f, ColorAlpha(GOLD, 0.90f));
    DrawPolyLinesEx({ rightEdge + 12.0f, slotY + slotSize * 0.5f }, 4, 11.0f, 0.0f, 1.2f, ColorAlpha(GOLD, 0.45f));

    for (int i = 0; i < 3; ++i) {
        float x = startX + i * (slotSize + slotGap);
        Rectangle outer = { x, slotY, slotSize, slotSize };
        Rectangle inner = { x + 4.0f, slotY + 4.0f, slotSize - 8.0f, slotSize - 8.0f };
        Vector2 center = { x + slotSize * 0.5f, slotY + slotSize * 0.5f };

        bool owned = slots[i].owned;
        Color accent = owned ? slots[i].activeColor : Color{ 95, 95, 102, 160 };

        // Тінь і обсидіановий фон слоту
        DrawRectangle((int)outer.x + 3, (int)outer.y + 4, (int)outer.width, (int)outer.height, Fade(BLACK, 0.75f));
        DrawRectangleRec(outer, owned ? Color{ 16, 18, 22, 230 } : Color{ 10, 11, 14, 195 });

        if (owned) {
            float pulse = 0.75f + 0.25f * sinf(data.shaderTime * 3.0f + (float)i);
            DrawRectangleRec(outer, ColorAlpha(accent, 0.10f * pulse));
        }

        DrawRectangleLinesEx(outer, owned ? 1.6f : 1.1f, ColorAlpha(accent, owned ? 0.88f : 0.35f));
        DrawRectangleLinesEx(inner, 1.0f, ColorAlpha(accent, owned ? 0.30f : 0.12f));

        // Кутові ромбики активного слоту
        if (owned) {
            DrawPoly({ inner.x, inner.y }, 4, 2.8f, 0.0f, accent);
            DrawPoly({ inner.x + inner.width, inner.y }, 4, 2.8f, 0.0f, accent);
            DrawPoly({ inner.x, inner.y + inner.height }, 4, 2.8f, 0.0f, accent);
            DrawPoly({ inner.x + inner.width, inner.y + inner.height }, 4, 2.8f, 0.0f, accent);
        }

        // Текстура предмета (якщо вже додана) або стилізована заглушка
        Texture2D iconTex = AssetManager::Instance().GetTexture(slots[i].texKey);
        if (iconTex.id > 0) {
            float pad = 9.0f;
            float avail = slotSize - pad * 2.0f;
            float scale = fminf(avail / (float)iconTex.width, avail / (float)iconTex.height);
            float drawW = (float)iconTex.width * scale;
            float drawH = (float)iconTex.height * scale;

            Rectangle src = { 0.0f, 0.0f, (float)iconTex.width, (float)iconTex.height };
            Rectangle dst = { center.x - drawW * 0.5f, center.y - drawH * 0.5f, drawW, drawH };
            Color tint = owned ? WHITE : ColorAlpha(WHITE, 0.14f);
            DrawTexturePro(iconTex, src, dst, { 0.0f, 0.0f }, 0.0f, tint);
        }
        else {
            Color glyphCol = owned ? accent : ColorAlpha(GRAY, 0.22f);
            DrawPlaceholderKeyGlyph(i, center, slotSize * 0.72f, glyphCol);
        }

        // Підпис під слотом
        const char* subText = owned ? slots[i].shortLabel : "EMPTY";
        int subSize = 10;
        int subW = MeasureText(subText, subSize);
        DrawText(
            subText,
            (int)(center.x - subW * 0.5f),
            (int)(slotY + slotSize + 5.0f),
            subSize,
            owned ? ColorAlpha(accent, 0.90f) : ColorAlpha(GRAY, 0.42f)
        );
    }
}

void GameplayUI::DrawInteractPrompt(float animTime) const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float pulse = 0.85f + 0.15f * sinf(animTime * 4.5f);

    const char* label = "INTERACT";
    int labelSize = 17;
    int labelW = MeasureText(label, labelSize);

    float keySize = 28.0f;
    float padX = 18.0f;
    float gap = 12.0f;
    float boxW = padX * 2.0f + keySize + gap + (float)labelW;
    float boxH = 44.0f;
    float boxX = (screenW - boxW) * 0.5f;
    float boxY = screenH - 134.0f;

    Rectangle rect = { boxX, boxY, boxW, boxH };
    Rectangle inner = { boxX + 4.0f, boxY + 4.0f, boxW - 8.0f, boxH - 8.0f };

    DrawRectangle((int)boxX + 3, (int)boxY + 4, (int)boxW, (int)boxH, Fade(BLACK, 0.78f));
    DrawRectangleRec(rect, Color{ 11, 12, 16, 232 });
    DrawRectangleLinesEx(rect, 1.5f, ColorAlpha(GOLD, 0.85f * pulse));
    DrawRectangleLinesEx(inner, 1.0f, ColorAlpha(GOLD, 0.28f));

    // Бокові ромби та вусики
    float midY = boxY + boxH * 0.5f;
    DrawLineEx({ boxX - 24.0f, midY }, { boxX - 6.0f, midY }, 1.2f, ColorAlpha(GOLD, 0.55f));
    DrawLineEx({ boxX + boxW + 6.0f, midY }, { boxX + boxW + 24.0f, midY }, 1.2f, ColorAlpha(GOLD, 0.55f));
    DrawPoly({ boxX - 6.0f, midY }, 4, 4.5f, 0.0f, GOLD);
    DrawPoly({ boxX + boxW + 6.0f, midY }, 4, 4.5f, 0.0f, GOLD);

    // Клавіша [ E ] у стилі об'ємного кейкапа
    float keyX = boxX + padX;
    float keyY = boxY + (boxH - keySize) * 0.5f - 1.0f;
    Rectangle keyBase = { keyX, keyY + 2.5f, keySize, keySize };
    Rectangle keyTop = { keyX, keyY, keySize, keySize };

    DrawRectangleRounded(keyBase, 0.22f, 6, Color{ 110, 84, 30, 255 });
    DrawRectangleRounded(keyTop, 0.22f, 6, Color{ 18, 20, 26, 255 });
    DrawRectangleRoundedLinesEx(keyTop, 0.22f, 6, 1.4f, GOLD);

    int eSize = 16;
    int eW = MeasureText("E", eSize);
    DrawText("E", (int)(keyX + (keySize - eW) * 0.5f), (int)(keyY + (keySize - eSize) * 0.5f), eSize, Color{ 255, 234, 155, 255 });

    // Напис INTERACT
    float textX = keyX + keySize + gap;
    float textY = boxY + (boxH - labelSize) * 0.5f;
    DrawText(label, (int)textX + 1, (int)textY + 1, labelSize, Fade(BLACK, 0.85f));
    DrawText(label, (int)textX, (int)textY, labelSize, Color{ 245, 236, 210, 255 });
}

void GameplayUI::DrawStatusNotification(const std::string& text, Color accentColor, float timer, float maxDuration, float animTime) const {
    if (timer <= 0.0f || text.empty()) return;

    float fadeIn = Clamp((maxDuration - timer) / 0.18f, 0.0f, 1.0f);
    float fadeOut = Clamp(timer / 0.35f, 0.0f, 1.0f);
    float alpha = fminf(fadeIn, fadeOut);

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    int fontSize = 18;
    int textW = MeasureText(text.c_str(), fontSize);
    float boxW = (float)textW + 64.0f;
    float boxH = 42.0f;
    float boxX = (screenW - boxW) * 0.5f;
    float boxY = screenH - 188.0f - (1.0f - fadeIn) * 8.0f;

    Rectangle rect = { boxX, boxY, boxW, boxH };
    Rectangle inner = { boxX + 4.0f, boxY + 4.0f, boxW - 8.0f, boxH - 8.0f };

    DrawRectangle((int)boxX + 3, (int)boxY + 4, (int)boxW, (int)boxH, Fade(BLACK, 0.78f * alpha));
    DrawRectangleRec(rect, ColorAlpha(Color{ 12, 11, 15, 255 }, 0.92f * alpha));
    DrawRectangleRec(rect, ColorAlpha(accentColor, 0.12f * alpha));

    DrawRectangleLinesEx(rect, 1.6f, ColorAlpha(accentColor, 0.90f * alpha));
    DrawRectangleLinesEx(inner, 1.0f, ColorAlpha(accentColor, 0.32f * alpha));

    float midY = boxY + boxH * 0.5f;
    DrawPoly({ boxX + 16.0f, midY }, 4, 4.5f, animTime * 40.0f, ColorAlpha(accentColor, alpha));
    DrawPoly({ boxX + boxW - 16.0f, midY }, 4, 4.5f, -animTime * 40.0f, ColorAlpha(accentColor, alpha));

    int tx = (int)(boxX + (boxW - (float)textW) * 0.5f);
    int ty = (int)(boxY + (boxH - (float)fontSize) * 0.5f);
    DrawText(text.c_str(), tx + 1, ty + 1, fontSize, Fade(BLACK, 0.85f * alpha));
    DrawText(text.c_str(), tx, ty, fontSize, ColorAlpha(RAYWHITE, 0.96f * alpha));
}

void GameplayUI::DrawDiceRollWorld(Vector2 playerHeadWorldPos, int d1, int d2, int sum, float timer, float duration) const {
    if (timer <= 0.0f) return;

    float elapsed = duration - timer;
    float alpha = Clamp(timer / 0.28f, 0.0f, 1.0f);
    float riseOffset = Clamp(elapsed / 0.25f, 0.0f, 1.0f) * 6.0f;

    // Коротка імітація перекочування граней у перші 0.22 с
    bool isRolling = (elapsed < 0.22f);
    int shownD1 = isRolling ? GetRandomValue(1, 6) : d1;
    int shownD2 = isRolling ? GetRandomValue(1, 6) : d2;
    float shakeX = isRolling ? (float)GetRandomValue(-2, 2) : 0.0f;
    float shakeY = isRolling ? (float)GetRandomValue(-2, 2) : 0.0f;

    const float dieSize = 20.0f;
    const float gap = 5.0f;
    const float totalW = dieSize * 2.0f + gap;
    float startX = playerHeadWorldPos.x - totalW * 0.5f + shakeX;
    float startY = playerHeadWorldPos.y - 26.0f - riseOffset + shakeY;

    Color accent = (sum >= 7) ? GOLD : Color{ 235, 65, 72, 255 };

    DrawArtDecoDieFace({ startX, startY, dieSize, dieSize }, shownD1, accent, RAYWHITE, alpha);
    DrawArtDecoDieFace({ startX + dieSize + gap, startY, dieSize, dieSize }, shownD2, accent, RAYWHITE, alpha);

    if (!isRolling) {
        const char* sumText = TextFormat("= %d", sum);
        int fontSize = 12;
        int tw = MeasureText(sumText, fontSize);
        float badgeW = (float)tw + 10.0f;
        float badgeH = 16.0f;
        float bx = playerHeadWorldPos.x - badgeW * 0.5f;
        float by = startY - badgeH - 3.0f;

        DrawRectangle((int)bx, (int)by, (int)badgeW, (int)badgeH, ColorAlpha(Color{ 10, 11, 14, 255 }, 0.88f * alpha));
        DrawRectangleLinesEx({ bx, by, badgeW, badgeH }, 1.0f, ColorAlpha(accent, 0.85f * alpha));
        DrawText(sumText, (int)(bx + 5.0f), (int)(by + 2.0f), fontSize, ColorAlpha(accent, alpha));
    }
}

void GameplayUI::DrawHud(const GameplayHudData& data) const {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    int lives = data.lives;
    float shaderTime = data.shaderTime;
    bool isCritical = (lives == 1);
    float dangerBeat = isCritical ? (0.5f + 0.5f * sinf(shaderTime * 7.5f)) : 0.0f;

    Color goldLine = ColorAlpha(GOLD, 0.42f);
    Color goldBright = ColorAlpha(GOLD, 0.85f);
    Color goldDim = ColorAlpha(GOLD, 0.22f);

    // =========================================================================
    // 1. ВІДКРИТЕ АР-ДЕКО ОБРАМЛЕННЯ ПО ПЕРИМЕТРУ ЕКРАНА
    // =========================================================================
    float margin = 26.0f;
    float arm = 38.0f;

    DrawLineEx({ margin, margin }, { margin + arm + 195.0f, margin }, 1.5f, goldLine);
    DrawLineEx({ margin, margin }, { margin, margin + arm + 225.0f }, 1.5f, goldLine);
    DrawPoly({ margin, margin }, 4, 5.0f, 0.0f, goldBright);

    DrawLineEx({ screenW - margin - arm - 180.0f, margin }, { screenW - margin, margin }, 1.5f, goldLine);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + arm + 95.0f }, 1.5f, goldLine);
    DrawPoly({ screenW - margin, margin }, 4, 4.5f, 0.0f, goldBright);

    DrawLineEx({ margin, screenH - margin }, { margin + arm + 320.0f, screenH - margin }, 1.5f, goldLine);
    DrawLineEx({ margin, screenH - margin - arm - 70.0f }, { margin, screenH - margin }, 1.5f, goldLine);
    DrawPoly({ margin, screenH - margin }, 4, 4.5f, 0.0f, goldBright);

    DrawLineEx({ screenW - margin - arm - 230.0f, screenH - margin }, { screenW - margin, screenH - margin }, 1.5f, goldLine);
    DrawLineEx({ screenW - margin, screenH - margin - arm - 65.0f }, { screenW - margin, screenH - margin }, 1.5f, goldLine);
    DrawPoly({ screenW - margin, screenH - margin }, 4, 4.5f, 0.0f, goldBright);

    // =========================================================================
    // 2. ЛІВИЙ ВЕРХНІЙ ПРОСТІР: КОЗИРНА ВІТРИНА ТУЗА (ГОЛОВНИЙ ЯКІР HUD)
    // =========================================================================
    Vector2 cardCenter = { 126.0f, 134.0f };
    Color cardAccent = isCritical ? ColorLerp(GOLD, Color{ 235, 52, 60, 255 }, 0.35f + 0.65f * dangerBeat) : GOLD;

    DrawCircleV(cardCenter, 88.0f, Fade(BLACK, 0.50f));
    DrawPoly(cardCenter, 4, 86.0f, 0.0f, Color{ 10, 11, 14, 175 });
    DrawPolyLinesEx(cardCenter, 4, 86.0f, 0.0f, 1.8f, ColorAlpha(cardAccent, isCritical ? (0.45f + 0.45f * dangerBeat) : 0.48f));
    DrawPolyLinesEx(cardCenter, 4, 75.0f, 0.0f, 1.0f, ColorAlpha(cardAccent, 0.24f));

    float wingSpan = 96.0f;
    DrawLineEx({ cardCenter.x - wingSpan, cardCenter.y }, { cardCenter.x - 74.0f, cardCenter.y }, 1.5f, ColorAlpha(cardAccent, 0.55f));
    DrawLineEx({ cardCenter.x + 74.0f, cardCenter.y }, { cardCenter.x + wingSpan, cardCenter.y }, 1.5f, ColorAlpha(cardAccent, 0.55f));
    DrawPoly({ cardCenter.x - wingSpan, cardCenter.y }, 4, 4.0f, 0.0f, ColorAlpha(cardAccent, 0.85f));
    DrawPoly({ cardCenter.x + wingSpan, cardCenter.y }, 4, 4.0f, 0.0f, ColorAlpha(cardAccent, 0.85f));

    float topCrownY = 46.0f;
    DrawLineEx({ cardCenter.x - 54.0f, topCrownY }, { cardCenter.x - 10.0f, topCrownY }, 1.2f, ColorAlpha(cardAccent, 0.45f));
    DrawLineEx({ cardCenter.x + 10.0f, topCrownY }, { cardCenter.x + 54.0f, topCrownY }, 1.2f, ColorAlpha(cardAccent, 0.45f));
    DrawPoly({ cardCenter.x, topCrownY }, 4, 4.5f, 0.0f, ColorAlpha(cardAccent, 0.9f));

    float floatY = sinf(shaderTime * 2.4f) * 3.8f;
    float tiltAngle = sinf(shaderTime * 1.7f) * 2.6f;
    Vector2 animatedCenter = { cardCenter.x, cardCenter.y + floatY };

    if (lives >= 3) {
        Rectangle backRight = { animatedCenter.x + 15.0f, animatedCenter.y + 4.0f, 94.0f, 130.0f };
        DrawRectanglePro(backRight, { backRight.width * 0.5f, backRight.height * 0.5f }, 11.0f + tiltAngle * 0.5f, Color{ 16, 17, 22, 215 });
        DrawPolyLinesEx({ backRight.x, backRight.y }, 4, 16.0f, 11.0f, 1.0f, ColorAlpha(GOLD, 0.25f));
    }
    if (lives >= 2) {
        Rectangle backLeft = { animatedCenter.x - 15.0f, animatedCenter.y + 4.0f, 94.0f, 130.0f };
        DrawRectanglePro(backLeft, { backLeft.width * 0.5f, backLeft.height * 0.5f }, -11.0f + tiltAngle * 0.5f, Color{ 20, 21, 27, 230 });
        DrawPolyLinesEx({ backLeft.x, backLeft.y }, 4, 16.0f, -11.0f, 1.0f, ColorAlpha(GOLD, 0.32f));
    }

    Texture2D cardTex = AssetManager::Instance().GetTexture("card_lives");
    if (cardTex.id > 0) {
        const float frameWidth = 32.0f;
        const float frameHeight = 32.0f;

        int frameIndex = 0;
        if (lives == 2) frameIndex = 1;
        else if (lives == 1) frameIndex = 2;
        else if (lives <= 0) frameIndex = 3;

        Rectangle cardSourceRec = { (float)frameIndex * frameWidth, 0.0f, frameWidth, frameHeight };
        float cardScale = 4.95f;

        Color cardColor = WHITE;
        Vector2 shakeOffset = { 0.0f, 0.0f };

        if (isCritical) {
            DrawCircleV(animatedCenter, 66.0f, ColorAlpha(RED, 0.24f * dangerBeat));
            cardColor = ColorLerp(WHITE, Color{ 255, 135, 135, 255 }, dangerBeat * 0.72f);
        }
        else {
            DrawCircleV(animatedCenter, 62.0f, ColorAlpha(GOLD, 0.08f));
        }

        if (data.cardFlickerTimer > 0.0f) {
            float alpha = (sinf(data.cardFlickerTimer * 40.0f) > 0.0f) ? 0.3f : 1.0f;
            cardColor = ColorAlpha(RED, alpha);
            shakeOffset.x = (float)GetRandomValue(-4, 4);
            shakeOffset.y = (float)GetRandomValue(-4, 4);
            tiltAngle += (float)GetRandomValue(-9, 9);
        }

        Rectangle cardDestRec = {
            animatedCenter.x + shakeOffset.x,
            animatedCenter.y + shakeOffset.y,
            frameWidth * cardScale,
            frameHeight * cardScale
        };
        Vector2 cardOrigin = { cardDestRec.width * 0.5f, cardDestRec.height * 0.5f };

        DrawTexturePro(cardTex, cardSourceRec, cardDestRec, cardOrigin, tiltAngle, cardColor);
    }

    float cardDividerY = 224.0f;
    float cardHalfSpan = 78.0f;
    DrawLineEx({ cardCenter.x - cardHalfSpan, cardDividerY }, { cardCenter.x - 28.0f, cardDividerY }, 1.5f, ColorAlpha(cardAccent, 0.62f));
    DrawLineEx({ cardCenter.x + 28.0f, cardDividerY }, { cardCenter.x + cardHalfSpan, cardDividerY }, 1.5f, ColorAlpha(cardAccent, 0.62f));

    for (int i = 0; i < 3; ++i) {
        float pipX = cardCenter.x + (float)(i - 1) * 16.0f;
        bool pipActive = (i < lives);
        if (pipActive) {
            DrawPoly({ pipX, cardDividerY }, 4, 5.2f, 0.0f, cardAccent);
        }
        else {
            DrawPolyLinesEx({ pipX, cardDividerY }, 4, 4.8f, 0.0f, 1.2f, ColorAlpha(GRAY, 0.45f));
        }
    }

    const char* fateLabel = isCritical ? "LAST DRAW // CRITICAL" : "ACE OF FATE";
    int fateW = MeasureText(fateLabel, 13);
    DrawText(fateLabel, (int)(cardCenter.x - fateW * 0.5f), (int)(cardDividerY + 11.0f), 13, ColorAlpha(cardAccent, 0.92f));

    // =========================================================================
    // 3. ЛІВИЙ НИЖНІЙ ПРОСТІР: ПОКРАЩЕНА ШКАЛА СТАМІНИ В СТИЛІ АР-ДЕКО
    // =========================================================================
    bool isExhausted = data.isExhausted;
    float exhaustPulse = isExhausted ? (0.5f + 0.5f * sinf(shaderTime * 11.0f)) : 0.0f;
    float staminaRatio = (data.maxStamina > 0.0f) ? Clamp(data.currentStamina / data.maxStamina, 0.0f, 1.0f) : 0.0f;

    Color stamAccent = isExhausted
        ? ColorLerp(Color{ 230, 52, 60, 255 }, GOLD, 0.2f * (1.0f - exhaustPulse))
        : GOLD;

    float stamX = 46.0f;
    float stamY = screenH - 82.0f;
    float stamW = 310.0f;
    float stamH = 34.0f;

    const char* stamTitle = isExhausted ? "STAMINA // EXHAUSTED" : "STAMINA";
    DrawText(stamTitle, (int)stamX + 14, (int)(stamY - 22.0f), 13, ColorAlpha(stamAccent, 0.9f));

    const char* stamPct = TextFormat("%d%%", (int)std::round(staminaRatio * 100.0f));
    int pctW = MeasureText(stamPct, 13);
    DrawText(stamPct, (int)(stamX + stamW - pctW - 14.0f), (int)(stamY - 22.0f), 13, ColorAlpha(RAYWHITE, 0.78f));

    DrawPoly({ stamX, stamY + stamH * 0.5f }, 4, 7.0f, 0.0f, ColorAlpha(stamAccent, 0.9f));
    DrawPolyLinesEx({ stamX, stamY + stamH * 0.5f }, 4, 11.0f, 0.0f, 1.2f, ColorAlpha(stamAccent, 0.45f));

    Rectangle outerBar = { stamX + 14.0f, stamY, stamW - 28.0f, stamH };
    Rectangle innerBar = { outerBar.x + 4.0f, outerBar.y + 4.0f, outerBar.width - 8.0f, outerBar.height - 8.0f };

    DrawRectangle((int)outerBar.x + 3, (int)outerBar.y + 4, (int)outerBar.width, (int)outerBar.height, Fade(BLACK, 0.72f));
    DrawRectangleRec(outerBar, Color{ 10, 11, 14, 215 });
    DrawRectangleLinesEx(outerBar, 1.5f, ColorAlpha(stamAccent, 0.82f));
    DrawRectangleLinesEx(innerBar, 1.0f, ColorAlpha(stamAccent, 0.25f));

    int staminaFrame = (int)std::round(staminaRatio * 10.0f);
    std::string staminaTexName = "stamina_" + std::to_string(staminaFrame);
    Texture2D staminaTex = AssetManager::Instance().GetTexture(staminaTexName);

    if (staminaTex.id > 0) {
        Color staminaTint = WHITE;
        float jitterX = 0.0f;
        if (isExhausted) {
            staminaTint = ColorLerp(Color{ 255, 110, 110, 255 }, Color{ 230, 35, 45, 255 }, exhaustPulse);
            jitterX = sinf(shaderTime * 48.0f) * 1.3f;
        }

        Rectangle texSrc = { 0.0f, 0.0f, (float)staminaTex.width, (float)staminaTex.height };
        Rectangle texDst = {
            innerBar.x + 2.0f + jitterX,
            innerBar.y + 2.0f,
            innerBar.width - 4.0f,
            innerBar.height - 4.0f
        };
        DrawTexturePro(staminaTex, texSrc, texDst, { 0.0f, 0.0f }, 0.0f, staminaTint);
    }

    for (int seg = 1; seg < 10; ++seg) {
        float segX = innerBar.x + (innerBar.width / 10.0f) * (float)seg;
        DrawLineEx(
            { segX, innerBar.y + 1.0f },
            { segX, innerBar.y + innerBar.height - 1.0f },
            1.0f,
            Fade(BLACK, 0.55f)
        );
    }

    DrawPoly({ stamX + stamW, stamY + stamH * 0.5f }, 4, 5.5f, 0.0f, ColorAlpha(stamAccent, 0.75f));

    // =========================================================================
    // 4. ПРАВИЙ ВЕРХНІЙ ПРОСТІР: ТЕЛЕМЕТРІЯ КАМЕРИ + РЕЗЕРВ ПІД ВИДИМІСТЬ ГРАВЦЯ
    // =========================================================================
    float rightAnchorX = screenW - 46.0f;
    float topRightY = 44.0f;

    const char* camHeader = TextFormat("CAM-0%d // FLOOR FEED", data.currentCamNumber);
    int camHeaderW = MeasureText(camHeader, 14);
    if (fmodf(shaderTime, 1.4f) < 0.85f) {
        DrawCircle((int)(rightAnchorX - camHeaderW - 14.0f), (int)(topRightY + 7.0f), 4.0f, ColorAlpha(RED, 0.85f));
    }
    DrawText(camHeader, (int)(rightAnchorX - camHeaderW), (int)topRightY, 14, ColorAlpha(RAYWHITE, 0.78f));

    float stealthCenterX = rightAnchorX - 96.0f;
    float stealthCenterY = 104.0f;

    DrawCircleV({ stealthCenterX, stealthCenterY }, 34.0f, Fade(BLACK, 0.35f));
    DrawPolyLinesEx({ stealthCenterX, stealthCenterY }, 4, 26.0f, 0.0f, 1.5f, goldDim);
    DrawPoly({ stealthCenterX, stealthCenterY }, 4, 5.0f, sinf(shaderTime * 1.4f) * 10.0f, ColorAlpha(GOLD, 0.35f));

    float stealthDivY = 142.0f;
    DrawLineEx({ stealthCenterX - 78.0f, stealthDivY }, { stealthCenterX - 11.0f, stealthDivY }, 1.5f, goldLine);
    DrawLineEx({ stealthCenterX + 11.0f, stealthDivY }, { stealthCenterX + 78.0f, stealthDivY }, 1.5f, goldLine);
    DrawPoly({ stealthCenterX, stealthDivY }, 4, 4.5f, 0.0f, goldBright);

    const char* stealthCaption = "VISIBILITY // STANDBY";
    int scW = MeasureText(stealthCaption, 11);
    DrawText(stealthCaption, (int)(stealthCenterX - scW * 0.5f), (int)(stealthDivY + 8.0f), 11, ColorAlpha(LIGHTGRAY, 0.48f));

    // =========================================================================
    // 5. ПРАВИЙ НИЖНІЙ ПРОСТІР: 3 СЛОТИ ІНВЕНТАРЮ КЛЮЧІВ
    // =========================================================================
    DrawInventorySlots(data, screenW, screenH);
}

void GameplayUI::DrawTeleportOverlay(float alpha, int camNumber) const {
    if (alpha <= 0.0f) return;

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, alpha));

    for (int i = 0; i < 6; ++i) {
        int barY = GetRandomValue(0, (int)sh);
        int barH = GetRandomValue(2, 6);
        DrawRectangle(0, barY, (int)sw, barH, ColorAlpha(RAYWHITE, alpha * 0.16f));
    }

    const char* switchText = TextFormat("SIGNAL SWITCH // CAM-0%d", camNumber);
    int fontSize = 22;
    int tw = MeasureText(switchText, fontSize);
    DrawText(
        switchText,
        (int)((sw - tw) * 0.5f) + GetRandomValue(-2, 2),
        (int)(sh * 0.5f - fontSize * 0.5f),
        fontSize,
        ColorAlpha(GOLD, alpha * 0.9f)
    );
}

void GameplayUI::DrawDeathOverlay(float deathTimer, float deathDuration) const {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float progress = Clamp(deathTimer / deathDuration, 0.0f, 1.0f);

    // 1. Ударний багряний спалах у перші 0.35 секунди
    if (progress < 0.18f) {
        float flashT = 1.0f - (progress / 0.18f);
        DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(Color{ 210, 25, 35, 255 }, flashT * 0.55f));
        DrawRectangle(0, 0, (int)sw, (int)sh, ColorAlpha(WHITE, flashT * 0.16f));
    }

    // 2. Плавне багряно-чорне затемнення ігрового поля
    float bgDarken = Clamp(progress / 0.65f, 0.0f, 1.0f);
    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(MAROON, bgDarken * 0.32f));
    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, bgDarken * 0.72f));

    // 3. Багряні перешкоди сигналу CCTV
    float glitchIntensity = sinf(Clamp(progress * 1.25f, 0.0f, 1.0f) * PI);
    if (glitchIntensity > 0.02f) {
        for (int i = 0; i < 9; ++i) {
            int barY = GetRandomValue(0, (int)sh);
            int barH = GetRandomValue(2, 8);
            Color barCol = (i % 2 == 0)
                ? ColorAlpha(Color{ 230, 45, 55, 255 }, glitchIntensity * 0.24f)
                : ColorAlpha(RAYWHITE, glitchIntensity * 0.10f);
            DrawRectangle(0, barY, (int)sw, barH, barCol);
        }
    }

    // 4. Поява розірваного Туза та повідомлення про обрив зв'язку
    if (progress > 0.15f && progress < 0.88f) {
        float cardPhase = Clamp((progress - 0.15f) / 0.22f, 0.0f, 1.0f);
        float cardFadeOut = (progress > 0.72f) ? Clamp(1.0f - (progress - 0.72f) / 0.16f, 0.0f, 1.0f) : 1.0f;
        float alpha = cardPhase * cardFadeOut;

        Vector2 center = { sw * 0.5f, sh * 0.46f };
        float jitterX = (progress < 0.45f) ? (float)GetRandomValue(-4, 4) * (1.0f - cardPhase * 0.5f) : 0.0f;
        float jitterY = (progress < 0.45f) ? (float)GetRandomValue(-4, 4) * (1.0f - cardPhase * 0.5f) : 0.0f;

        DrawPoly({ center.x + jitterX, center.y + jitterY }, 4, 84.0f * cardPhase, 0.0f, ColorAlpha(Color{ 14, 8, 10, 230 }, alpha));
        DrawPolyLinesEx({ center.x + jitterX, center.y + jitterY }, 4, 84.0f * cardPhase, 0.0f, 2.0f, ColorAlpha(Color{ 230, 48, 56, 255 }, alpha * 0.85f));

        Texture2D cardTex = AssetManager::Instance().GetTexture("card_lives");
        if (cardTex.id > 0) {
            const float fw = 32.0f;
            const float fh = 32.0f;
            Rectangle src = { 3.0f * fw, 0.0f, fw, fh };
            float scale = Lerp(6.2f, 4.8f, cardPhase);
            float tilt = sinf(deathTimer * 18.0f) * 3.5f * (1.0f - cardPhase) + 6.0f;

            Rectangle dst = {
                center.x + jitterX,
                center.y + jitterY,
                fw * scale,
                fh * scale
            };
            DrawTexturePro(cardTex, src, dst, { dst.width * 0.5f, dst.height * 0.5f }, tilt, ColorAlpha(WHITE, alpha));
        }

        float divY = center.y + 92.0f;
        DrawLineEx({ center.x - 150.0f, divY }, { center.x - 14.0f, divY }, 1.5f, ColorAlpha(RED, 0.65f * alpha));
        DrawLineEx({ center.x + 14.0f, divY }, { center.x + 150.0f, divY }, 1.5f, ColorAlpha(RED, 0.65f * alpha));
        DrawPoly({ center.x, divY }, 4, 5.0f, 0.0f, ColorAlpha(Color{ 235, 55, 62, 255 }, alpha));

        const char* fatalTitle = "FATAL HAND // SIGNAL TERMINATED";
        int titleW = MeasureText(fatalTitle, 20);
        DrawText(
            fatalTitle,
            (int)(center.x - titleW * 0.5f + jitterX),
            (int)(divY + 14.0f),
            20,
            ColorAlpha(Color{ 245, 75, 82, 255 }, alpha)
        );
    }

    // 5. Фінальне змикання багряно-обсидіанових стулок шлюзу
    if (progress > 0.52f) {
        float closeT = Clamp((progress - 0.52f) / 0.36f, 0.0f, 1.0f);
        float easeClose = (closeT < 0.5f)
            ? 4.0f * closeT * closeT * closeT
            : 1.0f - powf(-2.0f * closeT + 2.0f, 3.0f) * 0.5f;

        float halfW = sw * 0.5f;
        float panelW = halfW * easeClose;

        Color doorBg = Color{ 8, 6, 9, 255 };
        Color crimsonTrim = ColorAlpha(Color{ 225, 48, 56, 255 }, 0.9f);
        Color innerCrimson = ColorAlpha(MAROON, 0.45f);

        DrawRectangle(0, 0, (int)panelW, (int)sh, doorBg);
        if (panelW > 14.0f) {
            DrawRectangleLinesEx({ 12.0f, 12.0f, panelW - 18.0f, sh - 24.0f }, 1.0f, innerCrimson);
            DrawLineEx({ panelW - 2.0f, 0.0f }, { panelW - 2.0f, sh }, 2.5f, crimsonTrim);
            DrawPoly({ panelW - 12.0f, 24.0f }, 4, 4.5f, 0.0f, crimsonTrim);
            DrawPoly({ panelW - 12.0f, sh - 24.0f }, 4, 4.5f, 0.0f, crimsonTrim);
        }

        DrawRectangle((int)(sw - panelW), 0, (int)panelW, (int)sh, doorBg);
        if (panelW > 14.0f) {
            DrawRectangleLinesEx({ sw - panelW + 6.0f, 12.0f, panelW - 18.0f, sh - 24.0f }, 1.0f, innerCrimson);
            DrawLineEx({ sw - panelW + 2.0f, 0.0f }, { sw - panelW + 2.0f, sh }, 2.5f, crimsonTrim);
            DrawPoly({ sw - panelW + 12.0f, 24.0f }, 4, 4.5f, 0.0f, crimsonTrim);
            DrawPoly({ sw - panelW + 12.0f, sh - 24.0f }, 4, 4.5f, 0.0f, crimsonTrim);
        }

        for (int y = 0; y < (int)sh; y += 4) {
            DrawRectangle(0, y, (int)sw, 1, Fade(WHITE, 0.025f * easeClose));
        }

        if (closeT > 0.72f) {
            float lockAlpha = Clamp((closeT - 0.72f) / 0.28f, 0.0f, 1.0f);
            Vector2 center = { sw * 0.5f, sh * 0.5f };
            float lockRot = Lerp(45.0f, 0.0f, lockAlpha);

            DrawPoly(center, 4, 24.0f, lockRot, Color{ 14, 8, 10, 255 });
            DrawPolyLinesEx(center, 4, 24.0f, lockRot, 2.0f, ColorAlpha(crimsonTrim, lockAlpha));
            DrawPoly(center, 4, 8.0f, lockRot, ColorAlpha(crimsonTrim, lockAlpha));
        }
    }
}