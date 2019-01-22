#version 150

uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;
in vec4 ciColor;
in uint aColorIndex;
in int aAreaHighlightMask; 

out highp vec3 Normal;
out highp vec3 BarycentricCoordinates;
out highp vec3 ModelCoordinates;
flat out uint ColorIndex;
flat out int AreaHighlightMask;
out highp vec4 Color;


void main() {
    Normal = ciNormalMatrix * ciNormal;
    ColorIndex = aColorIndex;
    ModelCoordinates = ciPosition.xyz;
    Color = ciColor;
    gl_Position = ciModelViewProjection * ciPosition;
    AreaHighlightMask = aAreaHighlightMask;

    int vertexMod3 = gl_VertexID % 3;
    BarycentricCoordinates = vec3(float(vertexMod3 == 0), float(vertexMod3 == 1), float(vertexMod3 == 2));
}
