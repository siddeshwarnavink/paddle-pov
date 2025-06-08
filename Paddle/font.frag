#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D fontSampler;

void main() {
    float alpha = texture(fontSampler, fragUV).r;
    outColor = vec4(fragColor, alpha);
    if (alpha < 0.01)
        discard;
}
