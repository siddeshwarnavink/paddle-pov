#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUv;

layout(location = 0) out vec3 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(2.0, -2.0, 5.0));
const float AMBIENT = 0.25;

void main() {
    gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(inPosition, 1.0);

    vec3 normalWorldSpace = normalize(mat3(pushConstants.model) * inNormal);
    float lightIntensity = max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0);
    lightIntensity += AMBIENT;

    fragColor = inColor * lightIntensity;
}
