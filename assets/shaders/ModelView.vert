#version 150

uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;
in uint aColorIndex;

out highp vec3 Normal;
flat out uint ColorIndex;

void main() {
    Normal = ciNormalMatrix * ciNormal;
    ColorIndex = aColorIndex;
    gl_Position = ciModelViewProjection * ciPosition;
}
