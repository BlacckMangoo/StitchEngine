#pragma once
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

inline std::string GetFileText(const std::filesystem::path &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    return {
        (std::istreambuf_iterator(file)),
        std::istreambuf_iterator<char>()
    };
};


class Shader {
public:
    Shader(const std::string &vertFile, const std::string &fragFile) {
        program = CreateProgram(vertFile, fragFile);
    }

    ~Shader() {
        glDeleteProgram(program);
    }

    void Bind() const {
        glUseProgram(program);
    }

    static void Unbind() {
        glUseProgram(0);
    }

    void SetInt(const std::string &name, int value) {
        glUniform1i(GetUniformLocation(name), value);
    }

    void SetMat4(const std::string &name, const glm::mat4 &value) {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void SetFloat(const std::string &name, float value) {
        glUniform1f(GetUniformLocation(name), value);
    }

    void SetVec2(const std::string &name, const glm::vec2 &value) {
        glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    void SetVec3(const std::string &name, const glm::vec3 &value) {
        glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    void SetVec4(const std::string &name, const glm::vec4 &value) {
        glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
    }

    int GetId() const {
        return program;
    }


    unsigned int CreateProgram(const std::string &vscFile, const std::string &fscFile) {
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

    void Reload(const std::string &newVscpath, const std::string &newFscpath) {
        auto newProgram = CreateProgram(newVscpath, newFscpath);
        if (newProgram != 0) {
            glDeleteProgram(program);
            program = newProgram;
            uniformLocationCache.clear();
            std::cout << "Reloading shaders..." << newVscpath << " and " << newVscpath << "\n";
        } else {
            std::cerr << "Failed to reload shader: " << vertexShaderPath << " and " << fragmentShaderPath << "\n";
        }
    }

    std::string vertexShaderPath;
    std::string fragmentShaderPath;

private:
    int GetUniformLocation(const std::string &name) {
        if (const auto it = uniformLocationCache.find(name); it != uniformLocationCache.end()) {
            return it->second;
        }
        const int location = glGetUniformLocation(program, name.c_str());
        uniformLocationCache.emplace(name, location);
        return location;
    }

    unsigned int program = 0;

    // no point in getUniformLocation again and again every frame , just get it once and then reuse every frame
    std::unordered_map<std::string, int> uniformLocationCache;
};
