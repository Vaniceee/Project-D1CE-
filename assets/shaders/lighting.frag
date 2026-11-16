#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 playerScreenPos;
uniform vec2 resolution;
uniform float lightRadius;
uniform float flicker;
uniform vec4 ambientColor;
uniform float dangerFactor;

// Параметри стану гравця
uniform float exhaustionFactor; // 0.0 .. 1.0 (низька стаміна / виснаження)
uniform float panicFactor;      // 0.0 .. 1.0 (близькість охоронця / погоня / удар)
uniform float time;             // Час для пульсації серцебиття, хвиль та зерна

// Псевдовипадковий генератор без артефактів смуг
float hash13(vec3 p3)
{
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 centeredUV = uv - 0.5;
    float centerDist = length(centeredUV) * 1.4142; // 0.0 у центрі, ~1.0 по кутах
    vec2 radialDir = (centerDist > 0.0001) ? normalize(centeredUV) : vec2(0.0);
    float edgeMask = clamp(pow(centerDist, 2.0), 0.0, 1.0);

    float stressLevel = max(exhaustionFactor, panicFactor);

    // 1. ЛЕГКЕ ХВИЛЬОВЕ ВИКРИВЛЕННЯ ПО ПЕРИМЕТРУ
    float waveStrength = (exhaustionFactor * 0.0035 + panicFactor * 0.0025) * edgeMask;
    vec2 waveOffset = vec2(
        sin(uv.y * 18.0 + time * 4.5),
        cos(uv.x * 18.0 + time * 3.8)
    ) * waveStrength;
    vec2 distortedUV = clamp(uv + waveOffset, vec2(0.001), vec2(0.999));

    // 2. ХРОМАТИЧНА АБЕРАЦІЯ (розшарування R/B каналів по краях кадру)
    float pulseBeat = 0.5 + 0.5 * sin(time * mix(5.0, 11.5, panicFactor));
    float chromaAmount = (exhaustionFactor * 0.0045 + panicFactor * 0.0065 * (0.65 + 0.35 * pulseBeat)) * edgeMask;

    vec2 uvR = clamp(distortedUV + radialDir * chromaAmount, vec2(0.001), vec2(0.999));
    vec2 uvB = clamp(distortedUV - radialDir * chromaAmount, vec2(0.001), vec2(0.999));

    vec4 baseSample = texture(texture0, distortedUV);
    float rChannel = texture(texture0, uvR).r;
    float bChannel = texture(texture0, uvB).b;
    vec3 texRGB = vec3(rChannel, baseSample.g, bChannel);

    // 3. КІЛЬЦЕ СВІТЛА ТА ТЕМПЕРАТУРНИЙ ПЕРЕХІД
    vec2 pixelPos = distortedUV * resolution;
    float dist = distance(pixelPos, playerScreenPos);

    float currentRadius = lightRadius * flicker;
    float innerRadius = currentRadius * 0.035;
    float lightFactor = 1.0 - smoothstep(innerRadius, currentRadius, dist);

    float luma = dot(texRGB, vec3(0.299, 0.587, 0.114));
    vec3 desaturated = mix(vec3(luma), texRGB, mix(0.55, 1.05, lightFactor));
    vec3 warmCore = vec3(1.12, 1.01, 0.86);
    vec3 coldEdge = vec3(0.78, 0.84, 0.96);
    vec3 tempTint = mix(coldEdge, warmCore, smoothstep(0.15, 0.85, lightFactor));
    vec3 litColor = desaturated * tempTint;

    // Червоний контур небезпеки по краях кільця світла
    float rimMask = smoothstep(0.02, 0.38, lightFactor) * (1.0 - smoothstep(0.45, 0.92, lightFactor));
    vec3 dangerTint = vec3(1.0, 0.16, 0.12);
    litColor = mix(litColor, litColor * dangerTint * 1.45, rimMask * dangerFactor * 0.85);

    vec3 finalRGB = mix(texRGB * ambientColor.rgb, litColor, lightFactor);

    // 4. ФІЛЬТРАЦІЯ КОЛЬОРІВ ТА КОНТРАСТУ (виснаження vs адреналін)
    float finalLuma = dot(finalRGB, vec3(0.299, 0.587, 0.114));
    vec3 exhaustedLook = mix(finalRGB, vec3(finalLuma) * vec3(0.90, 0.94, 1.03), exhaustionFactor * 0.58);
    vec3 contrastLook = clamp((exhaustedLook - 0.5) * (1.0 + stressLevel * 0.22) + 0.5, 0.0, 1.0);
    finalRGB = contrastLook;

    // 5. ПУЛЬСУЮЧЕ ТУНЕЛЬНЕ ЗАТЕМНЕННЯ (динамічна віньєтка)
    float beatPhase = fract(time * mix(1.1, 2.35, panicFactor));
    float lub = exp(-pow((beatPhase - 0.15) * 14.0, 2.0));
    float dub = exp(-pow((beatPhase - 0.34) * 16.0, 2.0)) * 0.65;
    float heartPulse = clamp(lub + dub, 0.0, 1.0);

    float tunnelStart = mix(0.88, 0.42, stressLevel) - heartPulse * stressLevel * 0.09;
    float tunnelEnd = mix(1.35, 0.96, stressLevel) - heartPulse * stressLevel * 0.06;
    float vignette = 1.0 - smoothstep(tunnelStart, tunnelEnd, centerDist);

    vec3 tunnelTint = mix(vec3(0.0), vec3(0.14, 0.01, 0.01), panicFactor * (1.0 - vignette) * (0.4 + 0.6 * heartPulse));
    finalRGB = finalRGB * mix(1.0, vignette, 0.35 + stressLevel * 0.65) + tunnelTint;

    // 6. НЕНАВ'ЯЗЛИВА ПЛІВКОВА ЗЕРНИСТІСТЬ (24 FPS, прив'язка до сітки пікселів)
    vec2 grainPixel = floor(uv * resolution);
    float grainFrame = floor(time * 24.0);
    // Трикутний розподіл шуму (приємніший для ока, ніж рівномірний)
    float n1 = hash13(vec3(grainPixel, grainFrame));
    float n2 = hash13(vec3(grainPixel + 19.37, grainFrame + 7.13));
    float filmNoise = (n1 + n2 - 1.0);

    // У темряві та під час паніки/втоми зерно трохи відчутніше, у центрі світла — ледь помітне
    float shadowBoost = mix(1.25, 0.65, lightFactor);
    float grainStrength = (0.018 + stressLevel * 0.014) * shadowBoost;
    finalRGB = clamp(finalRGB + vec3(filmNoise * grainStrength), 0.0, 1.0);

    finalColor = vec4(finalRGB, baseSample.a);
}