#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uvs;

uniform mat4 model;

layout( std140, binding = 0) uniform CameraUniformData
{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
};

out vec2 UVs;
void main()
{
    mat4 MVP = projection * view * model;
    gl_Position = MVP * vec4(pos, 1.0);
    UVs = uvs;
}

