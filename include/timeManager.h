#pragma once
#include "glad/glad.h"
#include "GLFW/glfw3.h"

struct alignas(16) TimeUniformBufferObject {
    float deltaTime = 0.0;
    float time = 0.0;
    float pad1 = 0.0;
    float pad2 = 0.0;
};

struct TimeManager {
    TimeManager();

    void Tick();

    [[nodiscard]] float GetDeltaTime() const;

    [[nodiscard]] float GetCurrentTime() const;

    TimeUniformBufferObject timeUniformBufferData{};
    float lastFrame = 0.0f;
    unsigned int ubo{};

    ~TimeManager();
};
