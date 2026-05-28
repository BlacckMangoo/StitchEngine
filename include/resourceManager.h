#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Texture.h"
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "mesh.h"
#include "shader.h"

using ID = uint64_t;

struct IdGenerator {
    static std::atomic<uint64_t> cur;

    static ID GenerateId();
};

struct ResourceManager {
    ShaderHandle LoadShader(const std::string &vertPath,
                            const std::string &fragPath,
                            const std::string &name);

    ShaderHandle LoadShader(const std::string &vertPath,
                            const std::string &fragPath);

    ShaderHandle GetShader(const std::string &name) const;

    Shader *ResolveShader(ShaderHandle handle) const;

    TextureHandle LoadTexture(const std::string &path,
                              const TextureDescription &desc,
                              const std::string &name);

    TextureHandle LoadTexture(const std::string &path,
                              const TextureDescription &desc);

    TextureHandle CreateRenderTarget(int width, int height,
                                     const TextureDescription &desc,
                                     const std::string &name);

    TextureHandle GetTexture(const std::string &name) const;

    Texture *ResolveTexture(TextureHandle handle) const;

    SamplerHandle LoadSampler(const SamplerDescription &desc,
                              const std::string &name);

    SamplerHandle LoadSampler(const SamplerDescription &desc);

    SamplerHandle GetSampler(const std::string &name) const;

    Sampler *ResolveSampler(SamplerHandle handle) const;

    MaterialHandle LoadMaterial(
        ShaderHandle shader,
        const std::unordered_map<std::string, UniformValue> &uniforms,
        const std::unordered_map<std::string, std::pair<std::string, std::string> >
            &textureBindings,
        const std::string &name);

    MaterialHandle LoadMaterial(
        ShaderHandle shader,
        const std::unordered_map<std::string, UniformValue> &uniforms,
        const std::unordered_map<std::string, std::pair<std::string, std::string> >
            &textureBindings);

    MaterialHandle GetMaterial(const std::string &name) const;

    Material *ResolveMaterial(MaterialHandle handle) const;

    MeshHandle LoadMesh(std::vector<GlPrimitive> &&primitives,
                        const std::string &name);

    MeshHandle LoadMesh(std::vector<GlPrimitive> &&primitives);

    MeshHandle LoadGLTFMesh(const std::filesystem::path &path,
                            const std::string &name);

    MeshHandle LoadGLTFMesh(const std::filesystem::path &path);

    MeshHandle GetMesh(const std::string &name) const;

    Mesh *ResolveMesh(MeshHandle handle) const;

    void WatchPaths();

private:
    std::vector<std::shared_ptr<Shader> > shaders;
    std::vector<std::shared_ptr<Material> > materials;
    std::vector<std::shared_ptr<Texture> > textures;
    std::vector<std::shared_ptr<Mesh> > meshes;
    std::vector<std::shared_ptr<Sampler> > samplers;

    std::unordered_map<std::string, ShaderHandle> shaderNames;
    std::unordered_map<std::string, MaterialHandle> materialNames;
    std::unordered_map<std::string, TextureHandle> textureNames;
    std::unordered_map<std::string, MeshHandle> meshNames;
    std::unordered_map<std::string, SamplerHandle> samplerNames;

    std::set<std::pair<std::string, std::string> > shaderFilesToWatch;
    std::unordered_map<std::string, std::filesystem::file_time_type> shaderFileWriteTimes;

    std::vector<GlPrimitive> LoadPrimitivesFromGltf(const fastgltf::Mesh &mesh,
                                                    const fastgltf::Asset &asset,
                                                    const std::filesystem::path &gltfPath);
};
