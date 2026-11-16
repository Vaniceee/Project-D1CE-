#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform float transitionProgress; // 0.0 у меню -> 1.0 при старті гри
uniform vec2 mouseOffset;         // Нормалізований зсув миші від центру (-1.0 .. 1.0)

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

float hash13(vec3 p3)
{
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        value += amp * valueNoise(p);
        p = p * 2.05 + vec2(17.1, 31.7);
        amp *= 0.5;
    }
    return value;
}

float DustLayer(vec2 uv, float gridScale, float speed, float sizeRadius, float seed)
{
    vec2 aspectUV = vec2(uv.x * (resolution.x / max(resolution.y, 1.0)), uv.y);
    vec2 drift = vec2(
        -time * speed * 0.45 + sin(time * 0.5 + seed) * 0.03,
        -time * speed * 0.25 + cos(time * 0.7 + seed) * 0.03
    );
    vec2 p = (aspectUV + drift) * gridScale + seed * 13.7;

    vec2 cell = floor(p);
    vec2 f = fract(p) - 0.5;

    vec2 rnd = hash22(cell);
    if (rnd.x < 0.775) return 0.0;

    vec2 wobble = vec2(
        sin(time * (1.2 + rnd.y * 1.5) + rnd.x * 6.28),
        cos(time * (1.0 + rnd.x * 1.4) + rnd.y * 6.28)
    ) * 0.28;

    float d = length(f - wobble);
    float mote = 1.0 - smoothstep(sizeRadius * 0.45, sizeRadius, d);
    float twinkle = 0.45 + 0.55 * sin(time * (2.5 + rnd.y * 3.0) + rnd.x * 12.0);

    return mote * twinkle;
}

void main()
{
    // 0. ПАРАЛАКС ВІД РУХУ МИШІ + ЗАПАС ПО КРАЯХ
    vec2 clampedMouse = clamp(mouseOffset, vec2(-1.0), vec2(1.0));
    float zoom = 0.962;
    vec2 zoomedUV = (fragTexCoord - 0.5) * zoom + 0.5;

    float rouletteDepth = smoothstep(0.45, 0.85, fragTexCoord.x) * smoothstep(0.50, 0.95, fragTexCoord.y);
    float ghostFigureDepth = smoothstep(0.76, 0.96, fragTexCoord.x) * smoothstep(0.18, 0.85, fragTexCoord.y);
    float slotsDepth = (1.0 - smoothstep(0.05, 0.34, fragTexCoord.x)) * 0.35;
    float depthFactor = clamp(0.45 + rouletteDepth * 0.75 + ghostFigureDepth * 0.65 + slotsDepth, 0.45, 1.35);

    vec2 parallaxShift = clampedMouse * vec2(0.014, 0.010) * depthFactor;
    vec2 uv = clamp(zoomedUV + parallaxShift, vec2(0.001), vec2(0.999));
    vec2 atmosphereUV = zoomedUV + clampedMouse * vec2(0.022, 0.015);

    // 1. МАСКИ ЕКРАНІВ ТА СИЛУЕТІВ ДЛЯ ПАСХАЛОК
    float slotZone = smoothstep(0.02, 0.04, uv.x) * (1.0 - smoothstep(0.27, 0.29, uv.x))
                   * smoothstep(0.28, 0.305, uv.y) * (1.0 - smoothstep(0.515, 0.54, uv.y));

    float monitorZone = smoothstep(0.575, 0.595, uv.x) * (1.0 - smoothstep(0.68, 0.70, uv.x))
                      * smoothstep(0.015, 0.035, uv.y) * (1.0 - smoothstep(0.13, 0.15, uv.y));

    // Зона тіньового монстра праворуч біля прожектора
    float shadowMonsterZone = smoothstep(0.73, 0.78, uv.x) * (1.0 - smoothstep(0.89, 0.94, uv.x))
                            * smoothstep(0.24, 0.30, uv.y) * (1.0 - smoothstep(0.62, 0.68, uv.y));

    float screenDevicesMask = clamp(slotZone + monitorZone, 0.0, 1.0);

    // Таймери пасхалок (активуються після 15-ї секунди перебування в меню)
    float idleUnlocked = step(15.0, time) * (1.0 - transitionProgress);
    float scareCycle = mod(max(time - 15.0, 0.0), 14.0);
    // Спалах білого шуму на моніторі (0.0 .. 0.45 с циклу)
    float monitorStaticBurst = idleUnlocked * smoothstep(0.0, 0.05, scareCycle) * (1.0 - smoothstep(0.32, 0.48, scareCycle));
    // Різкий «ривок» силуету монстра у глітчі (6.5 .. 6.82 с циклу)
    float monsterTwitchBurst = idleUnlocked * smoothstep(6.50, 6.54, scareCycle) * (1.0 - smoothstep(6.74, 6.84, scareCycle));

    // 2. ГЛІТЧІ АВТОМАТІВ, МОНІТОРА ТА ПАСХАЛКИ МОНСТРА
    float burstClock = sin(time * 2.4) * sin(time * 5.9 + 1.4);
    float glitchBurst = smoothstep(0.68, 0.96, burstClock);
    glitchBurst = max(glitchBurst, max(transitionProgress * 0.95, monitorStaticBurst));

    float lineRow = floor(uv.y * 220.0);
    float tick = floor(time * 18.0);
    float rowRand = hash12(vec2(lineRow, tick));
    float activeLine = step(0.82 - glitchBurst * 0.25, rowRand);

    float localJitter = 0.0042 * screenDevicesMask * glitchBurst * activeLine;
    float globalTransitionJitter = 0.0045 * transitionProgress * activeLine;
    float jitterStrength = localJitter + globalTransitionJitter;

    float shiftX = (hash12(vec2(lineRow, tick + 9.0)) - 0.5) * 2.0 * jitterStrength;

    // Різкий зсув тіньового силуету монстра під час пасхалки
    float monsterStep = (hash12(vec2(floor(time * 32.0), 3.1)) > 0.45) ? 1.0 : -0.6;
    shiftX += shadowMonsterZone * monsterTwitchBurst * 0.022 * monsterStep;

    vec2 glitchedUV = clamp(vec2(uv.x + shiftX, uv.y), vec2(0.001), vec2(0.999));

    float chromaOffset = (0.0024 * screenDevicesMask * glitchBurst)
                       + (transitionProgress * 0.006 * length(uv - 0.5))
                       + (shadowMonsterZone * monsterTwitchBurst * 0.012);

    float r = texture(texture0, clamp(glitchedUV + vec2(chromaOffset, 0.0), vec2(0.001), vec2(0.999))).r;
    vec4 baseSample = texture(texture0, glitchedUV);
    float b = texture(texture0, clamp(glitchedUV - vec2(chromaOffset, 0.0), vec2(0.001), vec2(0.999))).b;
    vec3 col = vec3(r, baseSample.g, b);

    float screenFlicker = 1.0 + sin(time * 37.0 + uv.y * 40.0) * 0.045 * screenDevicesMask
                            - glitchBurst * 0.15 * screenDevicesMask * step(0.5, fract(time * 13.0));
    float scanline = 1.0 - 0.10 * screenDevicesMask * (0.5 + 0.5 * sin(uv.y * resolution.y * 1.5 - time * 12.0));
    col *= screenFlicker * scanline;

    // Білий сніг/перешкоди всередині монітора під час спалаху
    if (monitorStaticBurst > 0.01 && monitorZone > 0.01) {
        float tvSnow = hash13(vec3(floor(uv * resolution * 0.5), floor(time * 45.0)));
        vec3 snowCol = vec3(0.75, 0.92, 0.80) * (0.35 + 0.65 * tvSnow);
        col = mix(col, snowCol, monitorZone * monitorStaticBurst * 0.85);
    }

    // 3. ПУЛЬСАЦІЯ ЧЕРВОНИХ ОЧЕЙ І НЕОНУ
    float redDominance = clamp((col.r - max(col.g, col.b) * 1.35) * 3.0, 0.0, 1.0);
    float eyePulse = 0.88 + 0.24 * sin(time * 2.6) + 0.08 * sin(time * 11.0) + monsterTwitchBurst * 0.65;
    col.r = clamp(col.r * mix(1.0, eyePulse, redDominance), 0.0, 1.0);

    // 4. КЛУБИ СИГАРНОГО ДИМУ (FBM)
    vec2 smokeUV1 = atmosphereUV * vec2(2.4, 1.7) + vec2(time * 0.032, -time * 0.018);
    vec2 smokeUV2 = atmosphereUV * vec2(4.1, 3.0) + vec2(-time * 0.022, -time * 0.030);
    float smokeRaw = fbm(smokeUV1) * 0.62 + fbm(smokeUV2) * 0.38;
    float smokeBillows = smoothstep(0.34, 0.76, smokeRaw);

    float floorAndMidHaze = smoothstep(0.20, 0.92, uv.y);
    vec3 smokeColor = vec3(0.27, 0.33, 0.31);
    col = mix(col, col + smokeColor * smokeBillows, 0.36 * (0.45 + 0.55 * floorAndMidHaze));

    // 5. КОНУС ПРОЖЕКТОРА ПРАВОРУЧ ТА ДРІБНІ ПИЛИНКИ В НЬОМУ
    vec2 aspect = vec2(resolution.x / max(resolution.y, 1.0), 1.0);
    vec2 spotOrigin = vec2(0.885, 0.125) * aspect;
    vec2 pixelAspect = uv * aspect;
    vec2 toPixel = pixelAspect - spotOrigin;
    float spotDist = length(toPixel);
    vec2 spotDir = normalize(vec2(-0.83, 0.55));
    float cosAngle = (spotDist > 0.0001) ? dot(normalize(toPixel), spotDir) : 1.0;

    float beamWidth = smoothstep(0.935, 0.985, cosAngle);
    float beamLength = smoothstep(0.02, 0.08, spotDist) * (1.0 - smoothstep(0.45, 0.72, spotDist));
    float spotCone = beamWidth * beamLength;

    col += vec3(0.16, 0.22, 0.17) * smokeBillows * spotCone * 0.85;

    float dustBig = DustLayer(atmosphereUV, 16.0, 0.018, 0.108, 1.0);
    float dustMid = DustLayer(atmosphereUV, 24.0, 0.025, 0.090, 2.0);
    float dustSmall = DustLayer(atmosphereUV, 34.0, 0.015, 0.078, 3.0);
    float dustTotal = clamp(dustBig * 1.05 + dustMid * 0.8 + dustSmall * 0.6, 0.0, 1.3);

    vec3 dustGlow = vec3(0.82, 0.95, 0.84) * dustTotal * (spotCone * 0.85 + 0.04 * smokeBillows);
    col = clamp(col + dustGlow, 0.0, 1.0);

    // 6. ПЛІВКОВЕ ЗЕРНО (24 FPS)
    vec2 grainPixel = floor(fragTexCoord * resolution);
    float grainFrame = floor(time * 24.0);
    float n1 = hash13(vec3(grainPixel, grainFrame));
    float n2 = hash13(vec3(grainPixel + 17.3, grainFrame + 5.1));
    float filmNoise = (n1 + n2 - 1.0);
    col = clamp(col + vec3(filmNoise * (0.016 + transitionProgress * 0.015)), 0.0, 1.0);

    finalColor = vec4(col, baseSample.a) * fragColor;
}