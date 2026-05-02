#version 450

in vec2 UVs ;
uniform sampler2D screen ;
out vec4 FragColor ;

void main()
{
    FragColor = texture(screen,UVs);
}