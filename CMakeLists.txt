cmake_minimum_required(VERSION 4.1)
project(StitchEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(./external/glfw)
add_subdirectory(./external/glm)

add_library(glad external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

add_library(imgui
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        external/imgui
        external/imgui/backends
)

target_link_libraries(imgui PUBLIC glfw)

file(GLOB_RECURSE CPP_SOURCES src/*.cpp)
add_executable(StitchEngine ${CPP_SOURCES})

target_include_directories(StitchEngine PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(StitchEngine glfw glm imgui glad)