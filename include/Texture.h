#pragma once
#include <optional>
#include <string>

#include <glad/glad.h>
#include <stb/stbImage.h>

struct SamplerDescription {
    GLint minFilter{};
    GLint magFilter{};
    GLint wrapS{};
    GLint wrapT{};
    std::optional<float> maxAnisotropy;
};

struct TextureDescription {
    GLenum target{};
    GLenum internalformat;
    bool hasMipmap;
};

struct TextureData {
    explicit TextureData(const std::string &path);

    unsigned char *data;
    int width;
    int height;
    int channels;

    unsigned char *GetData() const;

    ~TextureData();
};

class Texture {
public:
    GLuint id{};
    int width{};
    int height{};
    TextureDescription textureDescription{};
    int mipLevels{};

    Texture(int width, int height, const TextureDescription &desc,
            const unsigned char *data = nullptr);

    ~Texture();

    void BindTo(GLuint unit) const;

    void GenerateMipmaps() const;

    Texture(const Texture &) = delete;

    Texture &operator=(const Texture &) = delete;

    Texture(Texture &&other) noexcept;

    Texture &operator=(Texture &&other) noexcept;

private:
    void AllocateStorage(int width, int height);

    void SetData(const unsigned char *data, GLenum format, GLenum type) const;
};

class Sampler {
public:
    GLuint id{};

    explicit Sampler(const SamplerDescription &desc);

    void BindTo(GLuint unit) const;
};
