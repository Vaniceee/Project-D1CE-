#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float time;
uniform float fadeIn;
uniform float hasBgTexture;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
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
        p = p * 2.05 + vec2(13.7, 29.3);
        amp *= 0.5;
    }
    return value;
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centered = uv - 0.5;
    float distCenter = length(centered) * 1.4142;

    // 1. Горизонтальні перешкоди втраченого сигналу камери (NO SIGNAL)
    float burst = smoothstep(0.75, 0.98, sin(time * 2.7) * sin(time * 6.3 + 0.8));
    // Сильний глітч у перші 0.7 с після смерті
    float initialShock = 1.0 - smoothstep(0.0, 0.75, time);
    float glitchPower = max(burst, initialShock);

    float row = floor(uv.y * 160.0);
    float tick = floor(time * 22.0);
    float lineNoise = hash12(vec2(row, tick));
    float activeRow = step(0.84 - glitchPower * 0.28, lineNoise);
    float shiftX = (hash12(vec2(row, tick + 5.0)) - 0.5) * 0.025 * glitchPower * activeRow;

    vec2 glitchedUV = clamp(vec2(uv.x + shiftX, uv.y), vec2(0.001), vec2(0.999));

    // 2. Базовий фон або похмурий багряний туман казино
    vec3 col = vec3(0.0);
    if (hasBgTexture > 0.5) {
        float chroma = 0.004 * glitchPower;
        float r = texture(texture0, clamp(glitchedUV + vec2(chroma, 0.0), vec2(0.001), vec2(0.999))).r;
        float g = texture(texture0, glitchedUV).g;
        float b = texture(texture0, clamp(glitchedUV - vec2(chroma, 0.0), vec2(0.001), vec2(0.999))).b;
        col = vec3(r, g, b) * vec3(0.55, 0.35, 0.38);
    }
    else {
        vec2 smokeUV1 = glitchedUV * vec2(2.6, 1.9) + vec2(time * 0.025, -time * 0.015);
        vec2 smokeUV2 = glitchedUV * vec2(4.4, 3.2) + vec2(-time * 0.018, -time * 0.028);
        float smoke = smoothstep(0.32, 0.82, fbm(smokeUV1) * 0.65 + fbm(smokeUV2) * 0.35);

        float bottomGlow = smoothstep(0.15, 1.0, glitchedUV.y);
        vec3 baseCrimson = vec3(0.13, 0.015, 0.02) * bottomGlow;
        vec3 smokeCrimson = vec3(0.24, 0.04, 0.05) * smoke * (0.35 + 0.65 * bottomGlow);
        col = baseCrimson + smokeCrimson;
    }

    // 3. Біжуча смуга перешкод VHS/CCTV
    float barPos = fract(time * 0.35);
    float barDist = abs(uv.y - barPos);
    float rollBar = exp(-barDist * 38.0) * 0.08 * (0.4 + 0.6 * glitchPower);
    col += vec3(0.45, 0.18, 0.20) * rollBar;

    // 4. Сканлайни старого монітора
    float scanline = 1.0 - 0.14 * (0.5 + 0.5 * sin(uv.y * resolution.y * 1.5));
    col *= scanline;

    // 5. Глибока віньєтка по кутах
    float vignette = 1.0 - smoothstep(0.45, 1.25, distCenter);
    col *= mix(0.25, 1.0, vignette);

    // 6. Плівкове зерно (24 FPS)
    vec2 grainPixel = floor(uv * resolution);
    float grainFrame = floor(time * 24.0);
    float n1 = hash13(vec3(grainPixel, grainFrame));
    float n2 = hash13(vec3(grainPixel + 13.1, grainFrame + 7.7));
    float filmNoise = (n1 + n2 - 1.0);
    col = clamp(col + vec3(filmNoise * (0.024 + glitchPower * 0.02)), 0.0, 1.0);

    finalColor = vec4(col * fadeIn, 1.0) * fragColor;
}