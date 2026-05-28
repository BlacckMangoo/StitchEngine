#pragma once
#include "glad/glad.h"

#include <iostream>
#include <vector>

struct FrameBuffer {
    FrameBuffer();

    void AddColorAttachment(unsigned int textureId);

    void AddDepthAttachmentRenderBuffer(unsigned int renderBufferId) const;

    void AddDepthAttachmentTexture(unsigned int textureId) const;

    void FinalizeColorAttachments() const;

    unsigned int getId() const;

    void Validate() const;

    ~FrameBuffer();

private:
    unsigned int frameBufferObject{};
    std::vector<GLuint> colorAttachments;
    int attachmentIndex{0};
};
