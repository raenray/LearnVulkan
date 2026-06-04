#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec4 outNorm;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec2 outMaterial;

void main() {
    outPos      = vec4(fragPos, 1.0);
    outNorm     = vec4(normalize(fragNormal), 0.0);
    outAlbedo   = vec4(0.8, 0.3, 0.1, 1.0);   // copper-orange
    outMaterial = vec2(0.0, 0.4);              // metallic=0, roughness=0.4
}
