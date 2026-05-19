#pragma once
#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    [[nodiscard]] bool isValid() const { return window_ != nullptr; }
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    [[nodiscard]] bool shouldClose() const { return glfwWindowShouldClose(window_); }
    [[nodiscard]] float aspectRatio() const { return static_cast<float>(width_) / height_; }
    [[nodiscard]] GLFWwindow* get() const { return window_; }
    static void pollEvents() { glfwPollEvents(); }
    void swapBuffers() const { glfwSwapBuffers(window_); }


private:
    GLFWwindow* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};