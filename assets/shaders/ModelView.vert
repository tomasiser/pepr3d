#version 150

uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;
in vec4 ciColor;
in uint aColorIndex;

out highp vec3 Normal;
out highp vec3 BarycentricCoordinates;
flat out uint ColorIndex;
out highp vec4 Color;

void main() {
    Normal = ciNormalMatrix * ciNormal;
    ColorIndex = aColorIndex;
    Color = ciColor;
    gl_Position = ciModelViewProjection * ciPosition;

    int vertexMod3 = gl_VertexID % 3;
    BarycentricCoordinates = vec3(float(vertexMod3 == 0), float(vertexMod3 == 1), float(vertexMod3 == 2));
}
