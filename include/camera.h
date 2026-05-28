#pragma once
#include "glad/glad.h"
#include "window.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct CameraUniformData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 camPos;
};

struct Camera {
    Camera();

    Camera(const Camera &) = delete;

    Camera &operator=(const Camera &) = delete;

    Camera(Camera &&other) noexcept;

    Camera &operator=(Camera &&other) noexcept;

    GLuint cameraUBO{};

    float fov = 45;
    float near = 0.1;
    float far = 1000;

    CameraUniformData cameraData{};

    void UpdateCamera(const Window &window, const glm::mat4 &transform);

    ~Camera();
};
