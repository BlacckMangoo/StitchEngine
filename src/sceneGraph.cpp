#include "sceneGraph.h"

#include <cassert>

#include "camera.h"
#include "imgui.h"
#include "resourceManager.h"
#include "window.h"

SceneNode::SceneNode() = default;

SceneNode::SceneNode(const MeshHandle &mesh)
    : mesh(mesh) {
}

DirectionalLightComponent::DirectionalLightComponent(DirectionalLight *light)
    : light(light) {
}

void DirectionalLightComponent::DrawUi() {
    ImGui::ColorEdit3("Light Color", &light->color.x);
    ImGui::DragFloat3("Light Direction", &light->direction.x, 0.05f);
}

PointLightComponent::PointLightComponent(const PointLight &light)
    : light(light) {
}

void PointLightComponent::DrawUi() {
    ImGui::ColorEdit3("Light Color", &light.color.x);

    ImGui::DragFloat(
        "Light Intensity",
        &light.intensity,
        0.1f,
        0.0f,
        100.0f);

    ImGui::DragFloat(
        "Radius",
        &light.radius,
        0.1f,
        0.0f,
        100.0f);
}

TransformComponent::TransformComponent(const Transform &trans)
    : transform(trans) {
}

void TransformComponent::DrawUi() {
    auto position = transform.position;
    auto scale = transform.scale;

    auto eulerDegrees = glm::degrees(glm::eulerAngles(transform.rotation));

    if (ImGui::DragFloat3("Position", &position.x, 0.05f)) {
        transform.position = position;
    }

    if (ImGui::DragFloat3("Scale", &scale.x, 0.05f, 0.01f, 100.0f)) {
        transform.scale = scale;
    }

    if (ImGui::DragFloat3("Rotation", &eulerDegrees.x, 0.5f)) {
        transform.rotation = glm::quat(glm::radians(eulerDegrees));
    }
}

void TransformComponent::Update(SceneNode &node, float dt) {
    const glm::mat4 local =
        glm::translate(glm::mat4(1.0f), transform.position) *
        glm::mat4_cast(transform.rotation) *
        glm::scale(glm::mat4(1.0f), transform.scale);

    auto parentWorld = glm::mat4(1.0f);

    if (node.parent) {
        auto *parentTransform = node.parent->GetComponent<TransformComponent>();

        if (parentTransform) {
            parentWorld = parentTransform->worldTransform;
        }
    }

    worldTransform = parentWorld * local;
}

CameraComponent::CameraComponent(Camera *cam, Window &window)
    : camera(cam), window(&window) {
}

void CameraComponent::DrawUi() {
    ImGui::DragFloat("FOV", &camera->fov, 0.5f, 1.0f, 179.0f);
}

void CameraComponent::Update(SceneNode &node, float dt) {
    auto *transform = node.GetComponent<TransformComponent>();

    const glm::mat4 &world = transform ? transform->worldTransform : glm::mat4(1.0f);

    camera->UpdateCamera(*window, world);
}

void SceneGraph::AddNode(
    SceneNode *parent,
    std::unique_ptr<SceneNode> child,
    ResourceManager &rm) {
    assert(parent && child);

    child->parent = parent;

    if (child->name.empty()) {
        child->name = rm.ResolveMesh(child->mesh)
                          ? rm.ResolveMesh(child->mesh)->name
                          : "Node";
    }

    parent->children.push_back(std::move(child));
}

void SceneGraph::Update(const float deltaTime) {
    UpdateNode(&rootNode, deltaTime);
}

void SceneGraph::UpdateNode(SceneNode *node, float deltaTime) {
    for (auto &comp: node->components) {
        if (dynamic_cast<TransformComponent *>(comp.get())) {
            comp->Update(*node, deltaTime);
        }
    }

    for (auto &comp: node->components) {
        if (!dynamic_cast<TransformComponent *>(comp.get())) {
            comp->Update(*node, deltaTime);
        }
    }

    for (auto &child: node->children) {
        UpdateNode(child.get(), deltaTime);
    }
}

void BuildRenderQueue(const SceneNode *node,
                      std::vector<RenderObject> &renderQueue,
                      ResourceManager &rm) {
    if (rm.ResolveMesh(node->mesh) != nullptr) {
        auto model = glm::mat4(1.0f);
        if (auto *transform = node->GetComponent<TransformComponent>()) {
            model = transform->worldTransform;
        }
        for (int i = 0; i < rm.ResolveMesh(node->mesh)->primitives.size(); ++i) {
            RenderObject ro{};

            ro.mesh = node->mesh;
            ro.primIndex = i;
            ro.transform = model;
            ro.material = rm.ResolveMesh(node->mesh)->primitives[i].material;
            renderQueue.push_back(ro);
        }
    }

    for (const auto &child: node->children) {
        BuildRenderQueue(child.get(), renderQueue, rm);
    }
}

void SceneGraphNodeInspector(const SceneNode *node, int &counter, const char *label) {
    ImGui::PushID(node);

    if (ImGui::TreeNode(label, "%s %d", label, counter++)) {
        for (auto &comp: node->components) {
            comp->DrawUi();
        }

        for (auto &child: node->children) {
            const char *childLabel = child->name.empty() ? "Node" : child->name.c_str();

            SceneGraphNodeInspector(child.get(), counter, childLabel);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void DrawSceneGraphWindow(const SceneGraph &graph) {
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin("SceneGraph");

    int counter = 0;

    SceneGraphNodeInspector(&graph.rootNode, counter, "Root");

    ImGui::End();
}

