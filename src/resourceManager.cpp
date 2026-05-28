#include "resourceManager.h"

#include <iostream>

#include "glm/gtx/string_cast.hpp"

namespace {
std::vector<float> GetInterleavedPosTex(const fastgltf::Asset &asset,
                                        const fastgltf::Primitive &primitive) {
    std::vector<float> vertices;

    auto *positionIt = primitive.findAttribute("POSITION");
    auto *normalIt = primitive.findAttribute("NORMAL");
    auto *texCoord = primitive.findAttribute("TEXCOORD_0");

    if (positionIt == primitive.attributes.end()) return vertices;
    if (texCoord == primitive.attributes.end()) return vertices;
    if (normalIt == primitive.attributes.end()) return vertices;

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
        asset, normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t idx) {
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

std::vector<uint32_t> GetIndices(const fastgltf::Asset &asset,
                                 const fastgltf::Primitive &primitive) {
    std::vector<uint32_t> indices;
    if (!primitive.indicesAccessor.has_value()) return indices;

    auto &accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<uint32_t>(asset, accessor, indices.data());
    return indices;
}
} // namespace

std::atomic<uint64_t> IdGenerator::cur = 1;

ID IdGenerator::GenerateId() {
    return cur++;
}

ShaderHandle ResourceManager::LoadShader(const std::string &vertPath,
                                         const std::string &fragPath,
                                         const std::string &name) {
    if (auto it = shaderNames.find(name); it != shaderNames.end())
        return it->second;

    ShaderHandle handle{static_cast<uint32_t>(shaders.size())};
    shaders.push_back(std::make_shared<Shader>(vertPath, fragPath));
    shaderNames[name] = handle;

    shaderFilesToWatch.insert({vertPath, name});
    shaderFileWriteTimes[vertPath] = std::filesystem::last_write_time(vertPath);
    shaderFilesToWatch.insert({fragPath, name});
    shaderFileWriteTimes[fragPath] = std::filesystem::last_write_time(fragPath);

    std::cout << "Loaded shader: " << name << " (id=" << handle.id << ")\n";
    return handle;
}

ShaderHandle ResourceManager::LoadShader(const std::string &vertPath,
                                         const std::string &fragPath) {
    auto name = std::to_string(IdGenerator::GenerateId());
    if (auto it = shaderNames.find(name); it != shaderNames.end())
        return it->second;

    ShaderHandle handle{static_cast<uint32_t>(shaders.size())};
    shaders.push_back(std::make_shared<Shader>(vertPath, fragPath));
    shaderNames[name] = handle;

    shaderFilesToWatch.insert({vertPath, name});
    shaderFileWriteTimes[vertPath] = std::filesystem::last_write_time(vertPath);
    shaderFilesToWatch.insert({fragPath, name});
    shaderFileWriteTimes[fragPath] = std::filesystem::last_write_time(fragPath);

    std::cout << "Loaded shader: " << name << " (id=" << handle.id << ")\n";
    return handle;
}

ShaderHandle ResourceManager::GetShader(const std::string &name) const {
    if (auto it = shaderNames.find(name); it != shaderNames.end())
        return it->second;
    return {};
}

Shader *ResourceManager::ResolveShader(const ShaderHandle handle) const {
    if (handle.IsValid() && handle.id < shaders.size())
        return shaders[handle.id].get();
    return nullptr;
}

TextureHandle ResourceManager::LoadTexture(const std::string &path,
                                           const TextureDescription &desc,
                                           const std::string &name) {
    if (auto it = textureNames.find(name); it != textureNames.end())
        return it->second;

    TextureHandle handle{static_cast<uint32_t>(textures.size())};
    TextureData data(path);
    textures.push_back(
        std::make_shared<Texture>(data.width, data.height, desc, data.GetData()));
    textureNames[name] = handle;
    return handle;
}

TextureHandle ResourceManager::LoadTexture(const std::string &path,
                                           const TextureDescription &desc) {
    auto name = std::to_string(IdGenerator::GenerateId());
    if (auto it = textureNames.find(name); it != textureNames.end())
        return it->second;

    TextureHandle handle{static_cast<uint32_t>(textures.size())};
    TextureData data(path);
    textures.push_back(
        std::make_shared<Texture>(data.width, data.height, desc, data.GetData()));
    textureNames[name] = handle;
    return handle;
}

TextureHandle ResourceManager::CreateRenderTarget(int width,
                                                  int height,
                                                  const TextureDescription &desc,
                                                  const std::string &name) {
    if (const auto it = textureNames.find(name); it != textureNames.end())
        return it->second;

    const TextureHandle handle{static_cast<uint32_t>(textures.size())};
    textures.push_back(std::make_shared<Texture>(width, height, desc));
    textureNames[name] = handle;
    return handle;
}

TextureHandle ResourceManager::GetTexture(const std::string &name) const {
    if (const auto it = textureNames.find(name); it != textureNames.end())
        return it->second;
    return {};
}

Texture *ResourceManager::ResolveTexture(const TextureHandle handle) const {
    if (handle.IsValid() && handle.id < textures.size())
        return textures[handle.id].get();
    return nullptr;
}

SamplerHandle ResourceManager::LoadSampler(const SamplerDescription &desc,
                                           const std::string &name) {
    if (auto it = samplerNames.find(name); it != samplerNames.end())
        return it->second;

    SamplerHandle handle{static_cast<uint32_t>(samplers.size())};
    samplers.push_back(std::make_shared<Sampler>(desc));
    samplerNames[name] = handle;
    return handle;
}

SamplerHandle ResourceManager::LoadSampler(const SamplerDescription &desc) {
    auto name = std::to_string(IdGenerator::GenerateId());

    if (auto it = samplerNames.find(name); it != samplerNames.end())
        return it->second;

    SamplerHandle handle{static_cast<uint32_t>(samplers.size())};
    samplers.push_back(std::make_shared<Sampler>(desc));
    samplerNames[name] = handle;
    return handle;
}

SamplerHandle ResourceManager::GetSampler(const std::string &name) const {
    if (auto it = samplerNames.find(name); it != samplerNames.end())
        return it->second;
    return {};
}

Sampler *ResourceManager::ResolveSampler(SamplerHandle handle) const {
    if (handle.IsValid() && handle.id < samplers.size())
        return samplers[handle.id].get();
    return nullptr;
}

MaterialHandle ResourceManager::LoadMaterial(
    ShaderHandle shader,
    const std::unordered_map<std::string, UniformValue> &uniforms,
    const std::unordered_map<std::string, std::pair<std::string, std::string> >
        &textureBindings,
    const std::string &name) {
    if (auto it = materialNames.find(name); it != materialNames.end())
        return it->second;

    auto material = std::make_shared<Material>();
    material->shader = shader;
    for (const auto &[uniformName, uniformValue]: uniforms)
        material->uniforms[uniformName] = uniformValue;

    for (const auto &[uniformName, texInfo]: textureBindings) {
        const auto &[texName, samplerName] = texInfo;

        Texture *texture = ResolveTexture(GetTexture(texName));
        Sampler *sampler = ResolveSampler(GetSampler(samplerName));

        if (!texture) {
            std::cerr << "LoadMaterial: texture not found: " << texName << "\n";
            continue;
        }
        if (!sampler) {
            std::cerr << "LoadMaterial: sampler not found: " << samplerName << "\n";
            continue;
        }

        material->textures[uniformName] =
            std::make_pair(GetTexture(texName), GetSampler(samplerName));
    }

    const MaterialHandle handle{static_cast<uint32_t>(materials.size())};
    materials.push_back(std::move(material));
    materialNames[name] = handle;

    return handle;
}

MaterialHandle ResourceManager::LoadMaterial(
    ShaderHandle shader,
    const std::unordered_map<std::string, UniformValue> &uniforms,
    const std::unordered_map<std::string, std::pair<std::string, std::string> >
        &textureBindings) {
    auto name = std::to_string(IdGenerator::GenerateId());

    if (auto it = materialNames.find(name); it != materialNames.end())
        return it->second;

    auto material = std::make_shared<Material>();
    material->shader = shader;
    for (const auto &[uniformName, uniformValue]: uniforms)
        material->uniforms[uniformName] = uniformValue;

    for (const auto &[uniformName, texInfo]: textureBindings) {
        const auto &[texName, samplerName] = texInfo;

        Texture *texture = ResolveTexture(GetTexture(texName));
        Sampler *sampler = ResolveSampler(GetSampler(samplerName));

        if (!texture) {
            std::cerr << "LoadMaterial: texture not found: " << texName << "\n";
            continue;
        }
        if (!sampler) {
            std::cerr << "LoadMaterial: sampler not found: " << samplerName << "\n";
            continue;
        }

        material->textures[uniformName] =
            std::make_pair(GetTexture(texName), GetSampler(samplerName));
    }

    const MaterialHandle handle{static_cast<uint32_t>(materials.size())};
    materials.push_back(std::move(material));
    materialNames[name] = handle;

    return handle;
}

MaterialHandle ResourceManager::GetMaterial(const std::string &name) const {
    if (auto it = materialNames.find(name); it != materialNames.end())
        return it->second;
    return {};
}

Material *ResourceManager::ResolveMaterial(MaterialHandle handle) const {
    if (handle.IsValid() && handle.id < materials.size())
        return materials[handle.id].get();
    return nullptr;
}

MeshHandle ResourceManager::LoadMesh(std::vector<GlPrimitive> &&primitives,
                                     const std::string &name) {
    if (const auto it = meshNames.find(name); it != meshNames.end())
        return it->second;

    MeshHandle handle{static_cast<uint32_t>(meshes.size())};
    meshes.push_back(std::make_shared<Mesh>(name, std::move(primitives)));
    meshNames[name] = handle;
    return handle;
}

MeshHandle ResourceManager::LoadMesh(std::vector<GlPrimitive> &&primitives) {
    auto name = std::to_string(IdGenerator::GenerateId());
    if (const auto it = meshNames.find(name); it != meshNames.end())
        return it->second;

    MeshHandle handle{static_cast<uint32_t>(meshes.size())};
    meshes.push_back(std::make_shared<Mesh>(name, std::move(primitives)));
    meshNames[name] = handle;
    return handle;
}

MeshHandle ResourceManager::LoadGLTFMesh(const std::filesystem::path &path,
                                         const std::string &name) {
    if (auto it = meshNames.find(name); it != meshNames.end())
        return it->second;

    fastgltf::Parser parser;
    auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);
    if (gltfData.error() != fastgltf::Error::None) {
        std::cerr << "LoadGLTFMesh: failed to load file: " << path << "\n";
        return {};
    }

    auto asset = parser.loadGltf(
        gltfData.get(),
        path.parent_path(),
        fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::GenerateMeshIndices);

    if (asset.error() != fastgltf::Error::None) {
        std::cerr << "LoadGLTFMesh: failed to parse GLTF: " << path << "\n";
        return {};
    }

    auto &gltfAsset = asset.get();
    if (gltfAsset.meshes.empty()) {
        std::cerr << "LoadGLTFMesh: no meshes in file: " << path << "\n";
        return {};
    }

    MeshHandle firstHandle{};
    for (size_t i = 0; i < gltfAsset.meshes.size(); ++i) {
        const auto &m = gltfAsset.meshes[i];
        std::cout << m.name << " loaded (" << m.primitives.size() << " primitives)\n";

        auto primitives = LoadPrimitivesFromGltf(m, gltfAsset, path);
        if (primitives.empty()) continue;

        const std::string &key = (i == 0) ? name : std::string(m.name);

        if (meshNames.contains(key)) continue;

        MeshHandle handle{static_cast<uint32_t>(meshes.size())};
        meshes.push_back(std::make_shared<Mesh>(m.name, std::move(primitives)));
        meshNames[key] = handle;

        if (i == 0) firstHandle = handle;
    }

    return firstHandle;
}

MeshHandle ResourceManager::LoadGLTFMesh(const std::filesystem::path &path) {
    auto name = std::to_string(IdGenerator::GenerateId());

    if (auto it = meshNames.find(name); it != meshNames.end())
        return it->second;

    fastgltf::Parser parser;
    auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);
    if (gltfData.error() != fastgltf::Error::None) {
        std::cerr << "LoadGLTFMesh: failed to load file: " << path << "\n";
        return {};
    }

    auto asset = parser.loadGltf(
        gltfData.get(),
        path.parent_path(),
        fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::GenerateMeshIndices);

    if (asset.error() != fastgltf::Error::None) {
        std::cerr << "LoadGLTFMesh: failed to parse GLTF: " << path << "\n";
        return {};
    }

    auto &gltfAsset = asset.get();
    if (gltfAsset.meshes.empty()) {
        std::cerr << "LoadGLTFMesh: no meshes in file: " << path << "\n";
        return {};
    }

    MeshHandle firstHandle{};
    for (size_t i = 0; i < gltfAsset.meshes.size(); ++i) {
        const auto &m = gltfAsset.meshes[i];
        std::cout << m.name << " loaded (" << m.primitives.size() << " primitives)\n";

        auto primitives = LoadPrimitivesFromGltf(m, gltfAsset, path);
        if (primitives.empty()) continue;

        const std::string &key = (i == 0) ? name : std::string(m.name);

        if (meshNames.contains(key)) continue;

        MeshHandle handle{static_cast<uint32_t>(meshes.size())};
        meshes.push_back(std::make_shared<Mesh>(m.name, std::move(primitives)));
        meshNames[key] = handle;

        if (i == 0) firstHandle = handle;
    }

    return firstHandle;
}

MeshHandle ResourceManager::GetMesh(const std::string &name) const {
    if (auto it = meshNames.find(name); it != meshNames.end())
        return it->second;
    return {};
}

Mesh *ResourceManager::ResolveMesh(MeshHandle handle) const {
    if (handle.IsValid() && handle.id < meshes.size())
        return meshes[handle.id].get();
    return nullptr;
}

void ResourceManager::WatchPaths() {
    for (const auto &[path, shaderName]: shaderFilesToWatch) {
        if (!std::filesystem::exists(path)) continue;

        auto lastWriteTime = std::filesystem::last_write_time(path);
        if (lastWriteTime == shaderFileWriteTimes[path]) continue;

        shaderFileWriteTimes[path] = lastWriteTime;
        std::cout << "Shader file changed: " << path << "\n";

        Shader *shader = ResolveShader(GetShader(shaderName));
        if (!shader) continue;

        if (std::filesystem::path(path).extension() == ".vert")
            shader->Reload(path, shader->fragmentShaderPath);
        else
            shader->Reload(shader->vertexShaderPath, path);
    }
}

std::vector<GlPrimitive> ResourceManager::LoadPrimitivesFromGltf(
    const fastgltf::Mesh &mesh,
    const fastgltf::Asset &asset,
    const std::filesystem::path &gltfPath) {
    std::vector<GlPrimitive> primitives;
    primitives.reserve(mesh.primitives.size());

    for (const auto &prim: mesh.primitives) {
        auto vertexData = GetInterleavedPosTex(asset, prim);
        if (vertexData.empty()) continue;

        auto indices = GetIndices(asset, prim);

        GlPrimitive output{{MakeVertexStream(vertexData)}, indices, PosNormTexLayout};
        if (prim.materialIndex.has_value()) {
            const auto &material = asset.materials[prim.materialIndex.value()];
            const auto &pbr = material.pbrData;

            auto baseColorFactor = glm::vec4(pbr.baseColorFactor.x(),
                                             pbr.baseColorFactor.y(),
                                             pbr.baseColorFactor.z(),
                                             pbr.baseColorFactor.w());
            float metallicFactor = pbr.metallicFactor;
            float roughnessFactor = pbr.roughnessFactor;

            auto primMat = LoadMaterial(
                ShaderHandle{}, {}, {}, std::to_string(IdGenerator::GenerateId()));
            output.material = primMat;

            // set PBR uniforms
            ResolveMaterial(primMat)->uniforms["baseColorFactor"] = baseColorFactor;
            ResolveMaterial(primMat)->uniforms["metallicFactor"] = metallicFactor;
            ResolveMaterial(primMat)->uniforms["roughnessFactor"] = roughnessFactor;

            // Read wrap/filter from the GLTF sampler if present
            SamplerDescription sd{
                .minFilter = GL_LINEAR_MIPMAP_LINEAR,
                .magFilter = GL_LINEAR,
                .wrapS = GL_REPEAT,
                .wrapT = GL_REPEAT,
                .maxAnisotropy = std::nullopt,
            };

            if (pbr.baseColorTexture.has_value()) {
                const auto &tex = asset.textures[pbr.baseColorTexture->textureIndex];
                if (tex.samplerIndex.has_value()) {
                    const auto &gltfSampler = asset.samplers[tex.samplerIndex.value()];
                    if (gltfSampler.wrapS == fastgltf::Wrap::ClampToEdge)
                        sd.wrapS = GL_CLAMP_TO_EDGE;
                    if (gltfSampler.wrapT == fastgltf::Wrap::ClampToEdge)
                        sd.wrapT = GL_CLAMP_TO_EDGE;
                    if (gltfSampler.wrapS == fastgltf::Wrap::MirroredRepeat)
                        sd.wrapS = GL_MIRRORED_REPEAT;
                    if (gltfSampler.wrapT == fastgltf::Wrap::MirroredRepeat)
                        sd.wrapT = GL_MIRRORED_REPEAT;
                }
            }

            auto sampler = LoadSampler(sd, std::to_string(IdGenerator::GenerateId()));

            if (pbr.baseColorTexture.has_value()) {
                const auto &tex = asset.textures[pbr.baseColorTexture->textureIndex];
                const auto &image = asset.images[tex.imageIndex.value()];

                if (auto *uri = std::get_if<fastgltf::sources::URI>(&image.data)) {
                    auto texPath = gltfPath.parent_path() / uri->uri.fspath();
                    auto baseColorTex = LoadTexture(texPath.string(),
                                                    {.internalformat = GL_RGBA8,
                                                     .hasMipmap = true});
                    ResolveMaterial(primMat)->textures["baseColor"] =
                        std::make_pair(baseColorTex, sampler);
                }
            }
            if (pbr.metallicRoughnessTexture.has_value()) {
                const auto &tex = asset.textures[pbr.metallicRoughnessTexture->textureIndex];
                const auto &image = asset.images[tex.imageIndex.value()];

                if (auto *uri = std::get_if<fastgltf::sources::URI>(&image.data)) {
                    auto texPath = gltfPath.parent_path() / uri->uri.fspath();
                    auto metallicRoughnessTex = LoadTexture(
                        texPath.string(),
                        {.internalformat = GL_RGBA8, .hasMipmap = true});
                    ResolveMaterial(primMat)->textures["metallicRoughness"] =
                        std::make_pair(metallicRoughnessTex, sampler);
                }
            }

            primitives.push_back(std::move(output));
        }
    }
    return primitives;
}

