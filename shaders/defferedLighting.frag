#version 460 core

uniform sampler2D Normal;
uniform sampler2D Position;
uniform sampler2D Albedo;

uniform sampler2D Depth;
uniform sampler2D ShadowMap;
uniform mat4      MLP;

struct PointLight
{
    vec3  color;
    float intensity;
    vec3  position;
    float radius;
};

#define MAX_POINT_LIGHTS 40
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int        pointLightCount;

uniform vec3 DirectionalLightDirection;
uniform vec3 DirectionalLightColor;

layout(std140, binding = 0) uniform CameraUniformData
{
    mat4  view;
    mat4  projection;
    vec4  cameraPos;
    float near;
    float far;
};

in  vec2 UVs;
out vec4 FragColor;

const float PI = 3.14159265359;


float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness)
    * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(vec3 fragPos)
{
    vec4 fragPosLS  = MLP * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    projCoords.z > 1.0)
    return 1.0;

    float shadow    = 0.0;
    float bias      = 0.005;
    vec2 texelSize = 1.0 / vec2(textureSize(ShadowMap, 0));
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float pcfDepth = texture(ShadowMap,
                                 projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += (projCoords.z - bias > pcfDepth) ? 0.0 : 1.0;
    }

    return shadow / 9.0;
}

vec3 CalcPBR(vec3 N, vec3 V, vec3 L, vec3 radiance,
             vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0001);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(HdotV, F0);

    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
    vec3 kD       = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse  = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

void main()
{
    vec4 normalSample   = texture(Normal,   UVs);
    vec4 positionSample = texture(Position, UVs);
    vec4 albedoSample   = texture(Albedo,   UVs);

    vec3  fragPos   = positionSample.rgb;
    vec3  N         = normalize(normalSample.rgb);
    vec3  albedo    = pow(albedoSample.rgb, vec3(2.2));
    float metallic  = positionSample.a;
    float roughness = clamp(albedoSample.a, 0.04, 1.0);
    float ao        = normalSample.a;

    vec3 V  = normalize(cameraPos.xyz - fragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float shadow = ShadowCalculation(fragPos);
    vec3  L_dir  = normalize(-DirectionalLightDirection);
    vec3  Lo     = CalcPBR(N, V, L_dir, DirectionalLightColor,
                           albedo, metallic, roughness, F0) * shadow;

    for (int i = 0; i < pointLightCount; i++)
    {
        vec3  toLight     = pointLights[i].position - fragPos;
        float dist        = length(toLight);
        if (dist > pointLights[i].radius) continue;

        vec3  L_pt        = normalize(toLight);
        float distOverR   = dist / pointLights[i].radius;
        float window      = clamp(1.0 - distOverR * distOverR * distOverR * distOverR, 0.0, 1.0);
        float attenuation = (window * window) / max(dist * dist, 0.0001);
        vec3  radiance    = pointLights[i].color * pointLights[i].intensity * attenuation;

        Lo += CalcPBR(N, V, L_pt, radiance, albedo, metallic, roughness, F0);
    }

    vec3 ambient = vec3(0.05) * albedo * ao;

    vec3 color   = ambient + Lo;

    float rawDepth    = texture(Depth, UVs).r;
    float linearDepth = (2.0 * near * far) / (far + near - rawDepth * (far - near));
    float fogFactor   = clamp((linearDepth - near) / (far - near), 0.0, 1.0);
    fogFactor  *= fogFactor;
    color = mix(color, vec3(0.6, 0.7, 0.9), fogFactor);


    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}