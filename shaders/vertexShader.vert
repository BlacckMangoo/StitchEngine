#version 460 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

uniform mat4 model;

out vec3 worldPos;

layout( std140, binding = 0) uniform CameraUniformData
{
    mat4 view;
    mat4 projection;
    vec4 cameraPos;
};


layout( std140, binding = 1) uniform TimeData
{
    float deltaTime ;
    float time;
};

out vec2 UVs;
out vec3 Normal;

void main()
{
    mat4 MVP = projection * view * model;
    gl_Position = MVP * vec4(pos, 1.0);
    UVs = uv;
    vec4 world = model * vec4(pos, 1.0);
    worldPos = world.xyz;

    Normal = mat3(transpose(inverse(model))) * normal; // Normal Matrix for correcting non uniform scaling

}

