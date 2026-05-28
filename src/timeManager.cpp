#include "timeManager.h"

TimeManager::TimeManager() {
    glCreateBuffers(1, &ubo);
    glNamedBufferData(ubo, sizeof(TimeUniformBufferObject), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
}

void TimeManager::Tick() {
    const auto currentFrame = static_cast<float>(glfwGetTime());
    timeUniformBufferData.deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    timeUniformBufferData.time += timeUniformBufferData.deltaTime;

    glNamedBufferSubData(ubo, 0, sizeof(TimeUniformBufferObject), &timeUniformBufferData);
}

float TimeManager::GetDeltaTime() const {
    return timeUniformBufferData.deltaTime;
}

float TimeManager::GetCurrentTime() const {
    return timeUniformBufferData.time;
}

TimeManager::~TimeManager() {
    glDeleteBuffers(1, &ubo);
}

