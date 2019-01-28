#version 150

#define PEPR3D_MAX_PALETTE_COLORS 16


in highp vec3 Normal;
in highp vec3 BarycentricCoordinates;
in highp vec3 ModelCoordinates;
in vec4 Color;

flat in uint ColorIndex;
flat in int AreaHighlightMask;

out vec4 oColor;

uniform vec4 uColorPalette[PEPR3D_MAX_PALETTE_COLORS];
uniform vec3 uAreaHighlightColor;
uniform vec3 uAreaHighlightOrigin;
uniform float uAreaHighlightSize;
uniform bool uShowWireframe;
uniform bool uAreaHighlightEnabled;
uniform bool uOverridePalette;
uniform float uGridOffset;
uniform vec2 uPreviewMinMaxHeight;
uniform mat4 ciModelMatrix;

// From Florian Boesch post on barycentric coordinates
// http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/
float edgeFactor(float lineWidth) {
    vec3 d = fwidth(BarycentricCoordinates);
    vec3 a3 = smoothstep(vec3(0.0), d * lineWidth, BarycentricCoordinates);
    return min(min(a3.x, a3.y), a3.z);
}

vec3 wireframe(vec3 fill, vec3 stroke, float lineWidth) {
    return mix(stroke, fill, edgeFactor(lineWidth));
}

vec3 getWireframeColor(vec3 fill) {
    float brightness = 0.2126 * fill.r + 0.7152 * fill.g + 0.0722 * fill.b;
    return (brightness > 0.75) ? vec3(0.11, 0.165, 0.208) : vec3(0.988, 0.988, 0.988);
}

float getAreaHighlightAlpha() {
    if(uAreaHighlightEnabled && AreaHighlightMask!=0)
    {
        vec3 lineToOrigin = ModelCoordinates - uAreaHighlightOrigin;
        float distToOriginSquared = dot(lineToOrigin, lineToOrigin);
        float distLimitSquared = uAreaHighlightSize*uAreaHighlightSize;
    
        if(distToOriginSquared < distLimitSquared) {
            return 1;
        }
    }
    
    return 0;
}

void main() {
    // discard pixels that are below / above the preview height limits:
    float minRelativeHeight = uPreviewMinMaxHeight.x;
    float maxRelativeHeight = uPreviewMinMaxHeight.y;
    if (minRelativeHeight > 0.0f || maxRelativeHeight < 1.0f) {
        // we need relativeHeight == 0 for the lowest part of the model:
        // (uGridOffset is half-height of the model)
        float relativeHeight = (ciModelMatrix * vec4(ModelCoordinates, 1.0)).y - uGridOffset;
        // additionally, we need relativeHeight == 1 for the highest part of the model:
        relativeHeight /= 2.0f * abs(uGridOffset);

        if (minRelativeHeight > 0.0f && relativeHeight < minRelativeHeight) {
            discard;
            return;
        }

        if (maxRelativeHeight < 1.0f && relativeHeight > maxRelativeHeight) {
            discard;
            return;
        }
    }

    const vec3 L = vec3(0, 0, 1);
    vec3 N = normalize(Normal);
    float lambert = abs(dot(N, L));//max(0.0, dot(N, L));
    float ambient = 0.2;
    float lightIntensity = lambert + ambient;

    float areaHighlightAlpha = getAreaHighlightAlpha();
    vec3 materialColor = uOverridePalette ? Color.rgb : uColorPalette[ColorIndex].rgb;
    materialColor = mix(materialColor, uAreaHighlightColor, areaHighlightAlpha);
    vec3 wireframeColor = uShowWireframe ? getWireframeColor(materialColor) : materialColor;
    vec3 triangleColor = wireframe(materialColor, wireframeColor, 1.0);

    oColor = vec4(vec3(lightIntensity * triangleColor), uOverridePalette ? Color.a : 1.0);

}
