#version 460 core

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;     // rgb = world normal, a = packed AO

uniform sampler2D baseColor;
uniform sampler2D metallicRoughness;
uniform sampler2D occlusion;
uniform sampler2D normalMap;

uniform vec4  baseColorFactor;
uniform float metallicFactor;
uniform float roughnessFactor;

in vec2 UVs;
in vec3 WorldPos;
in mat3 TBN;

void main()
{
    vec4 albedoSample = texture(baseColor, UVs) * baseColorFactor;

    vec4  mrSample  = texture(metallicRoughness, UVs);
    float metallic  = mrSample.b * metallicFactor;
    float roughness = clamp(mrSample.g * roughnessFactor, 0.04, 1.0);

    float ao = texture(occlusion, UVs).r;

    vec3 normalSample = texture(normalMap, UVs).rgb * 2.0 - 1.0;
    vec3 N = normalize(TBN * normalSample);

    gColor    = vec4(albedoSample.rgb, roughness);  // a = roughness
    gPosition = vec4(WorldPos,         metallic);   // a = metallic
    gNormal   = vec4(N,                ao);         // a = AO

}