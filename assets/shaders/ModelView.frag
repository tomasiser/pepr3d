#version 150

#define PEPR3D_MAX_PALETTE_COLORS 8

in highp vec3 Normal;
in highp vec3 BarycentricCoordinates;
in highp vec3 ModelCoordinates;
flat in uint ColorIndex;
flat in int AreaHighlightMask;

out vec4 oColor;

uniform vec4 uColorPalette[PEPR3D_MAX_PALETTE_COLORS];
uniform vec3 uAreaHighlightColor;
uniform vec3 uAreaHighlightOrigin;
uniform float uAreaHighlightSize;
uniform bool uShowWireframe;
uniform bool uAreaHighlightEnabled;

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

float getAreaHighlightAlpha()
{
	if(uAreaHighlightEnabled && AreaHighlightMask!=0)
	{
		vec3 lineToOrigin = ModelCoordinates - uAreaHighlightOrigin;
		float distToOriginSquared = dot(lineToOrigin, lineToOrigin);
		float distLimitSquared = uAreaHighlightSize*uAreaHighlightSize;
		
		if(distToOriginSquared < distLimitSquared)
			return 1;
	}

	return 0;
}

void main() {
    const vec3 L = vec3(0, 0, 1);
    vec3 N = normalize(Normal);
    float lambert = max(0.0, dot(N, L));
    float ambient = 0.2;
    float lightIntensity = lambert + ambient;

    vec3 materialColor = uColorPalette[ColorIndex].rgb;
	float areaHighlightAlpha = getAreaHighlightAlpha();
	materialColor = mix(materialColor, uAreaHighlightColor, areaHighlightAlpha);

    vec3 wireframeColor = uShowWireframe ? getWireframeColor(materialColor) : materialColor;
    vec3 triangleColor = wireframe(materialColor, wireframeColor, 1.0);


    oColor = vec4(vec3(lightIntensity * triangleColor), 1.0);
}
