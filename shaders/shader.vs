#version 330 core
layout(location = 0) in vec2 aPos;

out vec3 vColor;

uniform int uNumSegments;
uniform vec2 uTranslate;
uniform float uScale;

vec3 hsv2rgb(float h, float s, float v) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(vec3(h) + K.xyz) * 6.0 - K.www);
    return v * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), s);
}

void main() {
    float t = float(floor(gl_VertexID /2)) / float(uNumSegments);

    vColor = hsv2rgb(t, 1.0, 1.0);

    gl_Position = vec4((aPos + uTranslate) * uScale, 0.0, 1.0);
}