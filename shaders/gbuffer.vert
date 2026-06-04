#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(push_constant) uniform PC {
    mat4 mvp;
    mat4 model;
};

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;

void main() {
    gl_Position = mvp * vec4(inPosition, 1.0);
    fragPos     = vec3(model * vec4(inPosition, 1.0));
    fragNormal  = normalize(mat3(model) * inNormal);
}
