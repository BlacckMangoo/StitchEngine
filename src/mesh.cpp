#include "mesh.h"

#include <utility>

glm::mat4 Transform::GetModelMatrix(const Transform &t) {
    const glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);
    const glm::mat4 R = glm::toMat4(t.rotation);
    const glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
    return T * R * S;
}

VertexStream MakeVertexStream(const std::vector<float> &interleavedData) {
    VertexStream stream;
    stream.data.resize(interleavedData.size() * sizeof(float));
    std::memcpy(stream.data.data(), interleavedData.data(), stream.data.size());
    return stream;
}

const VertexLayout PosTexLayout{
    .attributes = {
        {0, 0, 3, GL_FLOAT, GL_FALSE, 0},
        {1, 0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3}
    },
    .bindings = {
        {0, 0, static_cast<GLsizei>(sizeof(float) * 5)}
    }
};

const VertexLayout PosNormTexLayout{
    .attributes = {
        {0, 0, 3, GL_FLOAT, GL_FALSE, 0},
        {1, 0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3},
        {2, 0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6}
    },
    .bindings = {
        {0, 0, static_cast<GLsizei>(sizeof(float) * 8)}
    }
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

const unsigned int indices[36] = {
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

const unsigned int quadIndices[6] = {0, 1, 2, 2, 3, 0};

GlPrimitive::GlPrimitive(std::vector<VertexStream> streams,
                         std::vector<unsigned int> indices,
                         VertexLayout layout)
    : vertexStreams(std::move(streams)),
      indices(std::move(indices)),
      layout(std::move(layout)) {
    InitializeBuffers();
}

GlPrimitive::GlPrimitive(PrimitiveData &&data) {
    vertexStreams = std::move(data.streams);
    indices = std::move(data.indices);
    layout = std::move(data.layout);
    InitializeBuffers();
}

GlPrimitive::GlPrimitive(const PrimitiveData &data) {
    vertexStreams = data.streams;
    indices = data.indices;
    layout = data.layout;
    InitializeBuffers();
}

GlPrimitive::GlPrimitive(GlPrimitive &&other) noexcept {
    MoveFrom(other);
}

GlPrimitive &GlPrimitive::operator=(GlPrimitive &&other) noexcept {
    if (this != &other) {
        Release();
        MoveFrom(other);
    }
    return *this;
}

GlPrimitive::~GlPrimitive() {
    Release();
}

void GlPrimitive::Draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    assert(glGetError() == GL_NO_ERROR && "Draw call failed!");
}

void GlPrimitive::InitializeBuffers() {
    glCreateBuffers(1, &EBO);
    glCreateVertexArrays(1, &VAO);
    VBOs.resize(vertexStreams.size());
    glCreateBuffers(static_cast<GLsizei>(VBOs.size()), VBOs.data());

    for (size_t i = 0; i < VBOs.size(); i++) {
        glNamedBufferData(VBOs[i],
                          static_cast<GLsizeiptr>(vertexStreams[i].data.size()),
                          vertexStreams[i].data.data(),
                          GL_STATIC_DRAW);
    }
    glNamedBufferData(
        EBO,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(VAO);
    for (const auto &[location, binding, size, type, normalized, offset]: layout.attributes) {
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

void GlPrimitive::Release() {
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

void GlPrimitive::MoveFrom(GlPrimitive &other) {
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

Mesh::Mesh(std::string_view name, std::vector<GlPrimitive> &&primitives)
    : primitives(std::move(primitives)), name(name) {
}

