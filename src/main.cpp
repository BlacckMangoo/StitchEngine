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
#include "timeManager.h"

struct SceneNode;

const std::filesystem::path GLTF_MODEL_PATH = "assets/glTF-Sample-Assets/Models";

const VertexStream cubeVertexStream = MakeVertexStream(cubeInterleavedDataPosNormTex);
const VertexStream quadVertexStream = MakeVertexStream(QuadInterleavedDataPosNormTex);
const VertexStream quadForFullScreenVertexStream = MakeVertexStream(QuadInterleavedData);

inline glm::mat4 CalculateLightSpaceMatrix(const DirectionalLight &dirLight)
{
    glm::vec3 lightDir = glm::normalize(dirLight.direction);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(lightDir, up)) > 0.99f)
    {
        up = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::vec3 right = glm::normalize(glm::cross(up, lightDir));
    up = glm::normalize(glm::cross(lightDir, right));

    glm::mat4 lightView = glm::lookAt(-lightDir * 10.0f, // Position the light source far in the direction opposite to its direction
                                      glm::vec3(0.0f),   // Look at the origin
                                      up);               // Up vector

    float orthoSize = 20.0f;
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 20.0f);

    return lightProjection * lightView;
}

void ImguiInit(const Window &window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init();
}

int main()
{
    Window window(1920, 1080, "GameEngine");

    if (gladLoadGL() == 0)
    {
        std::cout << "Failed to initialize OpenGL context!\n";
        return -1;
    }

    auto timeManager = TimeManager();
    ImguiInit(window);
    ResourceManager resourceManager;

    // Texture

    TextureDescription TexDesc{.internalformat = GL_RGBA8, .hasMipmap = true};
    TextureDescription posDesc{.internalformat = GL_RGB32F, .hasMipmap = false};
    TextureDescription depthDesc{.internalformat = GL_DEPTH_COMPONENT24, .hasMipmap = false};
    auto catTexture = resourceManager.LoadTexture("assets/img.png", TexDesc, "catTexture");
    auto metalTexture = resourceManager.LoadTexture("assets/metal.jpg", TexDesc, "metalTexture");

    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    SamplerDescription samplerDesc{
        .minFilter = GL_LINEAR_MIPMAP_LINEAR,
        .magFilter = GL_LINEAR,
        .wrapS = GL_REPEAT,
        .wrapT = GL_REPEAT,
        .maxAnisotropy = maxAniso,
    };

    auto sampler = resourceManager.LoadSampler(samplerDesc, "sampler");

    TextureDescription rtDesc{.internalformat = GL_RGBA8, .hasMipmap = false};
    SamplerDescription rtSamplerDesc{
        .minFilter = GL_NEAREST,
        .magFilter = GL_NEAREST,
        .wrapS = GL_CLAMP_TO_EDGE,
        .wrapT = GL_CLAMP_TO_EDGE,
        .maxAnisotropy = std::nullopt,
    };

    auto colorTex = resourceManager.CreateRenderTarget(window.width(), window.height(), rtDesc,
                                                       "colorTargetTexture");
    auto PosTex = resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc,
                                                     "PositionTexture");
    auto uvTex = resourceManager.CreateRenderTarget(window.width(), window.height(), rtDesc,
                                                    "UvTexture");
    auto normalTex = resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc,
                                                        "NormalTexture");
    auto depthTex = resourceManager.CreateRenderTarget(window.width(), window.height(), depthDesc,
                                                       "DepthTexture");

    // Sampler frameBufferSampler(rtSamplerDesc);
    resourceManager.LoadSampler(rtSamplerDesc, "frameBufferSampler");

    FrameBuffer shadowMapBuffer;
    auto shadowMapTex = resourceManager.CreateRenderTarget(2048, 2048, depthDesc, "shadowMapTexture");
    shadowMapBuffer.AddDepthAttachmentTexture(resourceManager.ResolveTexture(shadowMapTex)->id);
    shadowMapBuffer.FinalizeDepthOnly();
    shadowMapBuffer.Validate();

    FrameBuffer gBuffer;

    gBuffer.AddColorAttachment(resourceManager.ResolveTexture(colorTex)->id);
    gBuffer.AddColorAttachment(resourceManager.ResolveTexture(PosTex)->id);
    gBuffer.AddColorAttachment(resourceManager.ResolveTexture(uvTex)->id);
    gBuffer.AddColorAttachment(resourceManager.ResolveTexture(normalTex)->id);
    gBuffer.AddDepthAttachmentTexture(resourceManager.ResolveTexture(depthTex)->id);

    gBuffer.FinalizeColorAttachments();
    gBuffer.Validate();

    // Lighting Pass Frame Buffer setup
    unsigned int lightingPassRBO;
    glCreateRenderbuffers(1, &lightingPassRBO);
    glNamedRenderbufferStorage(lightingPassRBO, GL_DEPTH_COMPONENT24, window.width(), window.height());

    auto litSceneTex = resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc, "litSceneTexture");

    FrameBuffer lightingPassBuffer;

    lightingPassBuffer.AddColorAttachment(resourceManager.ResolveTexture(litSceneTex)->id);
    lightingPassBuffer.AddDepthAttachmentRenderBuffer(lightingPassRBO);
    lightingPassBuffer.FinalizeColorAttachments();
    lightingPassBuffer.Validate();

    auto defaultShader = resourceManager.LoadShader("shaders/vertexShader.vert",
                                                    "shaders/fragShader.frag", "defaultShader");
    auto fsShader = resourceManager.LoadShader("shaders/frameBufferVert.vert",
                                               "shaders/frameBufferFrag.frag",
                                               "fullScreenShader");
    auto deferredLitShad = resourceManager.LoadShader("shaders/frameBufferVert.vert",
                                                      "shaders/defferedLighting.frag",
                                                      "deferredLightingShader");
    auto shadowMapShader = resourceManager.LoadShader("shaders/shadowMap.vert",
                                                      "shaders/shadowMap.frag",
                                                      "shadowMapShader");

    DirectionalLight dirLight{
        .direction = {0.5f, 1.0f, 0.5f},
        .color = {1.0f, 1.0f, 1.0f}};

    std::vector<PointLight> pointLights{
        {.color = {1.0f, 0.0f, 0.0f},
         .intensity = 1.0f,
         .radius = 5.0f},
        {.color = {0.0f, 1.0f, 0.0f},
         .intensity = 1.0f,
         .radius = 5.0f},
        {.color = {0.0f, 0.0f, 1.0f},
         .intensity = 1.0f,
         .radius = 5.0f}};

    std::cout << glGetString(GL_RENDERER) << '\n';

    auto catMat = resourceManager.LoadMaterial(defaultShader, {}, {{"baseColor", std::make_pair("catTexture", "sampler")}},
                                               "catMat");

    auto metalMat = resourceManager.LoadMaterial(defaultShader, {}, {{"baseColor", std::make_pair("metalTexture", "sampler")}},
                                                 "metalMat");

    auto fullScreenMat = resourceManager.LoadMaterial(fsShader, {},
                                                      {{"screen", std::make_pair("litSceneTexture", "frameBufferSampler")}},
                                                      "fullScreenMat");

    auto litSceneMat = resourceManager.LoadMaterial(
        deferredLitShad,
        {{"DirectionalLightDirection", dirLight.direction},
         {"DirectionalLightColor", dirLight.color}},
        {{"Position", std::make_pair("PositionTexture", "frameBufferSampler")},
         {"Depth", std::make_pair("DepthTexture", "frameBufferSampler")},
         {"Normal", std::make_pair("NormalTexture", "frameBufferSampler")},
         {"Albedo", std::make_pair("colorTargetTexture", "frameBufferSampler")},
         {"ShadowMap", std::make_pair("shadowMapTexture", "frameBufferSampler")}},
        "deferredLightingMat");

    auto shadowMapMat = resourceManager.LoadMaterial(shadowMapShader, {}, {}, "shadowMapMat");

    // Meshes
    std::vector<GlPrimitive> quadPrimitives;
    quadPrimitives.emplace_back(
        std::vector<VertexStream>{quadVertexStream},
        std::vector<unsigned int>{quadIndices, quadIndices + 6},
        PosNormTexLayout);

    std::vector<GlPrimitive> quadForFullScreenPrimitives;
    for (const auto &p : quadPrimitives)
    {
        p.material = litSceneMat;
    }

    quadForFullScreenPrimitives.emplace_back(
        std::vector<VertexStream>{quadForFullScreenVertexStream},
        std::vector<unsigned int>{quadIndices, quadIndices + 6}, PosTexLayout);

    for (const auto &p : quadForFullScreenPrimitives)
    {
        p.material = fullScreenMat;
    }

    std::vector<GlPrimitive> cubePrimitives;
    cubePrimitives.emplace_back(std::vector<VertexStream>{cubeVertexStream},
                                std::vector<unsigned int>{indices, indices + 36}, PosNormTexLayout);

    resourceManager.LoadMesh(std::move(quadForFullScreenPrimitives),
                             "QuadForFullScreen");

    std::vector<MeshHandle> meshes;
    meshes.emplace_back(resourceManager.LoadMesh(std::move(quadPrimitives), "Quad"));
    meshes.emplace_back(resourceManager.LoadMesh(std::move(cubePrimitives), "Cube"));

    std::vector<MeshHandle> gltfMeshes;
    gltfMeshes.emplace_back(resourceManager.LoadGLTFMesh(
        GLTF_MODEL_PATH / "Sponza/glTF/Sponza.gltf", "Sponza"));

    for (auto &p : gltfMeshes)
    {
        for (auto &prim : resourceManager.ResolveMesh(p)->primitives)
        {
            resourceManager.ResolveMaterial(prim.material)->shader = defaultShader;
        }
    }

    // Scene graph
    auto scene_graph = std::make_unique<SceneGraph>();

    // Sponza
    auto SponzaNode = std::make_unique<SceneNode>();
    SponzaNode->AddComponent(TransformComponent{
        Transform{
            .position = {0.0f, 0.0f, 0.0f},
            .scale = {0.02f, 0.02f, 0.02f},
            .rotation = glm::quat(glm::vec3(0.0f))}});

    SponzaNode->name = "SponzaNode";
    SponzaNode->mesh = resourceManager.GetMesh("Sponza");
    SceneGraph::AddNode(&scene_graph->rootNode, std::move(SponzaNode), resourceManager);

    Camera cam;

    auto camNode = std::make_unique<SceneNode>();
    camNode->AddComponent<TransformComponent>(
        Transform{
            .position = {0.0f, 0.0f, 10.0f},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = glm::quat(glm::vec3(0.0f))});
    camNode->AddComponent(CameraComponent{&cam, window});
    camNode->name = "Camera";

    auto lightNode = std::make_unique<SceneNode>();
    lightNode->AddComponent<DirectionalLightComponent>(&dirLight);
    lightNode->AddComponent<TransformComponent>(
        Transform{
            .position = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = glm::quat(glm::vec3(0.0f))});
    lightNode->name = "Directional Light";

    for (const auto &light : pointLights)
    {
        auto ln = std::make_unique<SceneNode>();
        ln->AddComponent<TransformComponent>(Transform{
            .position = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = glm::quat(glm::vec3(0.0f))});
        ln->name = "PointLight";
        ln->AddComponent<PointLightComponent>(light);
        SceneGraph::AddNode(&scene_graph->rootNode, std::move(ln), resourceManager);
    }

    SceneGraph::AddNode(&scene_graph->rootNode, std::move(camNode), resourceManager);
    SceneGraph::AddNode(&scene_graph->rootNode, std::move(lightNode), resourceManager);

    std::vector<RenderObject> shadowMapRenderQueue;
    std::vector<RenderObject> offScreenRenderQueue;
    std::vector<RenderObject> deferredLightingRenderQueue;
    std::vector<RenderObject> fullScreenRenderQueue;

    RenderPass shadowMapPass{
        .framebuffer = shadowMapBuffer.getId(),
        .size = {2048, 2048},
        .clearColor = false,
        .clearDepth = true,
        .clearColorValue = {1.0f, 1.0f, 1.0f, 1.0f},
        .depthTest = true,
        .blending = false,
        .cullFace = false,
    };

    RenderPass OffScreenPass{
        .framebuffer = gBuffer.getId(),
        .size = {window.width(), window.height()},
        .clearColor = true,
        .clearDepth = true,
        .clearColorValue = {0.0f, 0.0f, 0.0f, 1.0f},
        .depthTest = true,
        .blending = false,
        .cullFace = false,
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

    int noOfPointLights = pointLights.size();

    while (!window.shouldClose())
    {
        Window::pollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawSceneGraphWindow(*scene_graph);
        scene_graph->Update(timeManager.GetDeltaTime());

        resourceManager.WatchPaths();

        // debug G buffer
        {
            if (glfwGetKey(window.get(), GLFW_KEY_C) == GLFW_PRESS)
            {
                resourceManager.ResolveMaterial(resourceManager.GetMaterial("fullScreenMat"))->textures["screen"] = std::make_pair(
                    resourceManager.GetTexture("colorTargetTexture"),
                    resourceManager.GetSampler("frameBufferSampler"));
            }

            if (glfwGetKey(window.get(), GLFW_KEY_U) == GLFW_PRESS)
            {
                resourceManager.ResolveMaterial(resourceManager.GetMaterial("fullScreenMat"))->textures["screen"] = std::make_pair(
                    resourceManager.GetTexture("UvTexture"), resourceManager.GetSampler("frameBufferSampler"));
            }

            if (glfwGetKey(window.get(), GLFW_KEY_P) == GLFW_PRESS)
            {
                resourceManager.ResolveMaterial(resourceManager.GetMaterial("fullScreenMat"))->textures["screen"] = std::make_pair(
                    resourceManager.GetTexture("PositionTexture"), resourceManager.GetSampler("frameBufferSampler"));
            }

            if (glfwGetKey(window.get(), GLFW_KEY_N) == GLFW_PRESS)
            {
                resourceManager.ResolveMaterial(resourceManager.GetMaterial("fullScreenMat"))->textures["screen"] = std::make_pair(
                    resourceManager.GetTexture("NormalTexture"), resourceManager.GetSampler("frameBufferSampler"));
            }

            if (glfwGetKey(window.get(), GLFW_KEY_L) == GLFW_PRESS)
            {
                resourceManager.ResolveMaterial(resourceManager.GetMaterial("fullScreenMat"))->textures["screen"] = std::make_pair(
                    resourceManager.GetTexture("litSceneTexture"), resourceManager.GetSampler("frameBufferSampler"));
            }

            if (glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS)
            {
                resourceManager.ResolveMaterial(resourceManager.GetMaterial("fullScreenMat"))->textures["screen"] = std::make_pair(
                    resourceManager.GetTexture("shadowMapTexture"), resourceManager.GetSampler("frameBufferSampler"));
            }
        }

        deferredLightingRenderQueue.push_back({resourceManager.GetMaterial("deferredLightingMat"), glm::mat4(1.0f), resourceManager.GetMesh("QuadForFullScreen")});
        fullScreenRenderQueue.push_back({fullScreenMat, glm::mat4(1.0f), resourceManager.GetMesh("QuadForFullScreen")});
        timeManager.Tick();

        auto &shadowMapUniforms = resourceManager.ResolveMaterial(shadowMapMat)->uniforms;
        shadowMapUniforms["MLP"] = CalculateLightSpaceMatrix(dirLight);

        BuildShadowMapRenderQueue(&scene_graph->rootNode, shadowMapRenderQueue, resourceManager, shadowMapMat);
        ExecuteRenderPass(shadowMapPass);
        Render(shadowMapRenderQueue, resourceManager);
        shadowMapRenderQueue.clear();

        BuildRenderQueue(&scene_graph->rootNode, offScreenRenderQueue, resourceManager);
        ExecuteRenderPass(OffScreenPass);

        Render(offScreenRenderQueue, resourceManager);
        offScreenRenderQueue.clear();

        int lightIndex = 0;

        for (const auto &node : scene_graph->rootNode.children)
        {
            auto *transformComp = node->GetComponent<TransformComponent>();
            auto *pointLightComp = node->GetComponent<PointLightComponent>();

            if (transformComp && pointLightComp)
            {
                auto &uniforms = resourceManager.ResolveMaterial(litSceneMat)->uniforms;

                uniforms["pointLights[" + std::to_string(lightIndex) + "].color"] = pointLightComp->light.color;
                uniforms["pointLights[" + std::to_string(lightIndex) + "].intensity"] = pointLightComp->light.intensity;
                uniforms["pointLights[" + std::to_string(lightIndex) + "].radius"] = pointLightComp->light.radius;
                uniforms["pointLights[" + std::to_string(lightIndex) + "].position"] = transformComp->transform.position;

                ++lightIndex;
            }
        }

        auto &uniforms = resourceManager.ResolveMaterial(litSceneMat)->uniforms;
        uniforms["pointLightCount"] = lightIndex;
        uniforms["DirectionalLightColor"] = dirLight.color;
        uniforms["DirectionalLightDirection"] = glm::normalize(dirLight.direction);
        uniforms["MLP"] = CalculateLightSpaceMatrix(dirLight);

        ExecuteRenderPass(LightingPass);
        Render(deferredLightingRenderQueue, resourceManager);
        deferredLightingRenderQueue.clear();

        ExecuteRenderPass(FullScreenPass);
        Render(fullScreenRenderQueue, resourceManager);
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
