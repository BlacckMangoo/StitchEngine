#pragma once
#include "resourceManager.h"

struct DirectionalLight ;

struct RenderObject {
    Material* material;
    glm::mat4 transform;
    Mesh* mesh;

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

inline void Render(const std::vector<RenderObject>& renderObjects)
{
    for (const auto&[material, transform, mesh] : renderObjects)
    {
        material->shader->Bind();

        for (auto& [name, value] : material->uniforms)
        {
            std::visit(
                [&]<typename T0>(T0&& v)
                {
                    using T = std::decay_t<T0>;

                    if constexpr (std::is_same_v<T, int>)
                        material->shader->SetInt(name, v);

                    else if constexpr (std::is_same_v<T, float>)
                        material->shader->SetFloat(name, v);

                    else if constexpr (std::is_same_v<T, glm::vec2>)
                        material->shader->SetVec2(name, v);

                    else if constexpr (std::is_same_v<T, glm::vec3>)
                        material->shader->SetVec3(name, v);

                    else if constexpr (std::is_same_v<T, glm::vec4>)
                        material->shader->SetVec4(name, v);

                    else if constexpr (std::is_same_v<T, glm::mat4>)
                        material->shader->SetMat4(name, v);

                },
                value
            );
        }

        int textureUnit = 0;

        for (const auto& [name, texture] : material->textures)
        {
            texture.first->BindTo(textureUnit);

            texture.second->BindTo(textureUnit);

            material->shader->SetInt(
                name,
                textureUnit
            );

            ++textureUnit;
        }

        material->shader->SetMat4(
            "model",
            transform
        );

        mesh->Draw();
    }
}


inline void ExecuteRenderPass(const RenderPass& pass) {

	glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer);
	glViewport(0, 0, pass.size.x, pass.size.y);

	if (pass.clearColor)
	{
		glClearColor(pass.clearColorValue.r, pass.clearColorValue.g, pass.clearColorValue.b, pass.clearColorValue.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	if (pass.clearDepth)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	};
	if (pass.cullFace)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	else
	{
		glDisable(GL_CULL_FACE);

	};
	if (pass.depthTest)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}

	if (pass.blending)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		glDisable(GL_BLEND);
	}

};
