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

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    vec2 pixelPos = fragTexCoord * resolution;
    float dist = distance(pixelPos, playerScreenPos);
    
    float currentRadius = lightRadius * flicker;
    float innerRadius = currentRadius * 0.05;
    float lightFactor = 1.0 - smoothstep(innerRadius, currentRadius, dist);
    
    // Виділяємо зовнішній край кола світла (півтінь) для червоного сигналу небезпеки
    float rimMask = smoothstep(0.02, 0.38, lightFactor) * (1.0 - smoothstep(0.45, 0.92, lightFactor));
    vec3 dangerTint = vec3(1.0, 0.16, 0.12);
    vec3 litColor = mix(texColor.rgb, texColor.rgb * dangerTint * 1.45, rimMask * dangerFactor * 0.85);
    
    vec3 finalRGB = mix(texColor.rgb * ambientColor.rgb, litColor, lightFactor);
    finalColor = vec4(finalRGB, texColor.a);
}