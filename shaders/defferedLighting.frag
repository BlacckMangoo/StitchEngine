#version 460 core
uniform sampler2D Normal;
uniform sampler2D Position;
uniform sampler2D Albedo;
uniform sampler2D Depth;

in vec2 UVs;

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

uniform vec3 DirectionalLightDirection;
uniform vec3 DirectionalLightColor;
out vec4 FragColor;

void main()
{
    vec3 fragPos = texture(Position, UVs).rgb;
    vec3 normal  = normalize(texture(Normal, UVs).rgb * 2.0 - 1.0);
    vec3 albedo  = texture(Albedo, UVs).rgb;

    vec3 lightDir = normalize(-DirectionalLightDirection);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient = albedo * 0.1;
    vec3 diffuse = albedo * DirectionalLightColor * diff;
    vec3 lighting = ambient + diffuse;

    FragColor = vec4(lighting, 1.0);;

}