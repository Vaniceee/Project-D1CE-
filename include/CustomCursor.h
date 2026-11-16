#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>

class CustomCursor {
public:
    static void Update(float dt, bool isHoveringInteractive) {
        if (!IsCursorHidden()) {
            HideCursor();
        }

        s_time += dt;

        float targetHover = isHoveringInteractive ? 1.0f : 0.0f;
        float blend = 1.0f - expf(-14.0f * dt);
        s_hoverAnim = Lerp(s_hoverAnim, targetHover, blend);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_clickPulse = 1.0f;
        }
        if (s_clickPulse > 0.0f) {
            s_clickPulse -= dt * 4.2f;
            if (s_clickPulse < 0.0f) s_clickPulse = 0.0f;
        }
    }

    static void Draw(bool isDanger = false) {
        Vector2 pos = GetMousePosition();
        Color accent = isDanger ? Color{ 235, 58, 68, 255 } : GOLD;

        // 1. Хвиля розширення при кліку
        if (s_clickPulse > 0.01f) {
            float burstRadius = 10.0f + (1.0f - s_clickPulse) * 22.0f;
            DrawCircleLinesV(pos, burstRadius, ColorAlpha(accent, s_clickPulse * 0.75f));
        }

        // 2. Зовнішнє кільце прицілу (стискається при наведенні на кнопку)
        float ringRadius = Lerp(14.0f, 9.5f, s_hoverAnim) - s_clickPulse * 2.5f;
        DrawCircleLinesV(pos, ringRadius, ColorAlpha(accent, 0.35f + 0.45f * s_hoverAnim));

        // 3. Короткі візирні риски по осях
        float tickInner = ringRadius + 2.0f;
        float tickOuter = tickInner + 4.0f;
        Color tickCol = ColorAlpha(RAYWHITE, 0.45f + 0.35f * s_hoverAnim);
        DrawLineEx({ pos.x - tickOuter, pos.y }, { pos.x - tickInner, pos.y }, 1.2f, tickCol);
        DrawLineEx({ pos.x + tickInner, pos.y }, { pos.x + tickOuter, pos.y }, 1.2f, tickCol);
        DrawLineEx({ pos.x, pos.y - tickOuter }, { pos.x, pos.y - tickInner }, 1.2f, tickCol);
        DrawLineEx({ pos.x, pos.y + tickInner }, { pos.x, pos.y + tickOuter }, 1.2f, tickCol);

        // 4. Центральний ромб казино (обертається при наведенні)
        float diamondRadius = Lerp(4.2f, 5.4f, s_hoverAnim);
        float angle = s_hoverAnim * 45.0f + sinf(s_time * 2.5f) * 4.0f;
        DrawPoly({ pos.x + 1.0f, pos.y + 1.0f }, 4, diamondRadius, angle, Fade(BLACK, 0.75f));
        DrawPoly(pos, 4, diamondRadius, angle, accent);
    }

private:
    static inline float s_time = 0.0f;
    static inline float s_hoverAnim = 0.0f;
    static inline float s_clickPulse = 0.0f;
};
