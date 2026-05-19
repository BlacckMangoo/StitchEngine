#pragma once
#include <optional>
#include <algorithm>
#include <cmath>
#include <glad/glad.h>

struct SamplerDescription {
    GLenum minFilter{};
    GLenum magFilter{};
    GLenum wrapS{};
    GLenum wrapT{};
    std::optional<float> maxAnisotropy;
};

struct TextureDescription {
    GLenum target {};
    GLenum internalformat;
    bool hasMipmap;
};

class Texture {
public:
    GLuint id{};
    int width{};
    int height{};
    TextureDescription textureDescription{};
    int mipLevels{};

    Texture(const int width, const int height, const TextureDescription &desc) : width(width), height(height),
        textureDescription(desc) {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);

        mipLevels = desc.hasMipmap
                        ? static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1
                        : 1;

        //allocate storage for texture
        glTextureStorage2D(
            id,
            mipLevels,
            desc.internalformat,
            width,
            height
        );

        // some things require empty textures like frame buffers so we separate allocation from filling
    }

    ~Texture() {
        glDeleteTextures(1, &id);
    }

    void BindTo(GLuint unit) const {
        glBindTextureUnit(unit, id);
    }

    void SetData(const unsigned char *data, const GLenum format, GLenum type) const {
        glTextureSubImage2D(
            id,
            0, // mip level
            0, // x offset
            0, // y offset
            width,
            height,
            format,
            type,
            data
        );
    }

    void GenerateMipmaps() const {
        if (textureDescription.hasMipmap) {
            glGenerateTextureMipmap(id);
        }
    }
};

class Sampler {
public:
    GLuint id{};

    Sampler(const SamplerDescription &desc) {
        glCreateSamplers(1, &id);

        glSamplerParameteri(
            id,
            GL_TEXTURE_MIN_FILTER,
            desc.minFilter
        );

        glSamplerParameteri(
            id,
            GL_TEXTURE_MAG_FILTER,
            desc.magFilter
        );

        glSamplerParameteri(
            id,
            GL_TEXTURE_WRAP_S,
            desc.wrapS
        );

        glSamplerParameteri(
            id,
            GL_TEXTURE_WRAP_T,
            desc.wrapT
        );

        if (desc.maxAnisotropy) {
            glSamplerParameterf(
                id,
                GL_TEXTURE_MAX_ANISOTROPY,
                *desc.maxAnisotropy
            );
        }
    }

    void BindTo(GLuint unit) const {
        glBindSampler(unit, id);
    }
};
