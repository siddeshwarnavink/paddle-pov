#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 proj;
} pc;

void main() {
    gl_Position = pc.proj * vec4(inPos, 1.0);
    fragUV = inUV;
    fragColor = inColor;
}
