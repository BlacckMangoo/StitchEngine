#pragma once
#include "glad/glad.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <string>
#include <utility>
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

// #INDEXED VS NON INDEXED

// non indexed : vertices duplicated multiple times
//  index      : reuse same vertex data and specify triangles using index buffer

// note to self :
// Old open gl combines binding and attribute into one thing , but they are
// fundamentally separate things
// separating them allows way more flexibility , enables stuff like instancing ,
// interleaved and non interleaved data

// defines how GPU interprets a set of bytes


struct TextureHandle  { uint32_t id = UINT32_MAX; bool IsValid() const { return id != UINT32_MAX; } };
struct MeshHandle     { uint32_t id = UINT32_MAX; bool IsValid() const { return id != UINT32_MAX; } };
struct MaterialHandle { uint32_t id = UINT32_MAX; bool IsValid() const { return id != UINT32_MAX; } };
struct ShaderHandle   { uint32_t id = UINT32_MAX; bool IsValid() const { return id != UINT32_MAX; } };
struct SamplerHandle  { uint32_t id = UINT32_MAX; bool IsValid() const { return id != UINT32_MAX; } };



struct Transform {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    glm::quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};

    static glm::mat4 GetModelMatrix(const Transform &t) {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);
        const glm::mat4 R = glm::toMat4(t.rotation);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
        return T * R * S;
    }
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
    size_t
    offset{}; // helps us avoid separate VBO and make large arena allocations
    GLsizei stride{};
};

struct VertexLayout {
    std::vector<VertexAttribute>
    attributes; // what does a bunch of 101010101 mean
    std::vector<VertexBinding> bindings; // how to fetch those 1010101010
};

struct VertexStream {
    std::vector<std::byte> data;
};

inline VertexStream
MakeVertexStream(const std::vector<float> &interleavedData) {
    VertexStream stream;
    stream.data.resize(interleavedData.size() * sizeof(float));
    std::memcpy(stream.data.data(), interleavedData.data(), stream.data.size());
    return stream;
}

inline VertexLayout PosTexLayout{
    .attributes =
    {
        {0, 0, 3, GL_FLOAT, GL_FALSE, 0}, // position attribute
        {1, 0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3}
        // texcoord attribute
    },
    .bindings = {
        {
            0, 0,
            static_cast<GLsizei>(
                sizeof(float) *
                5)
        } // single interleaved stream with position and texcoord
    }
};

inline VertexLayout PosNormTexLayout{
    .attributes =
    {
        {0, 0, 3, GL_FLOAT, GL_FALSE, 0}, // position attribute
        {
            1, 0, 3, GL_FLOAT, GL_FALSE,
            sizeof(float) * 3
        }, // normal attribute
        {2, 0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6}
        // texcoord attribute
    },
    .bindings = {{0, 0, static_cast<GLsizei>(sizeof(float) * 8)}}
};

struct PrimitiveData {
    std::vector<VertexStream> streams;
    std::vector<unsigned int> indices;
    VertexLayout layout;
};


using UniformValue =
std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;


struct Material {
    ShaderHandle shader ;
    std::unordered_map<std::string, UniformValue>
    uniforms; // "nameInShaderCode", value
    std::unordered_map<std::string, std::pair<TextureHandle, SamplerHandle> >
    textures; //  "nameInShaderCode",Texture
};


const std::vector<float> cubeInterleavedDataPosNormTex = {
    // position              // normal               // uv

    // Front face (+Z)
    -0.5f,
    -0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    0.5f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,

    // Back face (-Z)
    0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    -0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    1.0f,
    0.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,

    // Left face (-X)
    -0.5f,
    -0.5f,
    -0.5f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -0.5f,
    -0.5f,
    0.5f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    -0.5f,
    0.5f,
    0.5f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,

    // Right face (+X)
    0.5f,
    -0.5f,
    0.5f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    -0.5f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    -0.5f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.5f,
    0.5f,
    0.5f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    1.0f,

    // Top face (+Y)
    -0.5f,
    0.5f,
    0.5f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.5f,
    0.5f,
    0.5f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    0.0f,
    0.5f,
    0.5f,
    -0.5f,
    0.0f,
    1.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    0.5f,
    -0.5f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,

    // Bottom face (-Y)
    -0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.5f,
    -0.5f,
    -0.5f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,
    0.0f,
    0.5f,
    -0.5f,
    0.5f,
    0.0f,
    -1.0f,
    0.0f,
    1.0f,
    1.0f,
    -0.5f,
    -0.5f,
    0.5f,
    0.0f,
    -1.0f,
    0.0f,
    0.0f,
    1.0f,
};

constexpr unsigned int indices[36] = {
    0, 1, 2, 2, 3, 0, // front
    4, 5, 6, 6, 7, 4, // back
    8, 9, 10, 10, 11, 8, // left
    12, 13, 14, 14, 15, 12, // right
    16, 17, 18, 18, 19, 16, // top
    20, 21, 22, 22, 23, 20, // bottom
};

const std::vector<float> QuadInterleavedData = {
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};

const std::vector<float> QuadInterleavedDataPosNormTex = {
    // position              // normal              // uv

    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
};

constexpr unsigned int quadIndices[6] = {0, 1, 2, 2, 3, 0};

struct GlPrimitive {
    GlPrimitive(std::vector<VertexStream> streams,
                std::vector<unsigned int> indices, VertexLayout layout)
        : vertexStreams(std::move(streams)), indices(std::move(indices)),
          layout(std::move(layout)) {
        InitializeBuffers();
    }

    explicit GlPrimitive(PrimitiveData &&data) {
        vertexStreams = std::move(data.streams);
        indices = std::move(data.indices);
        layout = std::move(data.layout);
        InitializeBuffers();
    }

    explicit GlPrimitive(const PrimitiveData &data) {
        vertexStreams = data.streams;
        indices = data.indices;
        layout = data.layout;
        InitializeBuffers();
    }

    GlPrimitive(const GlPrimitive &) = delete;

    GlPrimitive &operator=(const GlPrimitive &) = delete;

    GlPrimitive(GlPrimitive &&other) noexcept { MoveFrom(other); }

    GlPrimitive &operator=(GlPrimitive &&other) noexcept {
        if (this != &other) {
            Release();
            MoveFrom(other);
        }
        return *this;
    }

    ~GlPrimitive() { Release(); }

    void Draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                       GL_UNSIGNED_INT, nullptr);
        assert(glGetError() == GL_NO_ERROR && "Draw call failed!");
    }

    // VAO described to the GPU where does the Data come from , how to fetch data
    // , and once fetched how to interpret it in a Shader Program
    unsigned int VAO{};
    std::vector<VertexStream> vertexStreams;
    unsigned int EBO{};
    std::vector<unsigned int> indices{};
    std::vector<unsigned int> VBOs{};
    VertexLayout layout{};
    mutable MaterialHandle material{};

private:
    void InitializeBuffers() {
        glCreateBuffers(1, &EBO);
        glCreateVertexArrays(1, &VAO);
        VBOs.resize(vertexStreams.size());
        glCreateBuffers(static_cast<GLsizei>(VBOs.size()), VBOs.data());

        for (size_t i = 0; i < VBOs.size(); i++) {
            glNamedBufferData(VBOs[i],
                              static_cast<GLsizeiptr>(vertexStreams[i].data.size()),
                              vertexStreams[i].data.data(), GL_STATIC_DRAW);
        }
        glNamedBufferData(
            EBO, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(VAO);
        for (const auto &[location, binding, size, type, normalized, offset]:
             layout.attributes) {
            glEnableVertexArrayAttrib(VAO, location);
            glVertexArrayAttribBinding(VAO, location, binding);
            glVertexArrayAttribFormat(VAO, location, size, type, normalized, offset);
            assert(glGetError() == GL_NO_ERROR && "Vertex attribute setup failed!");
        }

        for (const auto &[binding, offset, stride]: layout.bindings) {
            glVertexArrayVertexBuffer(VAO, binding, VBOs[binding],
                                      static_cast<GLintptr>(offset), stride);
            assert(glGetError() == GL_NO_ERROR && "Vertex buffer setup failed!");
        }

        glVertexArrayElementBuffer(VAO, EBO);
        assert(glGetError() == GL_NO_ERROR && "Element buffer setup failed!");
    }

    void Release() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
        }
        if (!VBOs.empty()) {
            glDeleteBuffers(static_cast<GLsizei>(VBOs.size()), VBOs.data());
        }
        VAO = 0;
        EBO = 0;
        VBOs.clear();
    }

    void MoveFrom(GlPrimitive &other) {
        VAO = other.VAO;
        EBO = other.EBO;
        VBOs = std::move(other.VBOs);
        vertexStreams = std::move(other.vertexStreams);
        indices = std::move(other.indices);
        layout = std::move(other.layout);
        other.VAO = 0;
        other.EBO = 0;
        other.VBOs.clear();
        material = std::exchange(other.material, MaterialHandle{});
    }
};

struct Mesh {
    Mesh(const std::string_view name, std::vector<GlPrimitive> &&primitives)
        : primitives(std::move(primitives)), name(name){
    }
    std::vector<GlPrimitive> primitives;
    std::string name{};
};
