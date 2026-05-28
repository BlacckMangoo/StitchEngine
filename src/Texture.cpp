#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

TextureData::TextureData(const std::string &path) {
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
        width = height = channels = 0;
    }
}

unsigned char *TextureData::GetData() const {
    return data;
}

TextureData::~TextureData() {
    if (data) {
        stbi_image_free(data);
    }
}

Texture::Texture(int width, int height, const TextureDescription &desc,
                 const unsigned char *data)
    : width(width), height(height), textureDescription(desc) {
    glCreateTextures(GL_TEXTURE_2D, 1, &id);

    if (width > 0 && height > 0) {
        AllocateStorage(width, height);
    }

    mipLevels = desc.hasMipmap
                    ? static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1
                    : 1;

    assert(glGetError() == GL_NO_ERROR && "Texture storage allocation failed!");
    assert(mipLevels == 1 ||
           (desc.hasMipmap && mipLevels > 1 && "Invalid mip level calculation!"));

    if (data) {
        SetData(data, GL_RGBA, GL_UNSIGNED_BYTE);
    }
}

Texture::~Texture() {
    glDeleteTextures(1, &id);
}

void Texture::BindTo(GLuint unit) const {
    glBindTextureUnit(unit, id);
}

void Texture::GenerateMipmaps() const {
    if (textureDescription.hasMipmap) {
        glGenerateTextureMipmap(id);
    }
}

Texture::Texture(Texture &&other) noexcept
    : id(other.id),
      width(other.width),
      height(other.height),
      textureDescription(other.textureDescription),
      mipLevels(other.mipLevels) {
    other.id = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept {
    if (this != &other) {
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

void Texture::AllocateStorage(int width, int height) {
    mipLevels = textureDescription.hasMipmap
                    ? static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1
                    : 1;
    glTextureStorage2D(id, mipLevels, textureDescription.internalformat, width, height);
}

void Texture::SetData(const unsigned char *data, GLenum format, GLenum type) const {
    glTextureSubImage2D(id,
                        0,
                        0,
                        0,
                        width,
                        height,
                        format,
                        type,
                        data);
    GenerateMipmaps();
}

Sampler::Sampler(const SamplerDescription &desc) {
    glCreateSamplers(1, &id);

    glSamplerParameteri(id, GL_TEXTURE_MIN_FILTER, desc.minFilter);

    glSamplerParameteri(id, GL_TEXTURE_MAG_FILTER, desc.magFilter);

    glSamplerParameteri(id, GL_TEXTURE_WRAP_S, desc.wrapS);

    glSamplerParameteri(id, GL_TEXTURE_WRAP_T, desc.wrapT);

    if (desc.maxAnisotropy) {
        glSamplerParameterf(id, GL_TEXTURE_MAX_ANISOTROPY, *desc.maxAnisotropy);
    }
}

void Sampler::BindTo(GLuint unit) const {
    glBindSampler(unit, id);
}

