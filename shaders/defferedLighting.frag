#version 460 core
uniform sampler2D Normal;
uniform sampler2D Position;
uniform sampler2D Albedo;
uniform sampler2D Depth ; 

uniform sampler2D ShadowMap;

in vec2 UVs;
layout (std140, binding = 0) uniform CameraUniformData
{
    mat4 view;
    mat4 projection;
    vec4 cameraPos;
    float near;
    float far;
};
struct PointLight
{
    vec3 color;
    float intensity;
    vec3 position;
    float radius;
};
#define MAX_POINT_LIGHTS 40
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int pointLightCount;
uniform vec3 DirectionalLightDirection;
uniform vec3 DirectionalLightColor;

out vec4 FragColor;

void main()
{
    vec3 fragPos = texture(Position, UVs).rgb;
    vec3 normal = normalize(texture(Normal, UVs).rgb);
    vec3 albedo = texture(Albedo, UVs).rgb;
    vec3 lightDir = normalize(-DirectionalLightDirection);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 ambient = albedo * 0.1;
    vec3 directional = albedo * DirectionalLightColor * diff;
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < pointLightCount; i++)
    {
        vec3 toLight = pointLights[i].position - fragPos;
        float distance = length(toLight);
        if (distance > pointLights[i].radius) continue;
        vec3 L = normalize(toLight);
        float NdotL = max(dot(normal, L), 0.0);
        float attenuation = 1.0 - (distance / pointLights[i].radius);
        attenuation *= attenuation;
        pointLighting += albedo * pointLights[i].color * pointLights[i].intensity * NdotL * attenuation;
    }
    vec3 finalColor = ambient + directional + pointLighting;
    float rawDepth = texture(Depth, UVs).r;
    float linearDepth = (2.0 * near * far) / (far + near - rawDepth * (far - near));
    float fogFactor = clamp((linearDepth - near) / (far - near), 0.0, 1.0);
    fogFactor *= fogFactor;
    vec3 atmosColor = vec3(0.6, 0.7, 0.9);
    finalColor = mix(finalColor, atmosColor, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}