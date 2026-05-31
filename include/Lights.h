#pragma once
#include "glm/vec3.hpp"

struct DirectionalLight
{
    glm::vec3 direction;
    glm::vec3 color;
    float size;
};

struct PointLight
{
    glm::vec3 color;
    float intensity;
    float radius;
};


inline glm::mat4 CalculateLightSpaceMatrix(const DirectionalLight &dirLight)
{
    const glm::vec3 lightDir = glm::normalize(dirLight.direction);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(lightDir, up)) > 0.99f)
    {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    const glm::vec3 right = glm::normalize(glm::cross(up, lightDir));
    up = glm::normalize(glm::cross(lightDir, right));

    const glm::mat4 lightView = glm::lookAt(-lightDir * 10.0f, // Position the light source far in the direction opposite to its direction
                                      glm::vec3(0.0f),   // Look at the origin
                                      up);               // Up vector

    const float orthoSize = dirLight.size;
    const glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 20.0f);

    return lightProjection * lightView;
}