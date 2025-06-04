#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform Light {
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
} light;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(light.lightPos - vec3(gl_FragCoord.xyz));
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Plastic specular
    vec3 viewDir = normalize(light.viewPos - vec3(gl_FragCoord.xyz));
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    // Ambient, diffuse, specular
    vec3 ambient = 0.2 * fragColor;
    vec3 diffuse = diff * fragColor;
    vec3 specular = spec * light.lightColor;
    
    // Emissive (glow) - increased for stronger emission
    vec3 emissive = 1.2 * fragColor;
    
    vec3 result = ambient + diffuse + specular + emissive;
    outColor = vec4(result, 1.0);
}
