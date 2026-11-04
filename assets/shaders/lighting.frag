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

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    vec2 pixelPos = fragTexCoord * resolution;
    float dist = distance(pixelPos, playerScreenPos);
    
    float currentRadius = lightRadius * flicker;
    float innerRadius = currentRadius * 0.05;
    float lightFactor = 1.0 - smoothstep(innerRadius, currentRadius, dist);
    
    vec3 finalRGB = mix(texColor.rgb * ambientColor.rgb, texColor.rgb, lightFactor);
    finalColor = vec4(finalRGB, texColor.a);
}