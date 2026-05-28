#pragma once
#include "resourceManager.h"

struct DirectionalLight;

struct RenderObject {
    MaterialHandle material;
    glm::mat4 transform;
    MeshHandle mesh;
    int primIndex;
};

struct RenderPass {
    GLuint framebuffer; // 0 for default framebuffer

    glm::ivec2 size; // set in viewport call before rendering

    bool clearColor;
    bool clearDepth;

    glm::vec4 clearColorValue;

    bool depthTest;
    bool blending;
    bool cullFace;
};

void Render(const std::vector<RenderObject> &renderObjects, const ResourceManager &rm);

void ExecuteRenderPass(const RenderPass &pass);
