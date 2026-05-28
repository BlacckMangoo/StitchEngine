#pragma once
#include "glad/glad.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// ##CASES FOR VERTEX DATA LAYOUT
// #STREAM LAYOUT
//  ( Non interleaved)  : Position stream : [ px1,py1,pz1,px2,py2,pz3.....] 1)
//  pos attribute
//           Normal  stream : [ nx1,ny1,nz1 ..............  ] 2)  normal
//           attribute
//  ( Interleaved stream)
//            vertex stream  : [px1,py1,pz1,ny1,ny2,ny3....] multiple attributes
//            interleaved
//                              [attribute 1 ][attribute 2]
//(Hybrid)
//   stream A : [ px1,py1,pz1,nx1,ny1,nx2 .........]
//   stream B : [ b1,b2,b3b,b4]
//
// #INDEXED VS NON INDEXED
//
// non indexed : vertices duplicated multiple times
//  index      : reuse same vertex data and specify triangles using index buffer
//
// note to self :
// Old open gl combines binding and attribute into one thing , but they are
// fundamentally separate things
// separating them allows way more flexibility , enables stuff like instancing ,
// interleaved and non interleaved data
//
// defines how GPU interprets a set of bytes

struct TextureHandle {
    uint32_t id = UINT32_MAX;
    bool IsValid() const { return id != UINT32_MAX; }
};
struct MeshHandle {
    uint32_t id = UINT32_MAX;
    bool IsValid() const { return id != UINT32_MAX; }
};
struct MaterialHandle {
    uint32_t id = UINT32_MAX;
    bool IsValid() const { return id != UINT32_MAX; }
};
struct ShaderHandle {
    uint32_t id = UINT32_MAX;
    bool IsValid() const { return id != UINT32_MAX; }
};
struct SamplerHandle {
    uint32_t id = UINT32_MAX;
    bool IsValid() const { return id != UINT32_MAX; }
};

struct Transform {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};

    static glm::mat4 GetModelMatrix(const Transform &t);
};

struct VertexAttribute {
    GLuint location; // location inside shader : layout(location = 0)
    GLuint binding; // binding point ( where does this attribute read bytes from)
    GLint size; // no of components like Vec3 has 3 x,y and z
    GLenum type; // type of data ex float,unsigned int etc..
    GLboolean normalized; // whether to normalize fixed-point data
    size_t offset; // byte offset from start of the vertex for the start of this
    // particular attribute
};

// determines how a GPU walks through a stream of bytes
struct VertexBinding {
    GLuint binding{};
    size_t offset{}; // helps us avoid separate VBO and make large arena allocations
    GLsizei stride{};
};

struct VertexLayout {
    std::vector<VertexAttribute> attributes; // what does a bunch of 101010101 mean
    std::vector<VertexBinding> bindings; // how to fetch those 1010101010
};

struct VertexStream {
    std::vector<std::byte> data;
};

VertexStream MakeVertexStream(const std::vector<float> &interleavedData);

extern const VertexLayout PosTexLayout;
extern const VertexLayout PosNormTexLayout;

struct PrimitiveData {
    std::vector<VertexStream> streams;
    std::vector<unsigned int> indices;
    VertexLayout layout;
};

using UniformValue =
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

struct Material {
    ShaderHandle shader;
    std::unordered_map<std::string, UniformValue> uniforms;
    std::unordered_map<std::string, std::pair<TextureHandle, SamplerHandle> >
        textures;
};

extern const std::vector<float> cubeInterleavedDataPosNormTex;
extern const unsigned int indices[36];
extern const std::vector<float> QuadInterleavedData;
extern const std::vector<float> QuadInterleavedDataPosNormTex;
extern const unsigned int quadIndices[6];

struct GlPrimitive {
    GlPrimitive(std::vector<VertexStream> streams,
                std::vector<unsigned int> indices,
                VertexLayout layout);

    explicit GlPrimitive(PrimitiveData &&data);

    explicit GlPrimitive(const PrimitiveData &data);

    GlPrimitive(const GlPrimitive &) = delete;

    GlPrimitive &operator=(const GlPrimitive &) = delete;

    GlPrimitive(GlPrimitive &&other) noexcept;

    GlPrimitive &operator=(GlPrimitive &&other) noexcept;

    ~GlPrimitive();

    void Draw() const;

    unsigned int VAO{};
    std::vector<VertexStream> vertexStreams;
    unsigned int EBO{};
    std::vector<unsigned int> indices{};
    std::vector<unsigned int> VBOs{};
    VertexLayout layout{};
    mutable MaterialHandle material{};

private:
    void InitializeBuffers();

    void Release();

    void MoveFrom(GlPrimitive &other);
};

struct Mesh {
    Mesh(std::string_view name, std::vector<GlPrimitive> &&primitives);
    std::vector<GlPrimitive> primitives;
    std::string name{};
};
