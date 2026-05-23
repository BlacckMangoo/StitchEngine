#pragma once
#include <set>

#include "Texture.h"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "mesh.h"
#include "shader.h"

using UniformValue =
std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

struct Material {
    Shader *shader{};
    std::unordered_map<std::string, UniformValue>
    uniforms; // "nameInShaderCode", value
    std::unordered_map<std::string, std::pair<Texture *, Sampler *> >
    textures; //  "nameInShaderCode",Texture
};

inline std::vector<float>
GetInterleavedPosTex(const fastgltf::Asset &asset,
                     const fastgltf::Primitive &primitive) {
    std::vector<float> vertices;

    auto *positionIt = primitive.findAttribute("POSITION");
    auto *normalIt = primitive.findAttribute("NORMAL");
    auto *texCoord = primitive.findAttribute("TEXCOORD_0");

    if (positionIt == primitive.attributes.end())
        return vertices;
    if (texCoord == primitive.attributes.end())
        return vertices;
    if (normalIt == primitive.attributes.end())
        return vertices;

    auto &posAccessor = asset.accessors[positionIt->accessorIndex];
    auto &normalAccessor = asset.accessors[normalIt->accessorIndex];
    auto &texAccessor = asset.accessors[texCoord->accessorIndex];

    const size_t vertexCount = posAccessor.count;
    vertices.resize(vertexCount * 8);

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, posAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
            const size_t base = idx * 8;
            vertices[base + 0] = pos.x();
            vertices[base + 1] = pos.y();
            vertices[base + 2] = pos.z();
        });

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, normalAccessor,
        [&](fastgltf::math::fvec3 normal, std::size_t idx) {
            const size_t base = idx * 8;
            vertices[base + 3] = normal.x();
            vertices[base + 4] = normal.y();
            vertices[base + 5] = normal.z();
        });

    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
        asset, texAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
            const size_t base = idx * 8;
            vertices[base + 6] = uv.x();
            vertices[base + 7] = uv.y();
        });

    return vertices;
}

inline std::vector<uint32_t> GetIndices(const fastgltf::Asset &asset,
                                        const fastgltf::Primitive &primitive) {
    std::vector<uint32_t> indices;
    if (!primitive.indicesAccessor.has_value())
        return indices;

    auto &accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<uint32_t>(asset, accessor, indices.data());
    return indices;
}

inline std::vector<GlPrimitive>
LoadPrimitivesFromGltf(const fastgltf::Mesh &mesh,
                       const fastgltf::Asset &asset) {
    std::vector<GlPrimitive> primitives;
    for (const auto &prim: mesh.primitives) {
        auto vertexData = GetInterleavedPosTex(asset, prim);
        const auto idx = GetIndices(asset, prim);
        GlPrimitive output = {
            {MakeVertexStream(vertexData)}, idx, PosNormTexLayout
        };
        primitives.push_back(std::move(output));
    }
    return primitives;
};

struct ResourceManager {


    void LoadTexture(const std::string &path, const TextureDescription &desc,
                     const std::string &name) {
        const std::string &key = name;

        if (!gpuTextures.contains(key)) {
            TextureData data(path);

            gpuTextures[key] = std::make_shared<Texture>(data.width, data.height,
                                                         desc, data.GetData());
        }
    }

    void CreateRenderTarget(int width, int height, const TextureDescription &desc,
                            const std::string &name) {
        const std::string &key = name;

        if (!gpuTextures.contains(key)) {
            gpuTextures[key] = std::make_shared<Texture>(width, height, desc);
        }
    }

    Texture *GetTexture(const std::string &name) {
        if (gpuTextures.contains(name)) {
            return gpuTextures[name].get();
        }
        return nullptr;
    }

    void LoadSampler(const SamplerDescription &desc, const std::string &name) {
        const std::string &key = name;
        if (!samplers.contains(key)) {
            samplers[key] = std::make_shared<Sampler>(desc);
        }
    }

    Sampler *GetSampler(const std::string &name) {
        if (samplers.contains(name)) {
            return samplers[name].get();
        }
        return nullptr;
    }

    void LoadMaterial(
        const std::string &shaderName,
        const std::unordered_map<std::string, UniformValue>
        &uniforms, // example : "albedoColor", glm::vec3(1.0f, 0.0f, 0.0f)
        const std::unordered_map<std::string, std::pair<std::string, std::string> >
        &textures,
        const std::string &name) {
        const std::string &key = name;
        if (!materials.contains(key)) {
            Shader *shader = GetShader(shaderName);
            if (!shader) {
                std::cerr << "Shader not found: " << shaderName << "\n";
                return;
            }
            auto material = std::make_shared<Material>();
            material->shader = shader;

            // uniforms
            for (const auto &[uniformName, uniformValue]: uniforms) {
                material->uniforms[uniformName] =
                        uniformValue; // example : "albedoColor", glm::vec3(1.0f, 0.0f,
                // 0.0f)
            }

            for (const auto &[uniformName, textureInfo]: textures) {
                const auto &[textureName, samplerName] = textureInfo;
                Texture *texture = GetTexture(textureName);
                Sampler *sampler = GetSampler(samplerName);
                if (!texture) {
                    std::cerr << "Texture not found: " << textureName << "\n";
                    continue;
                }
                if (!sampler) {
                    std::cerr << "Sampler not found: " << samplerName << "\n";
                    continue;
                }
                material->textures[uniformName] = {
                    texture, sampler
                }; // example : "albedoMap", {Texture*, Sampler*}
            }

            materials[key] = material;
        }
    }

    Material *GetMaterial(const std::string &name) {
        if (materials.contains(name)) {
            return materials[name].get();
        }
        return nullptr;
    };

    void LoadMesh(std::vector<GlPrimitive> &&primitives,
                  const std::string &name) {
        const std::string &key = name;
        if (!meshes.contains(key)) {
            meshes[key] = std::make_shared<Mesh>(name, std::move(primitives));
        }
    }

    void LoadGLTFMesh(const std::filesystem::path &path,
                      const std::string &name) {
        fastgltf::Parser parser;
        auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);
        if (gltfData.error() != fastgltf::Error::None) {
            std::cerr << "Failed to load GLTF file\n";
        }
        auto asset = parser.loadGltf(gltfData.get(),
                                     std::filesystem::path(path).parent_path(),
                                     fastgltf::Options::LoadExternalBuffers |
                                     fastgltf::Options::GenerateMeshIndices);
        if (asset.error() != fastgltf::Error::None) {
            std::cerr << "Failed to parse GLTF file\n";
        }
        auto &gltfAsset = asset.get();
        if (gltfAsset.meshes.empty()) {
            std::cerr << "No meshes found in GLTF file\n";
        }
        // name
        for (const auto &m: gltfAsset.meshes) {
            std::cout << m.name << "loaded " << "\n";
            std::cout << "with " << m.primitives.size() << " primitives\n";
        }

        for (int i = 0; i < gltfAsset.meshes.size(); ++i) {
            auto primitives = LoadPrimitivesFromGltf(gltfAsset.meshes[i], gltfAsset);
            const auto &key = name;
            if (!primitives.empty() && !meshes.contains(key))
                meshes[key] = std::make_shared<Mesh>(gltfAsset.meshes[i].name,
                                                     std::move(primitives));
        }
    }

    Mesh *GetMesh(const std::string &name) {
        if (meshes.contains(name)) {
            return meshes[name].get();
        }
        return nullptr;
    }

    void WatchPaths() {
        for ( const auto& [path,shaderName] : shaderFilesToWatch ) {
            if ( std::filesystem::exists(path) ) {
                auto lastWriteTime = std::filesystem::last_write_time( path );
                if ( lastWriteTime != shaderFileWriteTimes[path] ) {
                    shaderFileWriteTimes[path] = lastWriteTime;
                    std::cout << "shader file edited";
                    // check if path ends with frag or vert
                    if (std::filesystem::path(path).extension() == ".vert") {
                        GetShader(shaderName)->Reload((path), (GetShader(shaderName)->fragmentShaderPath));
                    }
                    else {
                        GetShader(shaderName)->Reload((GetShader(shaderName)->vertexShaderPath), (path));
                    }
                }
            }
        }
    }

    void LoadShader(const std::string &vertPath, const std::string &fragPath,
                const std::string &name) {
        const std::string &key = name;
        if (!shaders.contains(key)) {
            shaders[key] = std::make_shared<Shader>((vertPath),
                                                    (fragPath));
            shaderFilesToWatch.insert(std::make_pair(vertPath,name));
            shaderFileWriteTimes[vertPath] = std::filesystem::last_write_time(vertPath);
            shaderFilesToWatch.insert(std::make_pair(fragPath,name));
            shaderFileWriteTimes[fragPath] = std::filesystem::last_write_time(fragPath);

        }
        std::cout << "Loaded shader: " << shaders[key] << ":"
                << shaders[key]->GetId() << "\n";
    }

    Shader *GetShader(const std::string &name) {
        if (shaders.contains(name)) {
            return shaders[name].get();
        }
        return nullptr;
    }


private:
    std::unordered_map<std::string, std::shared_ptr<Shader> > shaders;
    std::unordered_map<std::string, std::shared_ptr<Material> > materials;
    std::unordered_map<std::string, std::shared_ptr<Texture> > gpuTextures;
    std::unordered_map<std::string, std::shared_ptr<Sampler> > samplers;
    std::unordered_map<std::string, std::shared_ptr<Mesh> > meshes;


    std::set<std::pair<std::string,std::string>> shaderFilesToWatch ;
    std::unordered_map<std::string, std::filesystem::file_time_type> shaderFileWriteTimes;

};
