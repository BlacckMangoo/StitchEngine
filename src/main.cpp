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

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include "glm/gtc/type_ptr.hpp"

struct SceneNode;

const std::filesystem::path GLTF_MODEL_PATH = "assets/glTF-Sample-Assets/Models";

const VertexStream cubeVertexStream = MakeVertexStream(cubeInterleavedDataPosNormTex);
const VertexStream quadVertexStream = MakeVertexStream(QuadInterleavedDataPosNormTex);
const VertexStream quadForFullScreenVertexStream = MakeVertexStream(QuadInterleavedData);



void ImguiInit(const Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForOpenGL(window.get(), true);
    ImGui_ImplOpenGL3_Init();
}


struct DirectionalLight {
	glm::vec3 direction;
	glm::vec3 color;
};


struct Component {
    virtual void DrawUi() {}
    virtual void Update(SceneNode& node, float dt) {}
    virtual ~Component() = default;
};

struct SceneNode {

    SceneNode() = default;

    SceneNode(Mesh* mesh, Material* mat)
        : mesh(mesh), material(mat) {
    }

    template<typename T>
    T* AddComponent(T component)
    {
        auto ptr = std::make_unique<T>(std::move(component));

        T* raw = ptr.get();

        components.push_back(std::move(ptr));

        return raw;
    }

    template<typename T>
    T* GetComponent()
    {
        for (auto& comp : components)
        {
            if (auto* casted = dynamic_cast<T*>(comp.get()))
            {
                return casted;
            }
        }

        return nullptr;
    }

    template<typename T>
    [[nodiscard]] const T* GetComponent() const
    {
        for (const auto& comp : components)
        {
            if (const auto* casted = dynamic_cast<const T*>(comp.get()))
            {
                return casted;
            }
        }

        return nullptr;
    }

    SceneNode* parent = nullptr;

    Mesh* mesh = nullptr;
    Material* material = nullptr;

    std::string name{};

    std::vector<std::unique_ptr<SceneNode>> children;
    std::vector<std::unique_ptr<Component>> components;
};

struct LightComponent : Component {
    LightComponent(DirectionalLight* light)
        : light(light) {
    }
    DirectionalLight* light{};
    void DrawUi() override
    {
        //color 
        ImGui::ColorEdit3(
            "Light Color", &light->color.x);
        ImGui::DragFloat3(
            "Light Direction",
            &light->direction.x,
            0.05f);
    }

};

struct TransformComponent : Component {

    TransformComponent(const Transform& trans)
        : transform(trans) {
    }

    Transform transform{};

    glm::mat4 worldTransform{1.0f};

    void DrawUi() override
    {
        auto position = transform.position;
        auto scale = transform.scale;

        auto eulerDegrees =
            glm::degrees(
                glm::eulerAngles(transform.rotation)
            );

        if (ImGui::DragFloat3(
            "Position",
            &position.x,
            0.05f))
        {
            transform.position = position;
        }

        if (ImGui::DragFloat3(
            "Scale",
            &scale.x,
            0.05f,
            0.01f,
            100.0f))
        {
            transform.scale = scale;
        }

        if (ImGui::DragFloat3(
            "Rotation",
            &eulerDegrees.x,
            0.5f))
        {
            transform.rotation =
                glm::quat(glm::radians(eulerDegrees));
        }
    }

    void Update(SceneNode& node, float dt) override
    {
        const glm::mat4 local =
            glm::translate(
                glm::mat4(1.0f),
                transform.position
            )
            *
            glm::mat4_cast(transform.rotation)
            *
            glm::scale(
                glm::mat4(1.0f),
                transform.scale
            );

        auto parentWorld = glm::mat4(1.0f);

        if (node.parent)
        {
            auto* parentTransform =
                node.parent->GetComponent<TransformComponent>();

            if (parentTransform)
            {
                parentWorld =
                    parentTransform->worldTransform;
            }
        }

        worldTransform = parentWorld * local;
    }
};

struct CameraComponent : Component {

    CameraComponent(Camera* cam, Window& window)
        : camera(cam), window(&window) {
    }

    Camera* camera = nullptr;
    Window* window = nullptr;

    void DrawUi() override
    {
        ImGui::DragFloat(
            "FOV",
            &camera->fov,
            0.5f,
            1.0f,
            179.0f
        );
    }

    void Update(SceneNode& node, float dt) override
    {
        auto* transform =
            node.GetComponent<TransformComponent>();

        const glm::mat4& world =
            transform
            ? transform->worldTransform
            : glm::mat4(1.0f);

        camera->UpdateCamera(*window, world);
    }
};

struct SceneGraph {

    SceneNode rootNode;

    static void AddNode(
        SceneNode* parent,
        std::unique_ptr<SceneNode> child)
    {
        assert(parent && child);

        child->parent = parent;

        if (child->name.empty())
        {
            child->name =
                child->mesh
                ? child->mesh->name
                : "Node";
        }

        parent->children.push_back(
            std::move(child)
        );
    }

    void Update(const float deltaTime)
    {
        UpdateNode(&rootNode,deltaTime);
    }

private:

    static void UpdateNode(SceneNode* node,float deltaTime)
    {
        // pass 1 : transforms first

        for (auto& comp : node->components)
        {
            if (dynamic_cast<TransformComponent*>(comp.get()))
            {
                comp->Update(*node, deltaTime);
            }
        }

        // pass 2 : everything else

        for (auto& comp : node->components)
        {
            if (!dynamic_cast<TransformComponent*>(comp.get()))
            {
                comp->Update(*node, deltaTime);
            }
        }

        for (auto& child : node->children)
        {
            UpdateNode(child.get(),deltaTime);
        }
    }
};

void BuildRenderQueue(
    const SceneNode* node,
    std::vector<RenderObject>& renderQueue)
{
    if (node->mesh && node->material)
    {
        auto model = glm::mat4(1.0f);

        if (auto* transform =
            node->GetComponent<TransformComponent>())
        {
            model = transform->worldTransform;
        }

        renderQueue.push_back({
            node->material,
            model,
            node->mesh
        });
    }

    for (const auto& child : node->children)
    {
        BuildRenderQueue(child.get(), renderQueue);
    }
}



static void SceneGraphNodeInspector(
    const SceneNode* node,
    int& counter,
    const char* label)
{
    ImGui::PushID(node);

    if (ImGui::TreeNode(
        label,
        "%s %d",
        label,
        counter++
    ))
    {
        for (auto& comp : node->components)
        {
            comp->DrawUi();
        }

        for (auto& child : node->children)
        {
            const char* childLabel =
                child->name.empty()
                ? "Node"
                : child->name.c_str();

            SceneGraphNodeInspector(
                child.get(),
                counter,
                childLabel
            );
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

static void DrawSceneGraphWindow(
    const SceneGraph& graph)
{
    ImGui::SetNextWindowSize(
        ImVec2(300, 400),
        ImGuiCond_FirstUseEver
    );

    ImGui::Begin("SceneGraph");

    int counter = 0;

    SceneGraphNodeInspector(
        &graph.rootNode,
        counter,
        "Root"
    );

    ImGui::End();
}

struct alignas(16) TimeUniformBufferObject {
    float deltaTime  = 0.0;
    float time  =0.0;
    float pad1 =0.0;
    float pad2 =0.0;
};


struct TimeManager {

    TimeManager() {
        glCreateBuffers(1, &ubo);
        glNamedBufferData(ubo, sizeof(TimeUniformBufferObject), nullptr,GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo);
    }

    void Tick() {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        timeUniformBufferData.deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        timeUniformBufferData.time += timeUniformBufferData.deltaTime;

        glNamedBufferSubData(ubo, 0, sizeof(TimeUniformBufferObject), &timeUniformBufferData);
    }

    [[nodiscard]] float GetDeltaTime() const  {
        return timeUniformBufferData.deltaTime ;
    }
    [[nodiscard]] float GetCurrentTime() const {
        return timeUniformBufferData.time ;
    }


    TimeUniformBufferObject timeUniformBufferData{};
    float lastFrame = 0.0f;
    unsigned int ubo{} ;

    ~TimeManager() {
        glDeleteBuffers(1, &ubo);
    }

};


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
    cubePrimitives.emplace_back(std::vector<VertexStream>{cubeVertexStream},std::vector<unsigned int>{indices, indices + 36},PosNormTexLayout);



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
TextureDescription depthDesc {.internalformat = GL_DEPTH_COMPONENT24, .hasMipmap = false};
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
    resourceManager.CreateRenderTarget(window.width(), window.height(),depthDesc,
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
  unsigned int lightingPassRBO ; 
  glCreateRenderbuffers(1, &lightingPassRBO);
  glNamedRenderbufferStorage(lightingPassRBO, GL_DEPTH_COMPONENT24, window.width(), window.height());


  resourceManager.CreateRenderTarget(window.width(), window.height(), posDesc,"litSceneTexture");

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

  
  DirectionalLight dirLight{.direction = {0.5f, 1.0f, 0.5f},
                            .color = {1.0f, 1.0f, 1.0f}};


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
      {{"DirectionalLightDirection", dirLight.direction},
       {"DirectionalLightColor", dirLight.color}},
      {{"Position", std::make_pair("PositionTexture", "frameBufferSampler")},
          {"Depth",std::make_pair("DepthTexture", "frameBufferSampler")},
       {"Normal", std::make_pair("NormalTexture", "frameBufferSampler")},
       {"Albedo", std::make_pair("colorTargetTexture", "frameBufferSampler")}},
      "deferredLightingMat");

  // Scene graph
  auto scene_graph = std::make_unique<SceneGraph>();

    //Sponza
    auto SponzaNode = std::make_unique<SceneNode>();
    SponzaNode->AddComponent(TransformComponent{Transform{
    .position = {0.0f, 0.0f, 0.0f},
    .scale = {0.2f, 0.2f, 0.2f},
    .rotation = glm::quat(glm::vec3(0.0f))}});
    SponzaNode->name = "SponzaNode";
    SponzaNode->mesh = resourceManager.GetMesh("Sponza");
    SponzaNode->material = resourceManager.GetMaterial("metalMat");
    SceneGraph::AddNode(&scene_graph->rootNode, std::move(SponzaNode));

  Camera cam;

  auto camNode = std::make_unique<SceneNode>();
  camNode->AddComponent<TransformComponent>(
      Transform{.position = {0.0f, 0.0f, 10.0f},
                .scale = {1.0f, 1.0f, 1.0f},
                .rotation = glm::quat(glm::vec3(0.0f))});
  camNode->AddComponent(CameraComponent{&cam, window});
  camNode->name = "Camera";


  auto lightNode = std::make_unique<SceneNode>();
  lightNode->AddComponent<LightComponent>(&dirLight);
  lightNode->AddComponent<TransformComponent>(
      Transform{.position = {0.0f, 0.0f, 0.0f},
                .scale = {1.0f, 1.0f, 1.0f},
                .rotation = glm::quat(glm::vec3(0.0f))});
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

		if (glfwGetKey(window.get(), GLFW_KEY_C) == GLFW_PRESS)
		{
			resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(resourceManager.GetTexture("colorTargetTexture"),
				resourceManager.GetSampler("frameBufferSampler"));
		}

		if (glfwGetKey(window.get(), GLFW_KEY_U) == GLFW_PRESS)
		{
			resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(resourceManager.GetTexture("UvTexture"), resourceManager.GetSampler			("frameBufferSampler"));
		}
					
		if (glfwGetKey(window.get(), GLFW_KEY_P) == GLFW_PRESS)
		{
			resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(resourceManager.GetTexture("PositionTexture"), resourceManager.GetSampler("frameBufferSampler"));
		}

		if (glfwGetKey(window.get(), GLFW_KEY_N) == GLFW_PRESS)
		{
			resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(resourceManager.GetTexture("NormalTexture"), resourceManager.GetSampler("frameBufferSampler"));
		}

	    if (glfwGetKey(window.get(), GLFW_KEY_L) == GLFW_PRESS)
	    {
	        resourceManager.GetMaterial("fullScreenMat")->textures["screen"] = std::make_pair(resourceManager.GetTexture("litSceneTexture"), resourceManager.GetSampler("frameBufferSampler"));
	    }


		deferredLightingRenderQueue.push_back({ resourceManager.GetMaterial("deferredLightingMat"),
			glm::mat4(1.0f), resourceManager.GetMesh("QuadForFullScreen") });

		fullScreenRenderQueue.push_back({ resourceManager.GetMaterial("fullScreenMat"),
			glm::mat4(1.0f), resourceManager.GetMesh("QuadForFullScreen") });

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

    glDeleteRenderbuffers(1,&lightingPassRBO);

	return 0;
}