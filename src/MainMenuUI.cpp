#include "MainMenuUI.h"
#include <cmath>

namespace {
    void DrawModalKeyCap(float x, float y, float w, float h, const char* label, float alpha) {
        Rectangle baseRect = { x, y + 3.0f, w, h };
        Rectangle topRect = { x, y, w, h };

        DrawRectangleRounded(baseRect, 0.22f, 6, ColorAlpha(Color{ 95, 72, 26, 255 }, alpha));
        DrawRectangleRounded(topRect, 0.22f, 6, ColorAlpha(Color{ 18, 20, 26, 255 }, alpha));
        DrawRectangleRoundedLinesEx(topRect, 0.22f, 6, 1.4f, ColorAlpha(GOLD, 0.90f * alpha));

        int fontSize = 14;
        int tw = MeasureText(label, fontSize);
        DrawText(
            label,
            (int)(x + (w - (float)tw) * 0.5f),
            (int)(y + (h - (float)fontSize) * 0.5f),
            fontSize,
            ColorAlpha(Color{ 255, 234, 155, 255 }, alpha)
        );
    }
}

void MainMenuUI::DrawNeonTitle(float screenW, float screenH, float shaderTime, Vector2 smoothedMouseOffset) const {
    std::string titleStr = "_PR0JECT_D1CE";
    float cycle = fmodf(shaderTime, 4.2f);
    float tubeBrightness = 1.0f;

    if (cycle > 3.55f && cycle < 3.72f) {
        tubeBrightness = 0.45f;
        titleStr[3] = (fmodf(shaderTime * 30.0f, 2.0f) > 1.0f) ? '7' : '#';
    }
    else if (cycle > 3.78f && cycle < 3.88f) {
        tubeBrightness = 0.65f;
        titleStr[10] = '7';
    }

    int fontSize = 62;
    int textWidth = MeasureText(titleStr.c_str(), fontSize);
    int titleX = (int)((screenW - textWidth) * 0.5f - smoothedMouseOffset.x * 5.0f);
    int titleY = (int)(screenH * 0.21f - smoothedMouseOffset.y * 3.5f);

    float breathe = 0.88f + 0.12f * sinf(shaderTime * 3.2f);
    float glowAlpha = breathe * tubeBrightness;

    DrawRectangleGradientH(titleX - 60, titleY - 14, textWidth / 2 + 60, fontSize + 28, Fade(BLACK, 0.0f), Fade(BLACK, 0.52f));
    DrawRectangleGradientH(titleX + textWidth / 2, titleY - 14, textWidth / 2 + 60, fontSize + 28, Fade(BLACK, 0.52f), Fade(BLACK, 0.0f));

    DrawText(titleStr.c_str(), titleX - 3, titleY + 2, fontSize, ColorAlpha(RED, 0.35f * glowAlpha));
    DrawText(titleStr.c_str(), titleX + 3, titleY - 1, fontSize, ColorAlpha(SKYBLUE, 0.18f * glowAlpha));

    Color outerGlow = ColorAlpha(ORANGE, 0.22f * glowAlpha);
    Color innerGlow = ColorAlpha(GOLD, 0.42f * glowAlpha);

    DrawText(titleStr.c_str(), titleX - 2, titleY, fontSize, outerGlow);
    DrawText(titleStr.c_str(), titleX + 2, titleY, fontSize, outerGlow);
    DrawText(titleStr.c_str(), titleX, titleY - 2, fontSize, outerGlow);
    DrawText(titleStr.c_str(), titleX, titleY + 2, fontSize, outerGlow);

    DrawText(titleStr.c_str(), titleX - 1, titleY - 1, fontSize, innerGlow);
    DrawText(titleStr.c_str(), titleX + 1, titleY + 1, fontSize, innerGlow);

    Color coreColor = (tubeBrightness < 0.8f)
        ? Color{ 225, 185, 75, 235 }
    : Color{ 255, 226, 110, 255 };
    DrawText(titleStr.c_str(), titleX, titleY, fontSize, coreColor);

    float centerX = titleX + textWidth * 0.5f;
    float lineY = (float)(titleY + fontSize + 14);
    float halfSpan = textWidth * 0.44f;

    Color lineColor = ColorAlpha(GOLD, 0.45f * glowAlpha);
    DrawLineEx({ centerX - halfSpan, lineY }, { centerX - 14.0f, lineY }, 1.5f, lineColor);
    DrawLineEx({ centerX + 14.0f, lineY }, { centerX + halfSpan, lineY }, 1.5f, lineColor);
    DrawPoly({ centerX, lineY }, 4, 5.5f, 0.0f, ColorAlpha(GOLD, 0.85f * glowAlpha));
}

void MainMenuUI::DrawButton(const MenuButton& btn, float shaderTime) const {
    float p = btn.hoverProgress;

    float expandX = p * 14.0f;
    float expandY = p * 2.0f;

    float jitterX = 0.0f;
    float jitterY = 0.0f;
    if (btn.isDanger && p > 0.05f) {
        jitterX = sinf(shaderTime * 58.0f) * 1.4f * p;
        jitterY = cosf(shaderTime * 47.0f) * 0.9f * p;
    }

    Rectangle rect = {
        btn.bounds.x - expandX + jitterX,
        btn.bounds.y - expandY + jitterY,
        btn.bounds.width + expandX * 2.0f,
        btn.bounds.height + expandY * 2.0f
    };

    Color accentColor = btn.isDanger ? Color{ 225, 48, 56, 255 } : GOLD;
    Color darkFillBase = btn.isDanger ? Color{ 16, 10, 12, 215 } : Color{ 12, 13, 16, 215 };
    Color hoverTint = btn.isDanger ? ColorAlpha(MAROON, 0.36f * p) : ColorAlpha(GOLD, 0.16f * p);

    DrawRectangle((int)rect.x + 4, (int)rect.y + 5, (int)rect.width, (int)rect.height, Fade(BLACK, 0.65f));
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
        float sweepPhase = fmodf(shaderTime * 1.35f, 1.8f) - 0.4f;
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

void MainMenuUI::DrawRulesModal(float screenW, float screenH, float openAnim, float shaderTime, const MenuButton& closeBtn) const {
    if (openAnim <= 0.01f) return;

    float ease = 1.0f - powf(1.0f - openAnim, 3.0f);

    // Затемнення фону поза панеллю
    DrawRectangle(0, 0, (int)screenW, (int)screenH, Fade(BLACK, 0.80f * ease));

    float modalW = 920.0f;
    float modalH = 540.0f;
    float modalX = (screenW - modalW) * 0.5f;
    float modalY = (screenH - modalH) * 0.5f + (1.0f - ease) * 18.0f;

    Rectangle panel = { modalX, modalY, modalW, modalH };
    Rectangle inner = { modalX + 8.0f, modalY + 8.0f, modalW - 16.0f, modalH - 16.0f };

    // Тінь + обсидіановий корпус панелі
    DrawRectangle((int)panel.x + 8, (int)panel.y + 10, (int)panel.width, (int)panel.height, Fade(BLACK, 0.85f * ease));
    DrawRectangleRec(panel, ColorAlpha(Color{ 11, 12, 16, 255 }, 0.96f * ease));
    DrawRectangleLinesEx(panel, 2.0f, ColorAlpha(GOLD, 0.88f * ease));
    DrawRectangleLinesEx(inner, 1.0f, ColorAlpha(GOLD, 0.28f * ease));

    // Кутові ромби Art Deco
    DrawPoly({ inner.x, inner.y }, 4, 5.0f, 0.0f, ColorAlpha(GOLD, ease));
    DrawPoly({ inner.x + inner.width, inner.y }, 4, 5.0f, 0.0f, ColorAlpha(GOLD, ease));
    DrawPoly({ inner.x, inner.y + inner.height }, 4, 5.0f, 0.0f, ColorAlpha(GOLD, ease));
    DrawPoly({ inner.x + inner.width, inner.y + inner.height }, 4, 5.0f, 0.0f, ColorAlpha(GOLD, ease));

    // Заголовок модального вікна
    const char* header = "HOUSE RULES & CONTROLS";
    int headerSize = 30;
    int headerW = MeasureText(header, headerSize);
    float centerX = modalX + modalW * 0.5f;
    int headerY = (int)(modalY + 26.0f);

    DrawText(header, (int)(centerX - headerW * 0.5f) + 2, headerY + 2, headerSize, Fade(BLACK, 0.85f * ease));
    DrawText(header, (int)(centerX - headerW * 0.5f), headerY, headerSize, ColorAlpha(Color{ 255, 226, 110, 255 }, ease));

    float divY = (float)(headerY + headerSize + 12);
    DrawLineEx({ modalX + 60.0f, divY }, { centerX - 16.0f, divY }, 1.5f, ColorAlpha(GOLD, 0.55f * ease));
    DrawLineEx({ centerX + 16.0f, divY }, { modalX + modalW - 60.0f, divY }, 1.5f, ColorAlpha(GOLD, 0.55f * ease));
    DrawPoly({ centerX, divY }, 4, 5.5f, 0.0f, ColorAlpha(GOLD, ease));

    // Вертикальний роздільник між колонками
    float colTop = divY + 22.0f;
    float colBot = modalY + modalH - 96.0f;
    float colMid = (colTop + colBot) * 0.5f;
    DrawLineEx({ centerX, colTop }, { centerX, colMid - 10.0f }, 1.2f, ColorAlpha(GOLD, 0.35f * ease));
    DrawLineEx({ centerX, colMid + 10.0f }, { centerX, colBot }, 1.2f, ColorAlpha(GOLD, 0.35f * ease));
    DrawPoly({ centerX, colMid }, 4, 4.5f, 0.0f, ColorAlpha(GOLD, 0.85f * ease));

    // =========================================================================
    // ЛІВА КОЛОНКА: CONTROLS
    // =========================================================================
    float leftColX = modalX + 42.0f;
    DrawText("CONTROLS", (int)leftColX, (int)colTop, 20, ColorAlpha(GOLD, 0.92f * ease));

    struct ControlRow {
        const char* key;
        float keyW;
        const char* title;
        const char* desc;
    };

    const ControlRow controls[] = {
        { "WASD",  68.0f, "MOVE",      "Navigate the casino floor quietly" },
        { "SHIFT", 68.0f, "SPRINT",    "Run fast; drains stamina & makes noise" },
        { "E",     68.0f, "INTERACT",  "Pick locks, open safes & exit gate" },
        { "ESC",   68.0f, "PAUSE",     "Freeze feed, restart or return to menu" }
    };

    float rowY = colTop + 42.0f;
    for (int i = 0; i < 4; ++i) {
        DrawModalKeyCap(leftColX, rowY, controls[i].keyW, 30.0f, controls[i].key, ease);

        float textStartX = leftColX + controls[i].keyW + 16.0f;
        DrawText(controls[i].title, (int)textStartX, (int)(rowY - 2.0f), 16, ColorAlpha(RAYWHITE, 0.95f * ease));
        DrawText(controls[i].desc, (int)textStartX, (int)(rowY + 17.0f), 13, ColorAlpha(LIGHTGRAY, 0.72f * ease));

        rowY += 68.0f;
    }

    // =========================================================================
    // ПРАВА КОЛОНКА: CASINO RULES
    // =========================================================================
    float rightColX = centerX + 34.0f;
    DrawText("CASINO RULES", (int)rightColX, (int)colTop, 20, ColorAlpha(GOLD, 0.92f * ease));

    struct RuleItem {
        Color bulletCol;
        const char* title;
        const char* line1;
        const char* line2;
    };

    const RuleItem rules[] = {
        {
            GOLD,
            "RIGGED DICE LOCKPICKING (2D6)",
            "Lockers require roll >= 7. Guard Safe requires >= 9.",
            "Failed rolls make noise — try again if clear."
        },
        {
            Color{ 110, 225, 175, 255 },
            "KEY PROGRESSION CHAIN",
            "1. Outer Lockers -> Guard Room Key",
            "2. Guard Room -> Safe Card -> Safe -> Master Key"
        },
        {
            Color{ 235, 65, 72, 255 },
            "GUARD PATROL & SENSES",
            "Avoid flashlight cone & sprinting near the guard.",
            "3 Aces of Fate (lives). Lose all 3 — house wins."
        }
    };

    float ruleY = colTop + 40.0f;
    for (int i = 0; i < 3; ++i) {
        DrawPoly({ rightColX + 6.0f, ruleY + 8.0f }, 4, 4.5f, 0.0f, ColorAlpha(rules[i].bulletCol, ease));
        DrawText(rules[i].title, (int)(rightColX + 20.0f), (int)ruleY, 15, ColorAlpha(rules[i].bulletCol, 0.95f * ease));
        DrawText(rules[i].line1, (int)(rightColX + 20.0f), (int)(ruleY + 21.0f), 13, ColorAlpha(RAYWHITE, 0.82f * ease));
        DrawText(rules[i].line2, (int)(rightColX + 20.0f), (int)(ruleY + 39.0f), 13, ColorAlpha(LIGHTGRAY, 0.68f * ease));
        ruleY += 86.0f;
    }

    // Кнопка закриття
    DrawButton(closeBtn, shaderTime);
}

void MainMenuUI::DrawCctvOverlay(float screenW, float screenH, float shaderTime, float transitionProgress, bool isTransitioning) const {
    float margin = 24.0f;
    float armLen = 28.0f;
    float thick = 1.5f;
    Color hudColor = ColorAlpha(Color{ 170, 195, 178, 255 }, 0.32f * (1.0f - transitionProgress));

    DrawLineEx({ margin, margin }, { margin + armLen, margin }, thick, hudColor);
    DrawLineEx({ margin, margin }, { margin, margin + armLen }, thick, hudColor);

    DrawLineEx({ screenW - margin - armLen, margin }, { screenW - margin, margin }, thick, hudColor);
    DrawLineEx({ screenW - margin, margin }, { screenW - margin, margin + armLen }, thick, hudColor);

    DrawLineEx({ margin, screenH - margin }, { margin + armLen, screenH - margin }, thick, hudColor);
    DrawLineEx({ margin, screenH - margin - armLen }, { margin, screenH - margin }, thick, hudColor);

    DrawLineEx({ screenW - margin - armLen, screenH - margin }, { screenW - margin, screenH - margin }, thick, hudColor);
    DrawLineEx({ screenW - margin, screenH - margin - armLen }, { screenW - margin, screenH - margin }, thick, hudColor);

    float recAlpha = 1.0f - transitionProgress;
    if (recAlpha > 0.05f) {
        bool recBlink = (fmodf(shaderTime, 1.4f) < 0.85f);
        if (recBlink) {
            DrawCircle((int)(margin + 18.0f), (int)(margin + 18.0f), 4.5f, ColorAlpha(RED, 0.85f * recAlpha));
        }
        DrawText("REC", (int)(margin + 30.0f), (int)(margin + 11.0f), 15, ColorAlpha(RAYWHITE, 0.65f * recAlpha));
        DrawText("CAM-04 // CASINO FLOOR", (int)(margin + 12.0f), (int)(margin + 32.0f), 13, ColorAlpha(LIGHTGRAY, 0.42f * recAlpha));

        const char* statusText = isTransitioning ? "SIGNAL: OVERRIDE..." : "[H] HOUSE RULES   //   SIGNAL: UNSTABLE";
        Color statusColor = isTransitioning ? ColorAlpha(RED, 0.75f * recAlpha) : ColorAlpha(LIGHTGRAY, 0.45f * recAlpha);
        int statusW = MeasureText(statusText, 13);
        DrawText(statusText, (int)(screenW - margin - statusW - 12.0f), (int)(screenH - margin - 24.0f), 13, statusColor);
    }
}

void MainMenuUI::DrawTransitionDoors(float screenW, float screenH, float progress) const {
    float closeT = Clamp(progress / 0.72f, 0.0f, 1.0f);
    float easeClose = (closeT < 0.5f)
        ? 4.0f * closeT * closeT * closeT
        : 1.0f - powf(-2.0f * closeT + 2.0f, 3.0f) * 0.5f;

    float halfW = screenW * 0.5f;
    float panelW = halfW * easeClose;

    if (progress < 0.75f) {
        for (int i = 0; i < 5; ++i) {
            int barY = GetRandomValue(0, (int)screenH);
            int barH = GetRandomValue(2, 5);
            DrawRectangle(0, barY, (int)screenW, barH, ColorAlpha(GOLD, 0.08f * (1.0f - progress)));
        }
    }

    Color panelBg = Color{ 9, 10, 13, 252 };
    Color goldTrim = ColorAlpha(GOLD, 0.85f);
    Color innerTrim = ColorAlpha(GOLD, 0.25f);

    Rectangle leftDoor = { 0.0f, 0.0f, panelW, screenH };
    DrawRectangleRec(leftDoor, panelBg);
    if (panelW > 14.0f) {
        Rectangle leftInner = { 12.0f, 12.0f, panelW - 18.0f, screenH - 24.0f };
        DrawRectangleLinesEx(leftInner, 1.0f, innerTrim);
        DrawLineEx({ panelW - 2.0f, 0.0f }, { panelW - 2.0f, screenH }, 2.5f, goldTrim);
        DrawPoly({ panelW - 12.0f, 24.0f }, 4, 4.5f, 0.0f, goldTrim);
        DrawPoly({ panelW - 12.0f, screenH - 24.0f }, 4, 4.5f, 0.0f, goldTrim);
    }

    Rectangle rightDoor = { screenW - panelW, 0.0f, panelW, screenH };
    DrawRectangleRec(rightDoor, panelBg);
    if (panelW > 14.0f) {
        Rectangle rightInner = { screenW - panelW + 6.0f, 12.0f, panelW - 18.0f, screenH - 24.0f };
        DrawRectangleLinesEx(rightInner, 1.0f, innerTrim);
        DrawLineEx({ screenW - panelW + 2.0f, 0.0f }, { screenW - panelW + 2.0f, screenH }, 2.5f, goldTrim);
        DrawPoly({ screenW - panelW + 12.0f, 24.0f }, 4, 4.5f, 0.0f, goldTrim);
        DrawPoly({ screenW - panelW + 12.0f, screenH - 24.0f }, 4, 4.5f, 0.0f, goldTrim);
    }

    for (int y = 0; y < (int)screenH; y += 4) {
        DrawRectangle(0, y, (int)screenW, 1, Fade(WHITE, 0.025f * easeClose));
    }

    if (closeT > 0.65f) {
        float lockAlpha = Clamp((closeT - 0.65f) / 0.35f, 0.0f, 1.0f);
        float fadeOutEnd = (progress > 0.82f) ? Clamp(1.0f - (progress - 0.82f) / 0.18f, 0.0f, 1.0f) : 1.0f;
        float uiAlpha = lockAlpha * fadeOutEnd;

        Vector2 center = { screenW * 0.5f, screenH * 0.5f };
        float lockRot = Lerp(45.0f, 0.0f, lockAlpha);

        DrawPoly(center, 4, 24.0f, lockRot, Color{ 12, 13, 16, 255 });
        DrawPolyLinesEx(center, 4, 24.0f, lockRot, 2.0f, ColorAlpha(GOLD, uiAlpha));
        DrawPoly(center, 4, 8.0f, lockRot, ColorAlpha(GOLD, uiAlpha));

        const char* topLabel = "ACCESS GRANTED // OVERRIDE";
        int topSize = 16;
        int topW = MeasureText(topLabel, topSize);
        DrawText(topLabel, (int)(center.x - topW * 0.5f), (int)(center.y - 62.0f), topSize, ColorAlpha(GOLD, 0.9f * uiAlpha));

        const char* botLabel = "ROUTING FEED TO CAM-01 [CASINO FLOOR]...";
        int botSize = 14;
        int botW = MeasureText(botLabel, botSize);
        DrawText(botLabel, (int)(center.x - botW * 0.5f), (int)(center.y + 44.0f), botSize, ColorAlpha(LIGHTGRAY, 0.78f * uiAlpha));
    }
}