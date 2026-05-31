#version 460

in vec2 UVs;
uniform sampler2D screen;
out vec4 FragColor;

void main()
{
    vec3 hdrColor = texture(screen, UVs).rgb;
    FragColor = vec4(hdrColor, 1.0);
}