#include "MainMenuUI.h"
#include <cmath>

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
    int titleY = (int)(screenH * 0.23f - smoothedMouseOffset.y * 3.5f);

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

    int fontSize = 25;
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

        const char* statusText = isTransitioning ? "SIGNAL: OVERRIDE..." : "SIGNAL: UNSTABLE";
        Color statusColor = isTransitioning ? ColorAlpha(RED, 0.75f * recAlpha) : ColorAlpha(LIGHTGRAY, 0.38f * recAlpha);
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

    // Легкі перешкоди сигналу на фоні перед змиканням
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

    // Ліва стулка
    Rectangle leftDoor = { 0.0f, 0.0f, panelW, screenH };
    DrawRectangleRec(leftDoor, panelBg);
    if (panelW > 14.0f) {
        Rectangle leftInner = { 12.0f, 12.0f, panelW - 18.0f, screenH - 24.0f };
        DrawRectangleLinesEx(leftInner, 1.0f, innerTrim);
        DrawLineEx({ panelW - 2.0f, 0.0f }, { panelW - 2.0f, screenH }, 2.5f, goldTrim);
        DrawPoly({ panelW - 12.0f, 24.0f }, 4, 4.5f, 0.0f, goldTrim);
        DrawPoly({ panelW - 12.0f, screenH - 24.0f }, 4, 4.5f, 0.0f, goldTrim);
    }

    // Права стулка
    Rectangle rightDoor = { screenW - panelW, 0.0f, panelW, screenH };
    DrawRectangleRec(rightDoor, panelBg);
    if (panelW > 14.0f) {
        Rectangle rightInner = { screenW - panelW + 6.0f, 12.0f, panelW - 18.0f, screenH - 24.0f };
        DrawRectangleLinesEx(rightInner, 1.0f, innerTrim);
        DrawLineEx({ screenW - panelW + 2.0f, 0.0f }, { screenW - panelW + 2.0f, screenH }, 2.5f, goldTrim);
        DrawPoly({ screenW - panelW + 12.0f, 24.0f }, 4, 4.5f, 0.0f, goldTrim);
        DrawPoly({ screenW - panelW + 12.0f, screenH - 24.0f }, 4, 4.5f, 0.0f, goldTrim);
    }

    // Сканлайни поверх закритих стулок
    for (int y = 0; y < (int)screenH; y += 4) {
        DrawRectangle(0, y, (int)screenW, 1, Fade(WHITE, 0.025f * easeClose));
    }

    // Центральний замок-ромб та термінал перемикання камери після змикання
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