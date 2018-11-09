#version 150

uniform mat4 ciModelViewProjection;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;
in float aColorIndex;

out highp vec3 Normal;
out highp float ColorIndex;

void main() {
    Normal = ciNormalMatrix * ciNormal;
    ColorIndex = aColorIndex;
    gl_Position = ciModelViewProjection * ciPosition;
}
