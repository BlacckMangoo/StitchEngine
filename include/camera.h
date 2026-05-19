#pragma once
#include "glad/glad.h"
#include "glm/fwd.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct CameraUniformData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 camPos;
};

struct Camera {
    Camera() {
        glCreateBuffers(1, &cameraUBO);
        glNamedBufferData(
            cameraUBO,sizeof(CameraUniformData),nullptr,GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, cameraUBO);
    }


    GLuint cameraUBO{} ;
    glm::vec3 position{0.0f, 0.0f, 10.0f} ;
    glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

    float fov = 45 ;
    float near = 0.1 ;
    float far = 1000;

    CameraUniformData cameraData{
    .camPos = glm::vec4(position,1.0f),};

    void UpdateCamera(const Window& window, const float deltaTime) {
        float speed = 2.5f * deltaTime;

        if (glfwGetKey(window.get(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f;
        if (glfwGetKey(window.get(), GLFW_KEY_W) == GLFW_PRESS)position += speed * camFront;
        if (glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS)position -= speed * camFront;
        if (glfwGetKey(window.get(), GLFW_KEY_A) == GLFW_PRESS)position -= glm::normalize(glm::cross(camFront, camUp)) * speed;
        if (glfwGetKey(window.get(), GLFW_KEY_D) == GLFW_PRESS)position += glm::normalize(glm::cross(camFront, camUp)) * speed;
        if (glfwGetKey(window.get(), GLFW_KEY_E) == GLFW_PRESS)position += speed * camUp;
        if (glfwGetKey(window.get(), GLFW_KEY_Q) == GLFW_PRESS)position -= speed * camUp;
        cameraData.camPos = glm::vec4(position,1.0f);

        cameraData.view = glm::lookAt( glm::vec3(position), glm::vec3(position)+ camFront, camUp);
        cameraData.proj = glm::perspective(
            glm::radians(fov),
            window.aspectRatio(),
            near,far
        );
        glNamedBufferSubData(
            cameraUBO,0,sizeof(cameraData),&cameraData);
    }

    ~Camera() {
        glDeleteBuffers(1, &cameraUBO);
    }

};


