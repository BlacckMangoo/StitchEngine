#version 460

in vec2 UVs;

uniform sampler2D screen;

out vec4 FragColor;

void main()
{
    vec2 uv = UVs;
    FragColor = texture(screen, UVs);

}