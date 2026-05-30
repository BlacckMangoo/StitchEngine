#pragma once
#include "glad/glad.h"

struct FrameBuffer
{
    FrameBuffer()
    {
        glCreateFramebuffers(1, &frameBufferObject);
    }

    void AddColorAttachment(const unsigned int textureId)
    {
        colorAttachments.push_back(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        glNamedFramebufferTexture(frameBufferObject,
                                  GL_COLOR_ATTACHMENT0 + attachmentIndex, textureId,
                                  0);

        std::cout << "color attachment: " << attachmentIndex << "\n";

        attachmentIndex++;
    }

    void AddDepthAttachmentRenderBuffer(unsigned int renderBufferId) const
    {
        glNamedFramebufferRenderbuffer(frameBufferObject, GL_DEPTH_ATTACHMENT,
                                       GL_RENDERBUFFER, renderBufferId);
    }

    void AddDepthAttachmentTexture(const unsigned int textureId) const
    {
        glNamedFramebufferTexture(frameBufferObject, GL_DEPTH_ATTACHMENT, textureId,
                                  0);
    }

    void FinalizeColorAttachments() const
    {
        glNamedFramebufferDrawBuffers(frameBufferObject,
                                      static_cast<GLsizei>(colorAttachments.size()),
                                      colorAttachments.data());
    }

    unsigned int getId() const { return frameBufferObject; }

    void Validate() const
    {
        if (glCheckNamedFramebufferStatus(frameBufferObject, GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer incomplete!\n";
    }

    ~FrameBuffer()
    {
        glDeleteFramebuffers(1, &frameBufferObject);
    }

private:
    unsigned int frameBufferObject{};
    std::vector<GLuint> colorAttachments;
    int attachmentIndex{0};
};
