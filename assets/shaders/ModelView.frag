#version 150

#define PEPR3D_MAX_PALETTE_COLORS 4

in vec3 Normal;
flat in uint ColorIndex;

out vec4 oColor;

uniform vec4 uColorPalette[PEPR3D_MAX_PALETTE_COLORS];

void main() {
    const vec3 L = vec3(0, 0, 1);
    vec3 N = normalize(Normal);
    float lambert = max(0.0, dot(N, L));
    float ambient = 0.2;
    float lightIntensity = lambert + ambient;
    oColor = uColorPalette[ColorIndex] * vec4(vec3(lightIntensity), 1.0);
}
