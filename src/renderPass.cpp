#include "renderPass.h"

#include <type_traits>

void Render(const std::vector<RenderObject> &renderObjects, const ResourceManager &rm) {
    for (const auto &[material, transform, mesh, i]: renderObjects) {
        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->Bind();

        for (auto &[name, value]: rm.ResolveMaterial(material)->uniforms) {
            std::visit(
                [&]<typename T0>(T0 &&v) {
                    using T = std::decay_t<T0>;

                    if constexpr (std::is_same_v<T, int>)
                        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetInt(name, v);

                    else if constexpr (std::is_same_v<T, float>)
                        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetFloat(name, v);

                    else if constexpr (std::is_same_v<T, glm::vec2>)
                        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetVec2(name, v);

                    else if constexpr (std::is_same_v<T, glm::vec3>)
                        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetVec3(name, v);

                    else if constexpr (std::is_same_v<T, glm::vec4>)
                        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetVec4(name, v);

                    else if constexpr (std::is_same_v<T, glm::mat4>)
                        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetMat4(name, v);
                },
                value);
        }

        int textureUnit = 0;

        for (const auto &[name, texture]: rm.ResolveMaterial(material)->textures) {
            rm.ResolveTexture(texture.first)->BindTo(textureUnit);
            rm.ResolveSampler(texture.second)->BindTo(textureUnit);
            rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetInt(name, textureUnit);
            ++textureUnit;
        }

        rm.ResolveShader(rm.ResolveMaterial(material)->shader)->SetMat4("model", transform);
        rm.ResolveMesh(mesh)->primitives[i].Draw();
    }
}

void ExecuteRenderPass(const RenderPass &pass) {
    glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer);
    glViewport(0, 0, pass.size.x, pass.size.y);

    if (pass.clearColor) {
        glClearColor(pass.clearColorValue.r,
                     pass.clearColorValue.g,
                     pass.clearColorValue.b,
                     pass.clearColorValue.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    if (pass.clearDepth) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    if (pass.cullFace) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
    if (pass.depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (pass.blending) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}
