#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mesh.h"
#include "renderPass.h"

class Window;
struct Camera;
struct ResourceManager;

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
};

struct PointLight {
    glm::vec3 color;
    float intensity;
    float radius ;
};

struct SceneNode;

struct Component {
    virtual void DrawUi() {
    }

    virtual void Update(SceneNode &node, float dt) {
    }

    virtual ~Component() = default;
};

struct SceneNode {
    SceneNode();
    explicit SceneNode(const MeshHandle &mesh);

    template<typename T>
    T *AddComponent(T component) {
        auto ptr = std::make_unique<T>(std::move(component));

        T *raw = ptr.get();
        components.push_back(std::move(ptr));

        return raw;
    }

    template<typename T>
    T *GetComponent() {
        for (auto &comp: components) {
            if (auto *casted = dynamic_cast<T *>(comp.get())) {
                return casted;
            }
        }

        return nullptr;
    }

    template<typename T>
    [[nodiscard]] const T *GetComponent() const {
        for (const auto &comp: components) {
            if (const auto *casted = dynamic_cast<const T *>(comp.get())) {
                return casted;
            }
        }

        return nullptr;
    }

    SceneNode *parent = nullptr;
    MeshHandle mesh{};

    std::string name{};

    std::vector<std::unique_ptr<SceneNode> > children;
    std::vector<std::unique_ptr<Component> > components;
};

struct DirectionalLightComponent : Component {
    explicit DirectionalLightComponent(DirectionalLight *light);

    DirectionalLight *light{};

    void DrawUi() override;
};

struct PointLightComponent : Component {
    explicit PointLightComponent(const PointLight &light);

    PointLight light{};

    void DrawUi() override;
};

struct TransformComponent : Component {
    explicit TransformComponent(const Transform &trans);

    Transform transform{};

    glm::mat4 worldTransform{1.0f};

    void DrawUi() override;

    void Update(SceneNode &node, float dt) override;
};

struct CameraComponent : Component {
    CameraComponent(Camera *cam, Window &window);

    Camera *camera = nullptr;
    Window *window = nullptr;

    void DrawUi() override;

    void Update(SceneNode &node, float dt) override;
};

struct SceneGraph {
    SceneNode rootNode;

    static void AddNode(
        SceneNode *parent,
        std::unique_ptr<SceneNode> child,
        ResourceManager &rm);

    void Update(float deltaTime);

private:
    static void UpdateNode(SceneNode *node, float deltaTime);
};

void BuildRenderQueue(const SceneNode *node,
                      std::vector<RenderObject> &renderQueue,
                      ResourceManager &rm);

void SceneGraphNodeInspector(const SceneNode *node, int &counter, const char *label);

void DrawSceneGraphWindow(const SceneGraph &graph);
