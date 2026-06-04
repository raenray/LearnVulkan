#version 450

// Fullscreen triangle from gl_VertexIndex — no vertex buffer needed
vec2 positions[3] = vec2[](vec2(-1,-1), vec2(3,-1), vec2(-1,3));

layout(location = 0) out vec2 fragUV;

void main() {
    vec2 p = positions[gl_VertexIndex];
    gl_Position = vec4(p, 0.0, 1.0);
    fragUV = p * 0.5 + 0.5;
}
