#include "shader.h"

#include <fstream>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

std::string GetFileText(const std::filesystem::path &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    return {
        (std::istreambuf_iterator(file)),
        std::istreambuf_iterator<char>()
    };
}

Shader::Shader(const std::string &vertFile, const std::string &fragFile) {
    program = CreateProgram(vertFile, fragFile);
}

Shader::~Shader() {
    glDeleteProgram(program);
}

void Shader::Bind() const {
    glUseProgram(program);
}

void Shader::Unbind() {
    glUseProgram(0);
}

void Shader::SetInt(const std::string &name, int value) {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetMat4(const std::string &name, const glm::mat4 &value) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetFloat(const std::string &name, float value) {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVec2(const std::string &name, const glm::vec2 &value) {
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string &name, const glm::vec3 &value) {
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string &name, const glm::vec4 &value) {
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

int Shader::GetId() const {
    return program;
}

unsigned int Shader::CreateProgram(const std::string &vscFile, const std::string &fscFile) {
    auto vsc = GetFileText(vscFile);
    auto fsc = GetFileText(fscFile);

    if (vsc.empty() || fsc.empty()) {
        std::cerr << "Shader source code is empty!" << std::endl;
        return 0;
    }

    vertexShaderPath = vscFile;
    fragmentShaderPath = fscFile;

    const char *vertexSrc = vsc.c_str();
    const char *fragSrc = fsc.c_str();

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

    const unsigned int newProgram = glCreateProgram();
    glAttachShader(newProgram, vertexShader);
    glAttachShader(newProgram, fragmentShader);
    glLinkProgram(newProgram);

    glGetProgramiv(newProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(newProgram, 512, nullptr, infoLog);
        std::cout << "Program link error: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return newProgram;
}

void Shader::Reload(const std::string &newVscpath, const std::string &newFscpath) {
    auto newProgram = CreateProgram(newVscpath, newFscpath);
    if (newProgram != 0) {
        glDeleteProgram(program);
        program = newProgram;
        uniformLocationCache.clear();
        std::cout << "Reloading shaders..." << newVscpath << " and " << newVscpath
                  << "\n";
    } else {
        std::cerr << "Failed to reload shader: " << vertexShaderPath << " and "
                  << fragmentShaderPath << "\n";
    }
}

int Shader::GetUniformLocation(const std::string &name) {
    if (const auto it = uniformLocationCache.find(name); it != uniformLocationCache.end()) {
        return it->second;
    }
    const int location = glGetUniformLocation(program, name.c_str());
    uniformLocationCache.emplace(name, location);
    return location;
}

