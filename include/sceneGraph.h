#pragma ocnce

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
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
    SceneNode() = default;

    SceneNode(Mesh *mesh, Material *mat)
        : mesh(mesh), material(mat) {
    }

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

    Mesh *mesh = nullptr;
    Material *material = nullptr;

    std::string name{};

    std::vector<std::unique_ptr<SceneNode> > children;
    std::vector<std::unique_ptr<Component> > components;
};

struct LightComponent : Component {
    LightComponent(DirectionalLight *light)
        : light(light) {
    }

    DirectionalLight *light{};

    void DrawUi() override {
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
    TransformComponent(const Transform &trans)
        : transform(trans) {
    }

    Transform transform{};

    glm::mat4 worldTransform{1.0f};

    void DrawUi() override {
        auto position = transform.position;
        auto scale = transform.scale;

        auto eulerDegrees =
                glm::degrees(
                    glm::eulerAngles(transform.rotation)
                );

        if (ImGui::DragFloat3(
            "Position",
            &position.x,
            0.05f)) {
            transform.position = position;
        }

        if (ImGui::DragFloat3(
            "Scale",
            &scale.x,
            0.05f,
            0.01f,
            100.0f)) {
            transform.scale = scale;
        }

        if (ImGui::DragFloat3(
            "Rotation",
            &eulerDegrees.x,
            0.5f)) {
            transform.rotation =
                    glm::quat(glm::radians(eulerDegrees));
        }
    }

    void Update(SceneNode &node, float dt) override {
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

        if (node.parent) {
            auto *parentTransform =
                    node.parent->GetComponent<TransformComponent>();

            if (parentTransform) {
                parentWorld =
                        parentTransform->worldTransform;
            }
        }

        worldTransform = parentWorld * local;
    }
};

struct CameraComponent : Component {
    CameraComponent(Camera *cam, Window &window)
        : camera(cam), window(&window) {
    }

    Camera *camera = nullptr;
    Window *window = nullptr;

    void DrawUi() override {
        ImGui::DragFloat(
            "FOV",
            &camera->fov,
            0.5f,
            1.0f,
            179.0f
        );
    }

    void Update(SceneNode &node, float dt) override {
        auto *transform =
                node.GetComponent<TransformComponent>();

        const glm::mat4 &world =
                transform
                    ? transform->worldTransform
                    : glm::mat4(1.0f);

        camera->UpdateCamera(*window, world);
    }
};

struct SceneGraph {
    SceneNode rootNode;

    static void AddNode(
        SceneNode *parent,
        std::unique_ptr<SceneNode> child) {
        assert(parent && child);

        child->parent = parent;

        if (child->name.empty()) {
            child->name =
                    child->mesh
                        ? child->mesh->name
                        : "Node";
        }

        parent->children.push_back(
            std::move(child)
        );
    }

    void Update(const float deltaTime) {
        UpdateNode(&rootNode, deltaTime);
    }

private:
    static void UpdateNode(SceneNode *node, float deltaTime) {
        // pass 1 : transforms first

        for (auto &comp: node->components) {
            if (dynamic_cast<TransformComponent *>(comp.get())) {
                comp->Update(*node, deltaTime);
            }
        }

        // pass 2 : everything else

        for (auto &comp: node->components) {
            if (!dynamic_cast<TransformComponent *>(comp.get())) {
                comp->Update(*node, deltaTime);
            }
        }

        for (auto &child: node->children) {
            UpdateNode(child.get(), deltaTime);
        }
    }
};

void BuildRenderQueue(
    const SceneNode *node,
    std::vector<RenderObject> &renderQueue) {
    if (node->mesh && node->material) {
        auto model = glm::mat4(1.0f);

        if (auto *transform =
                node->GetComponent<TransformComponent>()) {
            model = transform->worldTransform;
        }

        renderQueue.push_back({
            node->material,
            model,
            node->mesh
        });
    }

    for (const auto &child: node->children) {
        BuildRenderQueue(child.get(), renderQueue);
    }
}


static void SceneGraphNodeInspector(
    const SceneNode *node,
    int &counter,
    const char *label) {
    ImGui::PushID(node);

    if (ImGui::TreeNode(
        label,
        "%s %d",
        label,
        counter++
    )) {
        for (auto &comp: node->components) {
            comp->DrawUi();
        }

        for (auto &child: node->children) {
            const char *childLabel =
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
    const SceneGraph &graph) {
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
