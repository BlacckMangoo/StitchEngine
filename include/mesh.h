#pragma once
#include "glad/glad.h"
#include <memory>
#include <vector>
#include <cstring>


//##CASES FOR VERTEX DATA LAYOUT

//#STREAM LAYOUT
// ( Non interleaved)  : Position stream : [ px1,py1,pz1,px2,py2,pz3.....] 1)  pos attribute
//          Normal  stream : [ nx1,ny1,nz1 ..............  ] 2)  normal attribute
// ( Interleaved stream)
//           vertex stream  : [px1,py1,pz1,ny1,ny2,ny3....] multiple attributes interleaved
//                             [attribute 1 ][attribute 2]
//(Hybrid)
//  stream A : [ px1,py1,pz1,nx1,ny1,nx2 .........]
//  stream B : [ b1,b2,b3b,b4]

//#INDEXED VS NON INDEXED

//non indexed : vertices duplicated multiple times
// index      : reuse same vertex data and specify triangles using index buffer

// note to self :
//Old open gl combines binding and attribute into one thing , but they are fundamentally separate things
// separating them allows way more flexibility , enables stuff like instancing , interleaved and non interleaved data

// defines how GPU interprets a set of bytes
struct VertexAttribute
{
    GLuint location; // location inside shader : layout(location = 0)
    GLuint binding;  // binding point ( where does this attribute read bytes from)
    GLint  size;    // no of components like Vec3 has 3 x,y and z
    GLenum type;     //type of data ex float,unsigned int etc..
    GLboolean normalized; // whether to normalize fixed-point data
    size_t offset;       // byte offset from start of the vertex for the start of this particular attribute
};

//determines how a GPU walks through a stream of bytes
struct VertexBinding
{
    GLuint binding{};
    size_t offset{}; // helps us avoid separate VBO and make large arena allocations
    GLsizei stride{};
};

struct VertexLayout
{
    std::vector<VertexAttribute> attributes; // what does a bunch of 101010101 mean
    std::vector<VertexBinding> bindings; // how to fetch those 1010101010
};



struct VertexStream {
    std::vector<std::byte> data;
};

inline VertexStream MakeVertexStream(const std::vector<float>& interleavedData) {
    VertexStream stream;
    stream.data.resize(interleavedData.size() * sizeof(float));
    std::memcpy(stream.data.data(), interleavedData.data(), stream.data.size());
    return stream;
}




struct Mesh {
    explicit  Mesh(const std::vector<VertexStream> &vertexBufferData, const std::vector<unsigned int> &indices,const VertexLayout& layout) : vertexStreams(vertexBufferData) , indices(indices) , layout(layout){
        glCreateBuffers(1, &EBO);
        glCreateVertexArrays(1, &VAO);
        VBOs.resize(vertexStreams.size());
        glCreateBuffers(static_cast<GLsizei>(VBOs.size()), VBOs.data());

        for (size_t i = 0; i < VBOs.size(); i++) {
            glNamedBufferData(VBOs[i], static_cast<GLsizeiptr>(vertexBufferData[i].data.size()), vertexBufferData[i].data.data(), GL_STATIC_DRAW);
        }
        glNamedBufferData(EBO, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(VAO);
        for ( const auto&[location, binding, size, type, normalized, offset] : layout.attributes) {
            glEnableVertexArrayAttrib(VAO, location);
            glVertexArrayAttribBinding(VAO, location, binding);
            glVertexArrayAttribFormat(VAO, location, size, type, normalized, offset);

        }

        for ( const auto&[binding, offset, stride] : layout.bindings) {
            glVertexArrayVertexBuffer(VAO, binding, VBOs[binding], static_cast<GLintptr>(offset), stride);
        }

        glVertexArrayElementBuffer(VAO,EBO);

    };

    ~Mesh() {

        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(static_cast<GLsizei>(vertexStreams.size()), VBOs.data());
    }

    // VAO described to the GPU where does the Data come from , how to fetch data , and once fetched how to interpret it in a Shader Program
    unsigned int VAO {};
    std::vector<VertexStream> vertexStreams;
    unsigned int EBO {};
    std::vector<unsigned int> indices {};
    std::vector<unsigned int> VBOs {};
    VertexLayout layout {};

};
