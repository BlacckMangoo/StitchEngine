#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 tangent; // tangent.w = handedness

uniform mat4 model;

layout(std140, binding = 0) uniform CameraUniformData
{
    mat4 view;
    mat4 projection;
    vec4 cameraPos;
};

layout(std140, binding = 1) uniform TimeData
{
    float deltaTime;
    float time;
};

out vec2  UVs;
out vec3  WorldPos;
out mat3  TBN;

void main()
{
    mat4 MVP = projection * view * model;
    gl_Position = MVP * vec4(pos, 1.0);

    UVs      = uv;
    WorldPos = (model * vec4(pos, 1.0)).xyz;

    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vec3 N = normalize(normalMatrix * normal);
    vec3 T = normalize(normalMatrix * tangent.xyz);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalize
    vec3 B = cross(N, T) * tangent.w;      // handedness flips bitangent for mirrored UVs

    TBN = mat3(T, B, N);
}