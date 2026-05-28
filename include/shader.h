#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>

#include <glad/glad.h>
#include <glm/glm.hpp>

std::string GetFileText(const std::filesystem::path &filePath);

class Shader {
public:
    Shader(const std::string &vertFile, const std::string &fragFile);

    ~Shader();

    void Bind() const;

    static void Unbind();

    void SetInt(const std::string &name, int value);

    void SetMat4(const std::string &name, const glm::mat4 &value);

    void SetFloat(const std::string &name, float value);

    void SetVec2(const std::string &name, const glm::vec2 &value);

    void SetVec3(const std::string &name, const glm::vec3 &value);

    void SetVec4(const std::string &name, const glm::vec4 &value);

    int GetId() const;

    unsigned int CreateProgram(const std::string &vscFile, const std::string &fscFile);

    void Reload(const std::string &newVscpath, const std::string &newFscpath);

    std::string vertexShaderPath;
    std::string fragmentShaderPath;

private:
    int GetUniformLocation(const std::string &name);

    unsigned int program = 0;

    std::unordered_map<std::string, int> uniformLocationCache;
};
