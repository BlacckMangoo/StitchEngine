#version 460 core 


layout(location = 0) in vec3 pos;

uniform mat4 MLP; // Light space matrix
uniform mat4 model;  // model matrix of the primitive

void main()
{
    gl_Position = MLP * model * vec4(pos, 1.0);
}