#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <vector>
#include <filesystem>
#include <cstring>
#include <memory>
#include <string>
#include "shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "window.h"
#include <numeric>
#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "Texture.h"
#include "mesh.h"
#include "camera.h"
#include "frameBuffer.h"
#include "resourceManager.h"
#include "renderPass.h"
#include "sceneGraph.h"
#include  "timeManager.h"


struct SceneNode;

const std::filesystem::path GLTF_MODEL_PATH = "assets/glTF-Sample-Assets/Models";

const VertexStream cubeVertexStream = MakeVertexStream(cubeInterleavedDataPosNormTex);
const VertexStream quadVertexStream = MakeVertexStream(QuadInterleavedDataPosNormTex);
const VertexStream quadForFullScreenVertexStream = MakeVertexStream(QuadInterleavedData);


void ImguiInit(const Window &window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init();
}




int main() {
    Window window(1920, 1080, "GameEngine");


    if (gladLoadGL() == 0) {
        std::cout << "Failed to initialize OpenGL context!\n";
        return -1;
    }

    auto timeManager = TimeManager();

    // Meshes
    std::vector<GlPrimitive> quadPrimitives;
    quadPrimitives.emplace_back(
        std::vector<VertexStream>{quadVertexStream},
        std::vector<unsigned int>{quadIndices, quadIndices + 6},
        PosNormTexLayout);

    std::vector<GlPrimitive> quadForFullScreenPrimitives;
    quadForFullScreenPrimitives.emplace_back(
        std::vector<VertexStream>{quadForFullScreenVertexStream},
        std::vector<unsigned int>{quadIndices, quadIndices + 6}, PosTexLayout);


    std::vector<GlPrimitive> cubePrimitives;
    cubePrimitives.emplace_back(std::vector<VertexStream>{cubeVertexStream},
                                std::vector<unsigned int>{indices, indices + 36}, PosNormTexLayout);


    ImguiInit(window);

    ResourceManager resourceManager;

    resourceManager.LoadMesh(std::move(quadPrimitives), "Quad");
    resourceManager.LoadMesh(std::move(quadForFullScreenPrimitives),
                             "QuadForFullScreen");
    resourceManager.LoadMesh(std::move(cubePrimitives), "Cube");
    resourceManager.LoadGLTFMesh(
        GLTF_MODEL_PATH / "CesiumMan/glTF/CesiumMan.gltf", "CesiumMan");

    resourceManager.LoadGLTFMesh(
        GLTF_MODEL_PATH / "DamagedHelmet/glTF/DamagedHelmet.gltf",
        "DamagedHelmet");
    // lantern
    resourceManager.LoadGLTFMesh(
        GLTF_MODEL_PATH / "Lantern/glTF/Lantern.gltf",
        "Lantern");

    //sponza
    resourceManager.LoadGLTFMesh(
        GLTF_MODEL_PATH / "Sponza/glTF/Sponza.gltf",
        "Sponza");


    // Texture

    TextureDescription TexDesc{.internalformat = GL_RGBA8, .hasMipmap = true};
    TextureDescription posDesc{.internalformat = GL_RGB32F, .hasMipmap = false};
    TextureDescription depthDesc{.internalformat = GL_DEPTH_COMPONENT24, .hasMipmap = false};
    resourceManager.LoadTexture("assets/img.png", TexDesc, "catTexture");
    resourceManager.LoadTexture("assets/metal.jpg", TexDesc, "metalTexture");

    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    SamplerDescription samplerDesc{
        .minFilter = GL_LINEAR_MIPMAP_LINEAR,
        .magFilter = GL_LINEAR,
        .wrapS = GL_REPEAT,
        .wrapT = GL_REPEAT,
        .maxAnisotropy = maxAniso,
    };

    resourceManager.LoadSampler(samplerDesc, "sampler");

    TextureDescription rtDesc{.internalformat = GL_RGBA8, .hasMipmap = false};
    SamplerDescription rtSamplerDesc{
        .minFilter = GL_NEAREST,
        .magFilter = GL_NEAREST,
        .wrapS = GL_CLAMP_TO_EDGE,
        .wrapT = GL_CLAMP_TO_EDGE,
        .maxAnisotropy = std::nullopt,
    };

    resourceManager.CreateRenderTarget(window.width(), window.height(), rtDesc,
                                       "colorTargetTexture");
    resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc,
                                       "PositionTexture");
    resourceManager.CreateRenderTarget(window.width(), window.height(), rtDesc,
                                       "UvTexture");
    resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc,
                                       "NormalTexture");
    resourceManager.CreateRenderTarget(window.width(), window.height(), depthDesc,
                                       "DepthTexture");

    // Sampler frameBufferSampler(rtSamplerDesc);
    resourceManager.LoadSampler(rtSamplerDesc, "frameBufferSampler");

    FrameBuffer gBuffer;

    gBuffer.AddColorAttachment(
        resourceManager.GetTexture("colorTargetTexture")->id);
    gBuffer.AddColorAttachment(resourceManager.GetTexture("PositionTexture")->id);
    gBuffer.AddColorAttachment(resourceManager.GetTexture("UvTexture")->id);
    gBuffer.AddColorAttachment(resourceManager.GetTexture("NormalTexture")->id);
    gBuffer.AddDepthAttachmentTexture(resourceManager.GetTexture("DepthTexture")->id);

    gBuffer.FinalizeColorAttachments();
    gBuffer.Validate();

    // Lighting Pass Frame Buffer setup
    unsigned int lightingPassRBO;
    glCreateRenderbuffers(1, &lightingPassRBO);
    glNamedRenderbufferStorage(lightingPassRBO, GL_DEPTH_COMPONENT24, window.width(), window.height());


    resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc, "litSceneTexture");

    FrameBuffer lightingPassBuffer;

    lightingPassBuffer.AddColorAttachment(resourceManager.GetTexture("litSceneTexture")->id);
    lightingPassBuffer.AddDepthAttachmentRenderBuffer(lightingPassRBO);
    lightingPassBuffer.FinalizeColorAttachments();
    lightingPassBuffer.Validate();

    resourceManager.LoadShader("shaders/vertexShader.vert",
                               "shaders/fragShader.frag", "defaultShader");
    resourceManager.LoadShader("shaders/frameBufferVert.vert",
                               "shaders/frameBufferFrag.frag",
                               "fullScreenShader");
    resourceManager.LoadShader("shaders/frameBufferVert.vert",
                               "shaders/defferedLighting.frag",
                               "deferredLightingShader");


    DirectionalLight dirLight{
        .direction = {0.5f, 1.0f, 0.5f},
        .color = {1.0f, 1.0f, 1.0f}
    };


    std::cout << glGetString(GL_RENDERER) << '\n';

    resourceManager.LoadMaterial(
        "defaultShader", {}, {{"tex", std::make_pair("catTexture", "sampler")}},
        "catMat");

    resourceManager.LoadMaterial(
        "defaultShader", {}, {{"tex", std::make_pair("metalTexture", "sampler")}},
        "metalMat");

    resourceManager.LoadMaterial(
        "fullScreenShader", {},
        {{"screen", std::make_pair("litSceneTexture", "frameBufferSampler")}},
        "fullScreenMat");


    resourceManager.LoadMaterial(
        "deferredLightingShader",
        {
            {"DirectionalLightDirection", dirLight.direction},
            {"DirectionalLightColor", dirLight.color}
        },
        {
            {"Position", std::make_pair("PositionTexture", "frameBufferSampler")},
            {"Depth", std::make_pair("DepthTexture", "frameBufferSampler")},
            {"Normal", std::make_pair("NormalTexture", "frameBufferSampler")},
            {"Albedo", std::make_pair("colorTargetTexture", "frameBufferSampler")}
        },
        "deferredLightingMat");

    // Scene graph
    auto scene_graph = std::make_unique<SceneGraph>();

    //Sponza
    auto SponzaNode = std::make_unique<SceneNode>();
    SponzaNode->AddComponent(TransformComponent{
        Transform{
            .position = {0.0f, 0.0f, 0.0f},
            .scale = {0.2f, 0.2f, 0.2f},
            .rotation = glm::quat(glm::vec3(0.0f))
        }
    });
    SponzaNode->name = "SponzaNode";
    SponzaNode->mesh = resourceManager.GetMesh("Sponza");
    SponzaNode->material = resourceManager.GetMaterial("metalMat");
    SceneGraph::AddNode(&scene_graph->rootNode, std::move(SponzaNode));

    Camera cam;

    auto camNode = std::make_unique<SceneNode>();
    camNode->AddComponent<TransformComponent>(
        Transform{
            .position = {0.0f, 0.0f, 10.0f},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = glm::quat(glm::vec3(0.0f))
        });
    camNode->AddComponent(CameraComponent{&cam, window});
    camNode->name = "Camera";


    auto lightNode = std::make_unique<SceneNode>();
    lightNode->AddComponent<LightComponent>(&dirLight);
    lightNode->AddComponent<TransformComponent>(
        Transform{
            .position = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = glm::quat(glm::vec3(0.0f))
        });
    lightNode->name = "Directional Light";


    SceneGraph::AddNode(&scene_graph->rootNode, std::move(camNode));
    SceneGraph::AddNode(&scene_graph->rootNode, std::move(lightNode));

    std::vector<RenderObject> offScreenRenderQueue;
    std::vector<RenderObject> deferredLightingRenderQueue;
    std::vector<RenderObject> fullScreenRenderQueue;


    RenderPass OffScreenPass{
        .framebuffer = gBuffer.getId(),
        .size = {window.width(), window.height()},
        .clearColor = true,
        .clearDepth = true,
        .clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f},
        .depthTest = true,
        .blending = false,
        .cullFace = true,
    };

    RenderPass LightingPass{
        .framebuffer = lightingPassBuffer.getId(),
        .size = {window.width(), window.height()},
        .clearColor = true,
        .clearDepth = false,
        .clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f},
        .depthTest = false,
        .blending = false,
        .cullFace = false,
    };


    RenderPass FullScreenPass{
        .framebuffer = 0,
        .size = {window.width(), window.height()},
        .clearColor = true,
        .clearDepth = false,
        .clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f},
        .depthTest = false,
        .blending = false,
        .cullFace = false,
    };


    while (!window.shouldClose()) {
        Window::pollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawSceneGraphWindow(*scene_graph);
        scene_graph->Update(timeManager.GetDeltaTime());

        resourceManager.WatchPaths();

        if (glfwGetKey(window.get(), GLFW_KEY_C) == GLFW_PRESS) {
            resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(
                resourceManager.GetTexture("colorTargetTexture"),
                resourceManager.GetSampler("frameBufferSampler"));
        }

        if (glfwGetKey(window.get(), GLFW_KEY_U) == GLFW_PRESS) {
            resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(
                resourceManager.GetTexture("UvTexture"), resourceManager.GetSampler("frameBufferSampler"));
        }

        if (glfwGetKey(window.get(), GLFW_KEY_P) == GLFW_PRESS) {
            resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(
                resourceManager.GetTexture("PositionTexture"), resourceManager.GetSampler("frameBufferSampler"));
        }

        if (glfwGetKey(window.get(), GLFW_KEY_N) == GLFW_PRESS) {
            resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(
                resourceManager.GetTexture("NormalTexture"), resourceManager.GetSampler("frameBufferSampler"));
        }

        if (glfwGetKey(window.get(), GLFW_KEY_L) == GLFW_PRESS) {
            resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(
                resourceManager.GetTexture("litSceneTexture"), resourceManager.GetSampler("frameBufferSampler"));
        }


        deferredLightingRenderQueue.push_back({
            resourceManager.GetMaterial("deferredLightingMat"),
            glm::mat4(1.0f), resourceManager.GetMesh("QuadForFullScreen")
        });

        fullScreenRenderQueue.push_back({
            resourceManager.GetMaterial("fullScreenMat"),
            glm::mat4(1.0f), resourceManager.GetMesh("QuadForFullScreen")
        });

        timeManager.Tick();


        BuildRenderQueue(&scene_graph->rootNode, offScreenRenderQueue);

        ExecuteRenderPass(OffScreenPass);

        Render(offScreenRenderQueue);
        offScreenRenderQueue.clear();

        resourceManager.GetMaterial("deferredLightingMat")->uniforms["DirectionalLightDirection"] = dirLight.direction;
        resourceManager.GetMaterial("deferredLightingMat")->uniforms["DirectionalLightColor"] = dirLight.color;


        ExecuteRenderPass(LightingPass);
        Render(deferredLightingRenderQueue);
        deferredLightingRenderQueue.clear();


        ExecuteRenderPass(FullScreenPass);
        Render(fullScreenRenderQueue);
        fullScreenRenderQueue.clear();


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteRenderbuffers(1, &lightingPassRBO);

    return 0;
}
