#version 460 core

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec2 gUV;
layout(location = 3) out vec4 gNormal;

uniform sampler2D baseColor;
uniform sampler2D metallicRoughness;
uniform vec4 baseColorFactor;
uniform float metallicFactor;
uniform float roughnessFactor;

in vec2 UVs;
in vec3 worldPos;
in vec3 Normal;

void main()
{
    vec4 albedoSample = texture(baseColor, UVs);

    vec4 mrSample = texture(metallicRoughness, UVs);

    float metallic = mrSample.b * metallicFactor;
    float roughness = mrSample.g * roughnessFactor;

    vec3 normal = normalize(Normal);

    gColor    = albedoSample * baseColorFactor;
    gPosition = vec4(worldPos, 1.0);
    gUV       = UVs;
    gNormal   = vec4(normal, metallic * 0.5 + roughness * 0.5);
}