#pragma once
#include "glad/glad.h"

struct alignas(16) TimeUniformBufferObject {
    float deltaTime = 0.0;
    float time = 0.0;
    float pad1 = 0.0;
    float pad2 = 0.0;
};


struct TimeManager {
    TimeManager() {
        glCreateBuffers(1, &ubo);
        glNamedBufferData(ubo, sizeof(TimeUniformBufferObject), nullptr,GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
    }

    void Tick() {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        timeUniformBufferData.deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        timeUniformBufferData.time += timeUniformBufferData.deltaTime;

        glNamedBufferSubData(ubo, 0, sizeof(TimeUniformBufferObject), &timeUniformBufferData);
    }

    [[nodiscard]] float GetDeltaTime() const {
        return timeUniformBufferData.deltaTime;
    }

    [[nodiscard]] float GetCurrentTime() const {
        return timeUniformBufferData.time;
    }


    TimeUniformBufferObject timeUniformBufferData{};
    float lastFrame = 0.0f;
    unsigned int ubo{};

    ~TimeManager() {
        glDeleteBuffers(1, &ubo);
    }
};
