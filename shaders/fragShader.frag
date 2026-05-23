#version 460 core

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec2 gUV;
layout(location = 3) out vec4 gNormal;

uniform sampler2D tex;
in vec2 UVs;
in vec3 worldPos;
in vec3 Normal;

void main()
{
    gPosition = vec4(worldPos, 1.0);
    gUV = UVs;
    gColor = texture(tex, UVs);
    gNormal = vec4(Normal, 1.0);

}