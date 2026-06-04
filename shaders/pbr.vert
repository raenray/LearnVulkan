#version 450

layout(location = 0) in  vec3 inPosition;
layout(location = 1) in  vec3 inNormal;
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;

layout(push_constant) uniform PC {
    mat4 mvp;
    mat4 model;
} pc;

void main() {
    fragWorldPos   = vec3(pc.model * vec4(inPosition, 1.0));
    fragNormal     = normalize(mat3(pc.model) * inNormal);
    gl_Position    = pc.mvp   * vec4(inPosition, 1.0);
}
