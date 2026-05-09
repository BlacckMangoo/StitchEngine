#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <filesystem>
#include "shader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyObjLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stbImage.h>

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

// note to self :
//Old open gl combines binding and attribute into one thing , but they are fundamentally separate things
// separating them allows way more flexibility , enables stuff like instancing , interleaved and non interleaved data


struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
};

struct VertexStream {
    std::vector<std::byte> data;
    GLuint VBO {};
    uint32_t stride {};
};


struct Mesh {
    explicit  Mesh(const std::vector<VertexStream> &vertexBufferData, const std::vector<unsigned int> &indices) : vertexStreams(vertexBufferData) , indices(indices) {
        glCreateBuffers(1, &EBO);
        glCreateVertexArrays(1, &VAO);
    };

    ~Mesh() {
        for ( const auto& v : vertexStreams) {
            glDeleteBuffers(1, &v.VBO);
        }
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }

    // VAO described to the GPU where does the Data come from , how to fetch data , and once fetched how to interpret it in a Shader Program
    unsigned int VAO {};

    // could be just one Interleaved Buffer [ px,py,pz, u, v, nx, ny, nz ] or could be multiple buffers [px,py,pz][u,v][nx,ny,nz]
    //or could also be mixed [px,py,pz,u,v],[nx,ny,nz]
    std::vector<VertexStream> vertexStreams;

    unsigned int EBO {};
    std::vector<unsigned int> indices ;
};


const std::vector<Vertex> quadData = {
    // Front face
    { {-0.5f, -0.5f,  0.5f}, {0.0f,  0.0f} },
    { { 0.5f, -0.5f,  0.5f}, {1.0f,  0.0f} },
    { { 0.5f,  0.5f,  0.5f}, {1.0f,  1.0f} },
    { {-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f} },

    // Back face
    { { 0.5f, -0.5f, -0.5f}, {0.0f,  0.0f} },
    { {-0.5f, -0.5f, -0.5f}, {1.0f,  0.0f} },
    { {-0.5f,  0.5f, -0.5f}, {1.0f,  1.0f} },
    { { 0.5f,  0.5f, -0.5f}, {0.0f,  1.0f} },

    // Left face
    { {-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f} },
    { {-0.5f, -0.5f,  0.5f}, {1.0f,  0.0f} },
    { {-0.5f,  0.5f,  0.5f}, {1.0f,  1.0f} },
    { {-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f} },

    // Right face
    { { 0.5f, -0.5f,  0.5f}, {0.0f,  0.0f} },
    { { 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f} },
    { { 0.5f,  0.5f, -0.5f}, {1.0f,  1.0f} },
    { { 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f} },

    // Top face
    { {-0.5f,  0.5f,  0.5f}, {0.0f,  0.0f} },
    { { 0.5f,  0.5f,  0.5f}, {1.0f,  0.0f} },
    { { 0.5f,  0.5f, -0.5f}, {1.0f,  1.0f} },
    { {-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f} },

    // Bottom face
    { {-0.5f, -0.5f, -0.5f}, {0.0f,  0.0f} },
    { { 0.5f, -0.5f, -0.5f}, {1.0f,  0.0f} },
    { { 0.5f, -0.5f,  0.5f}, {1.0f,  1.0f} },
    { {-0.5f, -0.5f,  0.5f}, {0.0f,  1.0f} },
};

constexpr unsigned int indices[36] = {
    0,  1,  2,  2,  3,  0, // front
    4,  5,  6,  6,  7,  4, // back
    8,  9, 10, 10, 11,  8, // left
   12, 13, 14, 14, 15, 12, // right
   16, 17, 18, 18, 19, 16, // top
   20, 21, 22, 22, 23, 20, // bottom
};

const std::vector<Vertex> fbQuadData = {
    { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
    { { 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f} },
    { { 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f} },
    { {-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f} },
};

constexpr unsigned int fbIndices[6] = {
    0, 1, 2,
    2, 3, 0
};


constexpr unsigned short OPENGL_MAJOR_VERSION = 4;
constexpr unsigned short OPENGL_MINOR_VERSION = 6;

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 1280;

constexpr float ASPECT_RATIO = SCREEN_WIDTH / static_cast<float>(SCREEN_HEIGHT);

auto camPos   = glm::vec3(0.0f, 0.0f, 10.0f);
auto camFront = glm::vec3(0.0f, 0.0f, -1.0f);
auto camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;
float pitch = 0.0f;

float lastX = 320.0f;
float lastY = 240.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void MouseCallback(GLFWwindow* window, const double xPos, const double yPos) {
    if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) {
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = static_cast<float>(xPos);
        lastY = static_cast<float>(yPos);
        firstMouse = false;
        return;
    }

    const float xOffset = (static_cast<float>(xPos) - lastX) * 0.1f;
    const float yOffset = (static_cast<float>(yPos) - lastY) * 0.1f; // NOT inverted
    lastX = static_cast<float>(xPos);
    lastY = static_cast<float>(yPos);

    yaw   += xOffset;
    pitch -= yOffset;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    camFront = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    ));

    constexpr auto worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right   = glm::normalize(glm::cross(camFront, worldUp));
    camUp                   = glm::normalize(glm::cross(right, camFront));
}
void UpdateCamera(GLFWwindow* window) {
    float speed = 2.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += speed * camUp;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camPos -= speed * camUp;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

}




int main() {


    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR_VERSION);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR_VERSION);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);


    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (const int version = gladLoadGL(); version == 0) {
        std::cout << "Failed to initialize OpenGL context!" << std::endl;
        glfwTerminate();
        return -1;
    }

    int width, height, n;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load("assets/img.png", &width, &height, &n, 4);
    if (!data) {
        std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
        glfwTerminate();
        return -1;
    }


    //load models
    const std::string file = "assets/models/Sponza/sponza.obj";
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "assets/models/Sponza/"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(file, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    GLuint imageTex;
    const int mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &imageTex);
    glTextureParameteri(imageTex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(imageTex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(imageTex, GL_TEXTURE_MAX_ANISOTROPY, GL_MAX_TEXTURE_MAX_ANISOTROPY);
    glTextureParameteri(imageTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(imageTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureStorage2D(imageTex, mipLevels, GL_RGBA8, width, height);
    glTextureSubImage2D(imageTex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateTextureMipmap(imageTex);

    stbi_image_free(data);

    unsigned int  FBO;
    glCreateFramebuffers(1, &FBO);

    unsigned int fbVAO = 0, fbVBO = 0, fbEBO = 0;
    glCreateBuffers(1, &fbVBO);
    glCreateBuffers(1, &fbEBO);
    glCreateVertexArrays(1, &fbVAO);

    glNamedBufferData(fbVBO, sizeof(Vertex) * fbQuadData.size(), fbQuadData.data(), GL_STATIC_DRAW);
    glNamedBufferData(fbEBO, sizeof(fbIndices), fbIndices, GL_STATIC_DRAW);

    glEnableVertexArrayAttrib(fbVAO, 0);
    glVertexArrayAttribBinding(fbVAO, 0, 0);
    glVertexArrayAttribFormat(fbVAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
    glEnableVertexArrayAttrib(fbVAO, 1);
    glVertexArrayAttribBinding(fbVAO, 1, 0);
    glVertexArrayAttribFormat(fbVAO, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
    glVertexArrayVertexBuffer(fbVAO, 0, fbVBO, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(fbVAO, fbEBO);



    unsigned int  framebufferTex;
    glCreateTextures(GL_TEXTURE_2D, 1, &framebufferTex);
    glTextureParameteri(framebufferTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(framebufferTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(framebufferTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(framebufferTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureStorage2D(framebufferTex, 1, GL_RGB8, SCREEN_WIDTH, SCREEN_HEIGHT);
    glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0, framebufferTex, 0);

    unsigned int RBO;
    glCreateRenderbuffers(1, &RBO);
    glNamedRenderbufferStorage(RBO, GL_DEPTH_COMPONENT24, SCREEN_WIDTH, SCREEN_HEIGHT);
    glNamedFramebufferRenderbuffer(FBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (const auto fboStatus = glCheckNamedFramebufferStatus(FBO, GL_FRAMEBUFFER); fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer error: " << fboStatus << "\n";



    const auto triangleShaderProgram = CreateProgram(GetFileText("shaders/vertexShader.vert"), GetFileText("shaders/fragShader.frag"));
    const auto frameBufferProgram = CreateProgram(GetFileText("shaders/frameBufferVert.vert"), GetFileText("shaders/frameBufferFrag.frag"));

    unsigned int VAO = 0, VBO = 0, EBO = 0;


    glCreateBuffers(1, &VBO);
    glCreateBuffers(1, &EBO);
    glCreateVertexArrays(1, &VAO);

    glNamedBufferData(VBO, sizeof(Vertex) * quadData.size(), quadData.data(), GL_STATIC_DRAW);
    glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);


    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribBinding(VAO, 0, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));

    glEnableVertexArrayAttrib(VAO, 1);
    glVertexArrayAttribBinding(VAO, 1, 0);
    glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));

    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(VAO, EBO);



    std::cout << glGetString(GL_RENDERER) << std::endl;

    while (!glfwWindowShouldClose(window)) {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        UpdateCamera(window);

        auto transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(0.5f, 0.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, glm::vec3(1.0f, 1.0f, 1.0f));

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

        glm::mat4 proj = glm::perspective(
            glm::radians(45.0f),
            ASPECT_RATIO,
            0.1f,
            1000.0f
        );

        glm::mat4 finalMVP = proj * view * transform;
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glUseProgram(triangleShaderProgram);
        glBindVertexArray(VAO);
        glBindTextureUnit(0, imageTex);
        glUniform1i(glGetUniformLocation(triangleShaderProgram, "tex"), 0);
        glUniformMatrix4fv(glGetUniformLocation(triangleShaderProgram, "MVP"), 1, GL_FALSE, glm::value_ptr(finalMVP));
        glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(frameBufferProgram);
        glBindVertexArray(fbVAO);
        glBindTextureUnit(0, framebufferTex);
        glUniform1i(glGetUniformLocation(frameBufferProgram, "screen"), 0);
        glDrawElements(GL_TRIANGLES, std::size(fbIndices), GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteFramebuffers(1, &FBO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &imageTex);
    glDeleteTextures(1,&framebufferTex);
    glDeleteProgram(triangleShaderProgram);
    glDeleteProgram(frameBufferProgram);
    glDeleteRenderbuffers(1, &RBO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}