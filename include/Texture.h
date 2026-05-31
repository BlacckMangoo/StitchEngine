#pragma once
#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <optional>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stbImage.h>

struct SamplerDescription
{
    GLint minFilter{};
    GLint magFilter{};
    GLint wrapS{};
    GLint wrapT{};
    std::optional<float> maxAnisotropy;
};

struct TextureDescription
{
    GLenum target{};
    GLenum internalformat;
    bool hasMipmap;
};

struct ImageData
{
    explicit ImageData(const std::string &path)
    {
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            std::cout << "Failed to load image: " << stbi_failure_reason()
                      << std::endl;
            width = height = channels = 0;
        }
    }

    unsigned char *data;
    int width;
    int height;
    int channels;

    [[nodiscard]] unsigned char *GetData() const { return data; }

    ~ImageData()
    {
        if (data)
        {
            stbi_image_free(data);
        }
    }
};

class Texture
{
public:
    GLuint id{};
    int width{};
    int height{};
    TextureDescription textureDescription{};
    int mipLevels{};

    Texture(const int width, const int height, const TextureDescription &desc,
            const unsigned char *data = nullptr)
        : width(width), height(height), textureDescription(desc)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &id);

        if (desc.internalformat == GL_DEPTH_COMPONENT ||
            desc.internalformat == GL_DEPTH_COMPONENT16 ||
            desc.internalformat == GL_DEPTH_COMPONENT24 ||
            desc.internalformat == GL_DEPTH_COMPONENT32 ||
            desc.internalformat == GL_DEPTH_COMPONENT32F)
        {
            glTextureParameteri(id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }

        // allocate storage for texture

        if (width > 0 && height > 0)
            AllocateStorage(width, height);

        mipLevels =
            desc.hasMipmap
                ? static_cast<int>(std::floor(std::log2(std::max(width, height)))) +
                      1
                : 1;

        assert(glGetError() == GL_NO_ERROR && "Texture storage allocation failed!");
        assert(mipLevels == 1 ||
               desc.hasMipmap && mipLevels > 1 && "Invalid mip level calculation!");

        // note for future :  some things require empty textures like frame buffers
        // so  separated allocation from filling
        if (data)
        {
            SetData(data, GL_RGBA, GL_UNSIGNED_BYTE);
        }
    }

    ~Texture()
    {
        glDeleteTextures(1, &id);
    }

    void BindTo(GLuint unit) const
    {
        glBindTextureUnit(unit, id);
    }

    void GenerateMipmaps() const
    {
        if (textureDescription.hasMipmap)
        {
            glGenerateTextureMipmap(id);
        }
    }

    Texture(const Texture &) = delete;

    Texture &operator=(const Texture &) = delete;

    Texture(Texture &&other) noexcept
        : id(other.id), width(other.width), height(other.height),
          textureDescription(other.textureDescription),
          mipLevels(other.mipLevels)
    {
        other.id = 0;
    }

    Texture &operator=(Texture &&other) noexcept
    {
        if (this != &other)
        {
            glDeleteTextures(1, &id);

            id = other.id;
            width = other.width;
            height = other.height;
            textureDescription = other.textureDescription;
            mipLevels = other.mipLevels;

            other.id = 0;
        }

        return *this;
    }

private:
    void AllocateStorage(int width, int height)
    {
        mipLevels =
            textureDescription.hasMipmap
                ? static_cast<int>(std::floor(std::log2(std::max(width, height)))) +
                      1
                : 1;
        glTextureStorage2D(id, mipLevels, textureDescription.internalformat, width,
                           height);
    }

    void SetData(const unsigned char *data, const GLenum format,
                 GLenum type) const
    {
        glTextureSubImage2D(id,
                            0, // mip level
                            0, // x offset
                            0, // y offset
                            width, height, format, type, data);
        GenerateMipmaps();
    }
};

class Sampler
{
public:
    GLuint id{};

    explicit Sampler(const SamplerDescription &desc)
    {
        glCreateSamplers(1, &id);

        glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, desc.minFilter);

        glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, desc.magFilter);

        glSamplerParameteri(id, GL_TEXTURE_WRAP_S, desc.wrapS);

        glSamplerParameteri(id, GL_TEXTURE_WRAP_T, desc.wrapT);

        if (desc.maxAnisotropy)
        {
            glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY, *desc.maxAnisotropy);
        }
    }

    void BindTo(GLuint unit) const
    {
        glBindSampler(unit, id);
    }
};
