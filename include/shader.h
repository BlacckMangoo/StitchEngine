#pragma once
#include <fstream>
#include <string>

inline std::string GetFileText(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    return {
        (std::istreambuf_iterator(file)),
        std::istreambuf_iterator<char>()
    };
};


inline unsigned int CreateProgram(const std::string &vsc, const std::string &fsc) {
    const char* vertexSrc = vsc.c_str();
    const char* fragSrc = fsc.c_str();
    const auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "Vertex shader error: " << infoLog << std::endl;
    }

    const auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragSrc, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "Fragment shader error: " << infoLog << std::endl;
    }

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Program link error: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}