#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <filesystem>
#include <cstring>
#include "shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "window.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stbImage.h>

#include  "fastgltf/core.hpp"
#include "Texture.h"
#include "mesh.h"
#include "camera.h"

const std::vector<float> cubeInterleavedData = {
    // Front face
    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 0.0f, 1.0f,

    // Back face
     0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f, 0.0f, 1.0f,

    // Left face
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,

    // Right face
     0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 0.0f, 1.0f,

    // Top face
    -0.5f,  0.5f,  0.5f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,

    // Bottom face
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0.0f, 1.0f,
};

constexpr unsigned int indices[36] = {
    0,  1,  2,  2,  3,  0, // front
    4,  5,  6,  6,  7,  4, // back
    8,  9, 10, 10, 11,  8, // left
   12, 13, 14, 14, 15, 12, // right
   16, 17, 18, 18, 19, 16, // top
   20, 21, 22, 22, 23, 20, // bottom
};

const std::vector<float> QuadInterleavedData = {
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
};

constexpr unsigned int quadIndices[6] = {
    0, 1, 2,
    2, 3, 0
};

const VertexStream cubeVertexStream = MakeVertexStream(cubeInterleavedData);
const VertexStream quadVertexStream = MakeVertexStream(QuadInterleavedData);

constexpr GLsizei kPosUvStride = static_cast<GLsizei>(sizeof(float) * 5);
constexpr size_t kPosOffset = 0;
constexpr size_t kUvOffset = sizeof(float) * 3;

constexpr unsigned short OPENGL_MAJOR_VERSION = 4;
constexpr unsigned short OPENGL_MINOR_VERSION = 6;

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 1280;

constexpr float ASPECT_RATIO = SCREEN_WIDTH / static_cast<float>(SCREEN_HEIGHT);


float deltaTime = 0.0f;
float lastFrame = 0.0f;




int main() {
    Window window(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World");
    if (!window.isValid()) {
        return -1;
    }

    if (const int version = gladLoadGL(); version == 0) {
        std::cout << "Failed to initialize OpenGL context!" << std::endl;
        return -1;
    }

    int width, height, n;
    stbi_set_flip_vertically_on_load(true);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    unsigned char* data = stbi_load("assets/img.png", &width, &height, &n, 4);
    if (!data) {
        std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
        return -1;
    }


    const std::filesystem::path path = "assets/glTF-Sample-Assets/Models/CesiumMan/glTF/CesiumMan.gltf";
    fastgltf::Parser parser;
    auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);
    if (gltfData.error() != fastgltf::Error::None) {
        std::cerr << "Failed to load GLTF file: " ;
        return -1;
    }

    auto asset = parser.loadGltf(gltfData.get(), path.parent_path(), fastgltf::Options::None);
    if (auto error = asset.error(); error != fastgltf::Error::None) {
        // Some error occurred while reading the buffer, parsing the JSON, or validating the data.
        return -1;
    }

    TextureDescription catTextureDesc {
        .internalformat = GL_RGBA8,
        .hasMipmap = true,
    };
    Texture catImageTex(width, height, catTextureDesc);
    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    SamplerDescription catSamplerDesc {
        .minFilter = GL_LINEAR_MIPMAP_LINEAR,
        .magFilter = GL_LINEAR,
        .wrapS = GL_REPEAT,
        .wrapT = GL_REPEAT,
        .maxAnisotropy = maxAniso,
    };
    Sampler catSampler(catSamplerDesc);

    catImageTex.SetData(data, GL_RGBA, GL_UNSIGNED_BYTE);
    catImageTex.GenerateMipmaps();

    stbi_image_free(data);

    unsigned int  FBO;
    glCreateFramebuffers(1, &FBO);

    TextureDescription renderTargetTextureDesc {
        .internalformat = GL_RGB8,
        .hasMipmap = false,
    };
    Texture renderTargetTexture(SCREEN_WIDTH, SCREEN_HEIGHT, renderTargetTextureDesc);
    SamplerDescription renderTargetTextureSamplerDesc {
        .minFilter = GL_NEAREST,
        .magFilter = GL_NEAREST,
        .wrapS = GL_CLAMP_TO_EDGE,
        .wrapT = GL_CLAMP_TO_EDGE,
        .maxAnisotropy = std::nullopt,
    };
    Sampler frameBufferSampler(renderTargetTextureSamplerDesc);

    glNamedFramebufferTexture(FBO, GL_COLOR_ATTACHMENT0, renderTargetTexture.id, 0);

    unsigned int RBO;
    glCreateRenderbuffers(1, &RBO);
    glNamedRenderbufferStorage(RBO, GL_DEPTH_COMPONENT24, SCREEN_WIDTH, SCREEN_HEIGHT);
    glNamedFramebufferRenderbuffer(FBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

    if (const auto fboStatus = glCheckNamedFramebufferStatus(FBO, GL_FRAMEBUFFER); fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer error: " << fboStatus << "\n";


    Shader DefaultShader(GetFileText("shaders/vertexShader.vert"), GetFileText("shaders/fragShader.frag"));
    Shader fullScreenShader(GetFileText("shaders/frameBufferVert.vert"), GetFileText("shaders/frameBufferFrag.frag"));

    VertexAttribute posAttribute = {
        .location = 0 ,
        .binding = 0 ,
        .size = 3 ,
        .type = GL_FLOAT,
        .normalized = GL_FALSE,
        .offset = kPosOffset,
    };

    VertexAttribute texCoordAttribute = {
        .location = 1 ,
        .binding = 0 ,
        .size = 2 ,
        .type = GL_FLOAT,
        .normalized = GL_FALSE,
        .offset = kUvOffset,
    };

    VertexLayout PosTexLayout = {
        .attributes = {posAttribute, texCoordAttribute},
        .bindings = {
            {
                .binding = 0 ,
                .offset = 0 ,
                .stride = kPosUvStride ,
            }
        }
    };

    const Mesh cubeMesh = Mesh({cubeVertexStream}, {indices, indices + 36}, PosTexLayout);
    const Mesh quadMesh = Mesh({quadVertexStream}, {quadIndices, quadIndices + 6}, PosTexLayout);

    std::cout << glGetString(GL_RENDERER) << std::endl;

    Camera cam ;

    while (!window.shouldClose()) {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        Window::pollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

       cam.UpdateCamera(window,deltaTime);

        auto model = glm::mat4(1.0f);


        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        DefaultShader.Bind();
        catImageTex.BindTo(0);
        catSampler.BindTo(0);
        DefaultShader.SetInt("tex", 0);
        DefaultShader.SetMat4("model", model);



        glBindVertexArray(cubeMesh.VAO);
        glDrawElements(GL_TRIANGLES, std::size(indices), GL_UNSIGNED_INT, nullptr);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        fullScreenShader.Bind();
        renderTargetTexture.BindTo(0);
        frameBufferSampler.BindTo(0);
        fullScreenShader.SetInt("screen", 0);
        glBindVertexArray(quadMesh.VAO);
        glDrawElements(GL_TRIANGLES, std::size(quadIndices), GL_UNSIGNED_INT, nullptr);


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteFramebuffers(1, &FBO);
    glDeleteRenderbuffers(1, &RBO);

    return 0;
}