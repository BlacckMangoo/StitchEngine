#pragma once
#include <set>
#include <filesystem>
#include <iostream>

#include "Texture.h"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "shader.h"
#include "mesh.h"
#include <atomic>

#include "glm/gtx/string_cast.hpp"

using ID = uint64_t;

struct IdGenerator
{
    static inline std::atomic<uint64_t> cur = 1;

    static ID GenerateId()
    {
        return cur++;
    }
};

inline std::vector<float>
GetInterleavedPosTexTan(const fastgltf::Asset &asset,
                     const fastgltf::Primitive &primitive)
{
    std::vector<float> vertices;

    constexpr size_t floatsPerVertex = 12; // pos(3) + normal(3) + uv(2) + tangent(4)

    auto *positionIt = primitive.findAttribute("POSITION");
    auto *normalIt = primitive.findAttribute("NORMAL");
    auto *texCoord = primitive.findAttribute("TEXCOORD_0");
    auto *tangent = primitive.findAttribute("TANGENT");
    if (positionIt == primitive.attributes.end())
        return vertices;
    if (texCoord == primitive.attributes.end())
        return vertices;
    if (normalIt == primitive.attributes.end())
        return vertices;
    if (tangent == primitive.attributes.end())
        return vertices;


    auto &posAccessor = asset.accessors[positionIt->accessorIndex];
    auto &normalAccessor = asset.accessors[normalIt->accessorIndex];
    auto &texAccessor = asset.accessors[texCoord->accessorIndex];
    auto &tanAccessor = asset.accessors[tangent->accessorIndex];


    const size_t vertexCount = posAccessor.count;
    if (normalAccessor.count != vertexCount || texAccessor.count != vertexCount || tanAccessor.count != vertexCount)
    {
        std::cerr << "GetInterleavedPosTexTan: accessor count mismatch (pos=" << vertexCount
                  << ", normal=" << normalAccessor.count
                  << ", uv=" << texAccessor.count
                  << ", tangent=" << tanAccessor.count << ")\n";
        return {};
    }

    vertices.resize(vertexCount * floatsPerVertex);

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, posAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx)
        {
            const size_t base = idx * floatsPerVertex;
            vertices[base + 0] = pos.x();
            vertices[base + 1] = pos.y();
            vertices[base + 2] = pos.z(); });

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t idx)
        {
            const size_t base = idx * floatsPerVertex;
            vertices[base + 3] = normal.x();
            vertices[base + 4] = normal.y();
            vertices[base + 5] = normal.z(); });

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
        asset, texAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx)
        {
            const size_t base = idx * floatsPerVertex;
            vertices[base + 6] = uv.x();
            vertices[base + 7] = uv.y();
        });

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
        asset, tanAccessor, [&](fastgltf::math::fvec4 tan, std::size_t idx)
        {
            const size_t base = idx * floatsPerVertex;
            vertices[base + 8] = tan.x();
            vertices[base + 9] = tan.y();
            vertices[base + 10] = tan.z();
            vertices[base + 11] = tan.w();
        });

    return vertices;
}

inline std::vector<uint32_t>
GetIndices(const fastgltf::Asset &asset, const fastgltf::Primitive &primitive)
{
    std::vector<uint32_t> indices;
    if (!primitive.indicesAccessor.has_value())
        return indices;

    auto &accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<uint32_t>(asset, accessor, indices.data());
    return indices;
}

struct ResourceManager
{
    // Returns an invalid handle if the shader couldn't be created.
    ShaderHandle LoadShader(const std::string &vertPath,
                            const std::string &fragPath,
                            const std::string &name)
    {
        // Return existing handle if already loaded.
        if (auto it = shaderNames.find(name); it != shaderNames.end())
            return it->second;

        ShaderHandle handle{static_cast<uint32_t>(shaders.size())};
        shaders.push_back(std::make_shared<Shader>(vertPath, fragPath));
        shaderNames[name] = handle;

        std::error_code ec;
        shaderFilesToWatch.insert({vertPath, name});
        shaderFileWriteTimes[vertPath] = std::filesystem::last_write_time(vertPath, ec);
        shaderFilesToWatch.insert({fragPath, name});
        shaderFileWriteTimes[fragPath] = std::filesystem::last_write_time(fragPath, ec);

        std::cout << "Loaded shader: " << name
                  << " (id=" << handle.id << ")\n";
        return handle;
    }

    ShaderHandle LoadShader(const std::string &vertPath,
                            const std::string &fragPath)
    {
        auto name = std::to_string(IdGenerator::GenerateId());
        if (auto it = shaderNames.find(name); it != shaderNames.end())
            return it->second;

        ShaderHandle handle{static_cast<uint32_t>(shaders.size())};
        shaders.push_back(std::make_shared<Shader>(vertPath, fragPath));
        shaderNames[name] = handle;

        std::error_code ec;
        shaderFilesToWatch.insert({vertPath, name});
        shaderFileWriteTimes[vertPath] = std::filesystem::last_write_time(vertPath, ec);
        shaderFilesToWatch.insert({fragPath, name});
        shaderFileWriteTimes[fragPath] = std::filesystem::last_write_time(fragPath, ec);

        std::cout << "Loaded shader: " << name
                  << " (id=" << handle.id << ")\n";
        return handle;
    }

    ShaderHandle GetShader(const std::string &name) const
    {
        if (auto it = shaderNames.find(name); it != shaderNames.end())
            return it->second;
        return {};
    }

    Shader *ResolveShader(const ShaderHandle handle) const
    {
        if (handle.IsValid() && handle.id < shaders.size())
            return shaders[handle.id].get();
        return nullptr;
    }

    TextureHandle LoadTexture(const std::string &path,
                              const TextureDescription &desc,
                              const std::string &name)
    {
        if (auto it = textureNames.find(name); it != textureNames.end())
            return it->second;

        TextureHandle handle{static_cast<uint32_t>(textures.size())};
        ImageData data(path);
        textures.push_back(std::make_shared<Texture>(data.width, data.height, desc, data.GetData()));
        textureNames[name] = handle;
        return handle;
    }

    TextureHandle LoadTexture(const std::string &path,
                              const TextureDescription &desc)
    {
        auto name = std::to_string(IdGenerator::GenerateId());
        if (auto it = textureNames.find(name); it != textureNames.end())
            return it->second;

        TextureHandle handle{static_cast<uint32_t>(textures.size())};
        ImageData data(path);
        textures.push_back(std::make_shared<Texture>(data.width, data.height, desc, data.GetData()));
        textureNames[name] = handle;
        return handle;
    }

    TextureHandle CreateRenderTarget(int width, int height, const TextureDescription &desc, const std::string &name)
    {
        if (const auto it = textureNames.find(name); it != textureNames.end())
            return it->second;

        const TextureHandle handle{static_cast<uint32_t>(textures.size())};
        const auto tex = std::make_shared<Texture>(width, height, desc);
        textures.push_back(tex);
        textureNames[name] = handle;
        return handle;
    }

    TextureHandle GetTexture(const std::string &name) const
    {
        if (const auto it = textureNames.find(name); it != textureNames.end())
            return it->second;
        return {};
    }

    Texture *ResolveTexture(const TextureHandle handle) const
    {
        if (handle.IsValid() && handle.id < textures.size())
            return textures[handle.id].get();
        return nullptr;
    }

    SamplerHandle LoadSampler(const SamplerDescription &desc,
                              const std::string &name)
    {
        if (auto it = samplerNames.find(name); it != samplerNames.end())
            return it->second;

        SamplerHandle handle{static_cast<uint32_t>(samplers.size())};
        samplers.push_back(std::make_shared<Sampler>(desc));
        samplerNames[name] = handle;
        return handle;
    }

    SamplerHandle LoadSampler(const SamplerDescription &desc)
    {
        auto name = std::to_string(IdGenerator::GenerateId());

        if (auto it = samplerNames.find(name); it != samplerNames.end())
            return it->second;

        SamplerHandle handle{static_cast<uint32_t>(samplers.size())};
        samplers.push_back(std::make_shared<Sampler>(desc));
        samplerNames[name] = handle;
        return handle;
    }

    SamplerHandle GetSampler(const std::string &name) const
    {
        if (auto it = samplerNames.find(name); it != samplerNames.end())
            return it->second;
        return {};
    }

    Sampler *ResolveSampler(SamplerHandle handle) const
    {
        if (handle.IsValid() && handle.id < samplers.size())
            return samplers[handle.id].get();
        return nullptr;
    }

    // uniforms  — e.g. { "albedoColor", glm::vec3(1,0,0) }
    // textures  — e.g. { "albedoMap", { "myTex", "mySampler" } }

    MaterialHandle LoadMaterial(ShaderHandle shader,
                                const std::unordered_map<std::string, UniformValue> &uniforms,
                                const std::unordered_map<std::string, std::pair<std::string, std::string>> &
                                    textureBindings,
                                const std::string &name)
    {
        if (auto it = materialNames.find(name); it != materialNames.end())
            return it->second;

        auto material = std::make_shared<Material>();
        material->shader = shader;
        for (const auto &[uniformName, uniformValue] : uniforms)
            material->uniforms[uniformName] = uniformValue;

        for (const auto &[uniformName, texInfo] : textureBindings)
        {
            const auto &[texName, samplerName] = texInfo;

            Texture *texture = ResolveTexture(GetTexture(texName));
            Sampler *sampler = ResolveSampler(GetSampler(samplerName));

            if (!texture)
            {
                std::cerr << "LoadMaterial: texture not found: " << texName << "\n";
                continue;
            }
            if (!sampler)
            {
                std::cerr << "LoadMaterial: sampler not found: " << samplerName << "\n";
                continue;
            }

            material->textures[uniformName] = std::make_pair(GetTexture(texName), GetSampler(samplerName));
        }

        const MaterialHandle handle{static_cast<uint32_t>(materials.size())};
        materials.push_back(std::move(material));
        materialNames[name] = handle;

        return handle;
    }

    MaterialHandle LoadMaterial(ShaderHandle shader,
                                const std::unordered_map<std::string, UniformValue> &uniforms,
                                const std::unordered_map<std::string, std::pair<std::string, std::string>> &
                                    textureBindings)
    {
        auto name = std::to_string(IdGenerator::GenerateId());

        if (auto it = materialNames.find(name); it != materialNames.end())
            return it->second;

        auto material = std::make_shared<Material>();
        material->shader = shader;
        for (const auto &[uniformName, uniformValue] : uniforms)
            material->uniforms[uniformName] = uniformValue;

        for (const auto &[uniformName, texInfo] : textureBindings)
        {
            const auto &[texName, samplerName] = texInfo;

            Texture *texture = ResolveTexture(GetTexture(texName));
            Sampler *sampler = ResolveSampler(GetSampler(samplerName));

            if (!texture)
            {
                std::cerr << "LoadMaterial: texture not found: " << texName << "\n";
                continue;
            }
            if (!sampler)
            {
                std::cerr << "LoadMaterial: sampler not found: " << samplerName << "\n";
                continue;
            }

            material->textures[uniformName] = std::make_pair(GetTexture(texName), GetSampler(samplerName));
        }

        const MaterialHandle handle{static_cast<uint32_t>(materials.size())};
        materials.push_back(std::move(material));
        materialNames[name] = handle;

        return handle;
    }

    MaterialHandle GetMaterial(const std::string &name) const
    {
        if (auto it = materialNames.find(name); it != materialNames.end())
            return it->second;
        return {};
    }

    Material *ResolveMaterial(MaterialHandle handle) const
    {
        if (handle.IsValid() && handle.id < materials.size())
            return materials[handle.id].get();
        return nullptr;
    }

    MeshHandle LoadMesh(std::vector<GlPrimitive> &&primitives,
                        const std::string &name)
    {
        if (const auto it = meshNames.find(name); it != meshNames.end())
            return it->second;

        MeshHandle handle{static_cast<uint32_t>(meshes.size())};
        meshes.push_back(std::make_shared<Mesh>(name, std::move(primitives)));
        meshNames[name] = handle;
        return handle;
    }

    MeshHandle LoadMesh(std::vector<GlPrimitive> &&primitives)
    {
        auto name = std::to_string(IdGenerator::GenerateId());
        if (const auto it = meshNames.find(name); it != meshNames.end())
            return it->second;

        MeshHandle handle{static_cast<uint32_t>(meshes.size())};
        meshes.push_back(std::make_shared<Mesh>(name, std::move(primitives)));
        meshNames[name] = handle;
        return handle;
    }

    MeshHandle LoadGLTFMesh(const std::filesystem::path &path,
                            const std::string &name)
    {
        if (auto it = meshNames.find(name); it != meshNames.end())
            return it->second;

        fastgltf::Parser parser;
        auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);
        if (gltfData.error() != fastgltf::Error::None)
        {
            std::cerr << "LoadGLTFMesh: failed to load file: " << path << "\n";
            return {};
        }

        auto asset = parser.loadGltf(
            gltfData.get(), path.parent_path(),
            fastgltf::Options::LoadExternalBuffers |
                fastgltf::Options::GenerateMeshIndices);

        if (asset.error() != fastgltf::Error::None)
        {
            std::cerr << "LoadGLTFMesh: failed to parse GLTF: " << path << "\n";
            return {};
        }

        auto &gltfAsset = asset.get();
        if (gltfAsset.meshes.empty())
        {
            std::cerr << "LoadGLTFMesh: no meshes in file: " << path << "\n";
            return {};
        }

        MeshHandle firstHandle{};
        for (size_t i = 0; i < gltfAsset.meshes.size(); ++i)
        {
            const auto &m = gltfAsset.meshes[i];
            std::cout << m.name << " loaded (" << m.primitives.size() << " primitives)\n";

            auto primitives = LoadPrimitivesFromGltf(m, gltfAsset, path);
            if (primitives.empty())
                continue;

            const std::string &key = (i == 0) ? name : std::string(m.name);

            if (meshNames.contains(key))
                continue;

            MeshHandle handle{static_cast<uint32_t>(meshes.size())};
            meshes.push_back(std::make_shared<Mesh>(m.name, std::move(primitives)));
            meshNames[key] = handle;

            if (i == 0)
                firstHandle = handle;
        }

        return firstHandle;
    }

    MeshHandle LoadGLTFMesh(const std::filesystem::path &path)
    {
        auto name = std::to_string(IdGenerator::GenerateId());

        if (auto it = meshNames.find(name); it != meshNames.end())
            return it->second;

        fastgltf::Parser parser;
        auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);
        if (gltfData.error() != fastgltf::Error::None)
        {
            std::cerr << "LoadGLTFMesh: failed to load file: " << path << "\n";
            return {};
        }

        auto asset = parser.loadGltf(
            gltfData.get(), path.parent_path(),
            fastgltf::Options::LoadExternalBuffers |
                fastgltf::Options::GenerateMeshIndices);

        if (asset.error() != fastgltf::Error::None)
        {
            std::cerr << "LoadGLTFMesh: failed to parse GLTF: " << path << "\n";
            return {};
        }

        auto &gltfAsset = asset.get();
        if (gltfAsset.meshes.empty())
        {
            std::cerr << "LoadGLTFMesh: no meshes in file: " << path << "\n";
            return {};
        }

        MeshHandle firstHandle{};
        for (size_t i = 0; i < gltfAsset.meshes.size(); ++i)
        {
            const auto &m = gltfAsset.meshes[i];
            std::cout << m.name << " loaded (" << m.primitives.size() << " primitives)\n";

            auto primitives = LoadPrimitivesFromGltf(m, gltfAsset, path);
            if (primitives.empty())
                continue;

            const std::string &key = (i == 0) ? name : std::string(m.name);

            if (meshNames.contains(key))
                continue;

            MeshHandle handle{static_cast<uint32_t>(meshes.size())};
            meshes.push_back(std::make_shared<Mesh>(m.name, std::move(primitives)));
            meshNames[key] = handle;

            if (i == 0)
                firstHandle = handle;
        }

        return firstHandle;
    }

    MeshHandle GetMesh(const std::string &name) const
    {
        if (auto it = meshNames.find(name); it != meshNames.end())
            return it->second;
        return {};
    }

    Mesh *ResolveMesh(MeshHandle handle) const
    {
        if (handle.IsValid() && handle.id < meshes.size())
            return meshes[handle.id].get();
        return nullptr;
    }

    void WatchPaths()
    {
        for (const auto &[path, shaderName] : shaderFilesToWatch)
        {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec))
                continue;

            auto lastWriteTime = std::filesystem::last_write_time(path, ec);
            if (ec)
                continue;

            if (lastWriteTime == shaderFileWriteTimes[path])
                continue;

            shaderFileWriteTimes[path] = lastWriteTime;
            std::cout << "Shader file changed: " << path << "\n";

            Shader *shader = ResolveShader(GetShader(shaderName));
            if (!shader)
                continue;

            if (std::filesystem::path(path).extension() == ".vert")
                shader->Reload(path, shader->fragmentShaderPath);
            else
                shader->Reload(shader->vertexShaderPath, path);
        }
    }

private:
    std::vector<std::shared_ptr<Shader>> shaders;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Texture>> textures;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Sampler>> samplers;

    std::unordered_map<std::string, ShaderHandle> shaderNames;
    std::unordered_map<std::string, MaterialHandle> materialNames;
    std::unordered_map<std::string, TextureHandle> textureNames;
    std::unordered_map<std::string, MeshHandle> meshNames;
    std::unordered_map<std::string, SamplerHandle> samplerNames;

    std::set<std::pair<std::string, std::string>> shaderFilesToWatch;
    std::unordered_map<std::string, std::filesystem::file_time_type> shaderFileWriteTimes;

    std::vector<GlPrimitive> LoadPrimitivesFromGltf(const fastgltf::Mesh &mesh,
                                                    const fastgltf::Asset &asset,
                                                    const std::filesystem::path &gltfPath)
    {
        std::vector<GlPrimitive> primitives;
        primitives.reserve(mesh.primitives.size());

        for (const auto &prim : mesh.primitives)
        {
            auto vertexData = GetInterleavedPosTexTan(asset, prim);
            if (vertexData.empty())
                continue;

            auto indices = GetIndices(asset, prim);

            GlPrimitive output{{MakeVertexStream(vertexData)}, indices, PosNormTexTanLayout};

            // Always create a material so the render path doesn't crash on invalid handles.
            auto primMat = LoadMaterial(ShaderHandle{}, {}, {}, std::to_string(IdGenerator::GenerateId()));
            output.material = primMat;
            Material *primMaterial = ResolveMaterial(primMat);

            if (prim.materialIndex.has_value() && primMaterial != nullptr)
            {
                const auto &material = asset.materials[prim.materialIndex.value()];
                const auto &pbr = material.pbrData;

                auto baseColorFactor = glm::vec4(pbr.baseColorFactor.x(), pbr.baseColorFactor.y(), pbr.baseColorFactor.z(), pbr.baseColorFactor.w());
                float metallicFactor = pbr.metallicFactor;
                float roughnessFactor = pbr.roughnessFactor;


                // set PBR uniforms
                primMaterial->uniforms["baseColorFactor"] = baseColorFactor;
                primMaterial->uniforms["metallicFactor"] = metallicFactor;
                primMaterial->uniforms["roughnessFactor"] = roughnessFactor;

                // Read wrap/filter from the GLTF sampler if present
                SamplerDescription sd{
                    .minFilter = GL_LINEAR_MIPMAP_LINEAR,
                    .magFilter = GL_LINEAR,
                    .wrapS = GL_REPEAT,
                    .wrapT = GL_REPEAT,
                    .maxAnisotropy = std::nullopt,
                };

                // Prefer sampler settings from baseColorTexture (or metallicRoughnessTexture) if present.
                auto applySamplerFromTextureIndex = [&](size_t textureIndex)
                {
                    if (textureIndex >= asset.textures.size())
                        return;
                    const auto &t = asset.textures[textureIndex];
                    if (!t.samplerIndex.has_value())
                        return;
                    const size_t samplerIndex = t.samplerIndex.value();
                    if (samplerIndex >= asset.samplers.size())
                        return;
                    const auto &gltfSampler = asset.samplers[samplerIndex];
                    if (gltfSampler.wrapS == fastgltf::Wrap::ClampToEdge)
                        sd.wrapS = GL_CLAMP_TO_EDGE;
                    if (gltfSampler.wrapT == fastgltf::Wrap::ClampToEdge)
                        sd.wrapT = GL_CLAMP_TO_EDGE;
                    if (gltfSampler.wrapS == fastgltf::Wrap::MirroredRepeat)
                        sd.wrapS = GL_MIRRORED_REPEAT;
                    if (gltfSampler.wrapT == fastgltf::Wrap::MirroredRepeat)
                        sd.wrapT = GL_MIRRORED_REPEAT;
                };

                if (pbr.baseColorTexture.has_value())
                    applySamplerFromTextureIndex(pbr.baseColorTexture->textureIndex);
                else if (pbr.metallicRoughnessTexture.has_value())
                    applySamplerFromTextureIndex(pbr.metallicRoughnessTexture->textureIndex);

                auto sampler = LoadSampler(sd, std::to_string(IdGenerator::GenerateId()));

                if (pbr.baseColorTexture.has_value())
                {
                    const size_t texIndex = pbr.baseColorTexture->textureIndex;
                    if (texIndex >= asset.textures.size())
                    {
                        std::cerr << "LoadPrimitivesFromGltf: baseColorTexture index out of range: " << texIndex << "\n";
                    }
                    else
                    {
                        const auto &tex = asset.textures[texIndex];
                        if (!tex.imageIndex.has_value() || tex.imageIndex.value() >= asset.images.size())
                        {
                            std::cerr << "LoadPrimitivesFromGltf: baseColorTexture has invalid imageIndex\n";
                        }
                        else
                        {
                            const auto &image = asset.images[tex.imageIndex.value()];

                            if (auto *uri = std::get_if<fastgltf::sources::URI>(&image.data))
                            {
                                auto texPath = gltfPath.parent_path() / uri->uri.fspath();
                                auto baseColorTex = LoadTexture(texPath.string(), {.internalformat = GL_RGBA8, .hasMipmap = true});
                                primMaterial->textures["baseColor"] = std::make_pair(baseColorTex, sampler);
                            }
                        }
                    }
                }
                if (pbr.metallicRoughnessTexture.has_value())
                {
                    const size_t texIndex = pbr.metallicRoughnessTexture->textureIndex;
                    if (texIndex >= asset.textures.size())
                    {
                        std::cerr << "LoadPrimitivesFromGltf: metallicRoughnessTexture index out of range: " << texIndex << "\n";
                    }
                    else
                    {
                        const auto &tex = asset.textures[texIndex];
                        if (!tex.imageIndex.has_value() || tex.imageIndex.value() >= asset.images.size())
                        {
                            std::cerr << "LoadPrimitivesFromGltf: metallicRoughnessTexture has invalid imageIndex\n";
                        }
                        else
                        {
                            const auto &image = asset.images[tex.imageIndex.value()];

                            if (auto *uri = std::get_if<fastgltf::sources::URI>(&image.data))
                            {
                                auto texPath = gltfPath.parent_path() / uri->uri.fspath();
                                auto metallicRoughnessTex = LoadTexture(texPath.string(), {.internalformat = GL_RGBA8, .hasMipmap = true});
                                primMaterial->textures["metallicRoughness"] = std::make_pair(metallicRoughnessTex, sampler);
                            }
                        }
                    }
                }

                if (material.occlusionTexture.has_value())
                {
                    const size_t texIndex = material.occlusionTexture->textureIndex;
                    if (texIndex >= asset.textures.size())
                    {
                        std::cerr << "LoadPrimitivesFromGltf: occlusionTexture index out of range: " << texIndex << "\n";
                    }
                    else
                    {
                        const auto &tex = asset.textures[texIndex];
                        if (!tex.imageIndex.has_value() || tex.imageIndex.value() >= asset.images.size())
                        {
                            std::cerr << "LoadPrimitivesFromGltf: occlusionTexture has invalid imageIndex\n";
                        }
                        else
                        {
                            const auto &image = asset.images[tex.imageIndex.value()];

                            if (auto *uri = std::get_if<fastgltf::sources::URI>(&image.data))
                            {
                                auto texPath = gltfPath.parent_path() / uri->uri.fspath();
                                auto occlusionTex = LoadTexture(texPath.string(), {.internalformat = GL_RGBA8, .hasMipmap = true});
                                primMaterial->textures["occlusion"] = std::make_pair(occlusionTex, sampler);
                            }
                        }
                    }
                }

                if (material.normalTexture.has_value())
                {
                    const size_t texIndex = material.normalTexture->textureIndex;
                    if (texIndex >= asset.textures.size())
                    {
                        std::cerr << "LoadPrimitivesFromGltf: normalTexture index out of range: " << texIndex << "\n";
                    }
                    else
                    {
                        const auto &tex = asset.textures[texIndex];
                        if (!tex.imageIndex.has_value() || tex.imageIndex.value() >= asset.images.size())
                        {
                            std::cerr << "LoadPrimitivesFromGltf: normalTexture has invalid imageIndex\n";
                        }
                        else
                        {
                            const auto &image = asset.images[tex.imageIndex.value()];

                            if (auto *uri = std::get_if<fastgltf::sources::URI>(&image.data))
                            {
                                auto texPath = gltfPath.parent_path() / uri->uri.fspath();
                                auto normalTex = LoadTexture(texPath.string(), {.internalformat = GL_RGBA8, .hasMipmap = true});
                                primMaterial->textures["normalMap"] = std::make_pair(normalTex, sampler);
                            }
                        }
                    }
                }
            }

            primitives.push_back(std::move(output));
        }
        return primitives;
    }
};
