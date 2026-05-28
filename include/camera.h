#pragma once
#include "glad/glad.h"
#include "window.h"

#include <mesh.h>

struct CameraUniformData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 camPos;
};

struct Camera {
    Camera() {
        glCreateBuffers(1, &cameraUBO);
        glNamedBufferData(cameraUBO, sizeof(CameraUniformData), nullptr,
                          GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    }

    Camera(const Camera &) = delete;

    Camera &operator=(const Camera &) = delete;

    Camera(Camera &&other) noexcept {
        cameraUBO = other.cameraUBO;
        other.cameraUBO = 0;
        fov = other.fov;
        near = other.near;
        far = other.far;
        cameraData = other.cameraData;
    }

    Camera &operator=(Camera &&other) noexcept {
        if (this != &other) {
            if (cameraUBO)
                glDeleteBuffers(1, &cameraUBO);

            cameraUBO = other.cameraUBO;
            other.cameraUBO = 0;

            fov = other.fov;
            near = other.near;
            far = other.far;

            cameraData = other.cameraData;
        }
        return *this;
    }

    GLuint cameraUBO{};

    float fov = 45;
    float near = 0.1;
    float far = 1000;

    CameraUniformData cameraData{};

    void UpdateCamera(const Window &window, const glm::mat4 &transform) {
        const glm::vec3 position = glm::vec3(transform[3]);
        const glm::vec3 forward = -glm::normalize(glm::vec3(transform[2]));
        const glm::vec3 up = glm::normalize(glm::vec3(transform[1]));

        cameraData.camPos = glm::vec4(position, 1.0f);

        cameraData.view = glm::lookAt(position, position + forward, up);

        cameraData.proj =
                glm::perspective(glm::radians(fov), window.aspectRatio(), near, far);

        glNamedBufferSubData(cameraUBO, 0, sizeof(cameraData), &cameraData);
    }

    ~Camera() {
        glDeleteBuffers(1, &cameraUBO);
    }
};
