#version 460 core

// Post processing pass

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

out vec2 UVs;

void main()
{
    gl_Position = vec4(pos, 1.0);
    UVs = uv;
}