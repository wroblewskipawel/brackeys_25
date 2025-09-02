#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

#include "EntityComponentSystem/Components/MovableComponent.hpp"
#include "EntityComponentSystem/Components/RenderableComponent.hpp"
#include "EntityComponentSystem/ECS.hpp"
#include "EntityComponentSystem/Systems/CollidingSystem.hpp"
#include "EntityComponentSystem/Systems/CollisionResolutionSystem.hpp"
#include "EntityComponentSystem/Systems/MovementSystem.hpp"
#include "EntityComponentSystem/Systems/PlayerMovementSystem.hpp"
#include "EntityComponentSystem/Systems/RemoveEntitySystem.hpp"
#include "EntityComponentSystem/Systems/RenderingSystem.hpp"
#include "ImGui/ImGui.hpp"
#include "InputHandler/InputHandler.hpp"
#include "MusicManager/MusicManager.hpp"
#include "debug.h"
#include "gltf.h"
#include "model.h"
#include "renderer.h"
#include "shader.h"
#include "std140.h"

void debugSystem(ECS& ecs, const float& deltaTime,
                 RenderingQueues& renderingQueues) {
    auto& movables = ecs.getEntitiesWithComponent<MovableComponent>().get();
    // std::cout << "Debug: " << movables.size() << " movables tracked.\n";
}

constexpr float SCREEN_WIDTH = 1280;
constexpr float SCREEN_HEIGHT = 720;

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error("Failed to initialize OpenGL");
    };

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                          GL_TRUE);
#endif

    glfwSetKeyCallback(window, keyCallback);
    setupImGui(window);

    DocumentReader<UnlitVertex, UnlitMaterial> unlitDocument{
        "assets/WaterBottle/glTF/WaterBottle.gltf"};

    DocumentReader<UnlitVertex, UnlitMaterial> hexGrass{
        "assets/hexGrass/hex_grass.gltf"};

    DocumentReader<UnlitVertex, UnlitMaterial> unlitBarrel{
        "assets/barrel/barrel.gltf"};

    DocumentReader<UnlitVertex, UnlitMaterial> unlitMountain{
        "assets/mountains/mountain_B.gltf"};

    MaterialPackBuilder<EmptyMaterial> emptyMaterialPackBuilder{};
    auto emptyMaterial =
        emptyMaterialPackBuilder.addMaterial(MaterialBuilder<EmptyMaterial>{});
    auto emptyMaterialPack = emptyMaterialPackBuilder.build();

    MeshPackBuilder<UnlitVertex> unlitMeshPackBuilder{};
    auto unlitCubeMesh =
        unlitMeshPackBuilder.addMesh(createCube<UnlitVertex>());
    auto documentMeshes =
        unlitMeshPackBuilder.addMeshMulti(unlitDocument.takeMeshes());
    auto grassMeshes =
        unlitMeshPackBuilder.addMeshMulti(hexGrass.takeMeshes());
    auto barrelMeshes =
        unlitMeshPackBuilder.addMeshMulti(unlitBarrel.takeMeshes());
    auto mountainMeshes =
        unlitMeshPackBuilder.addMeshMulti(unlitMountain.takeMeshes());
    auto unlitMeshPack = unlitMeshPackBuilder.build();

    MaterialPackBuilder<UnlitMaterial> unlitMaterialPackBuilder{};
    MaterialBuilder<UnlitMaterial> unlitMaterialBuilder_1{};
    unlitMaterialBuilder_1.setAlbedoTextureData(TextureData::loadFromFile(
        "assets/textures/tile_1.png", TextureFormat::RGB));
    auto unlitMaterial_1 =
        unlitMaterialPackBuilder.addMaterial(unlitMaterialBuilder_1);
    MaterialBuilder<UnlitMaterial> unlitMaterialBuilder_2{};
    unlitMaterialBuilder_2.setAlbedoTextureData(TextureData::loadFromFile(
        "assets/textures/tile_2.png", TextureFormat::RGB));
    auto documentMaterials = unlitMaterialPackBuilder.addMaterialMulti(
        unlitDocument.takeMaterials());
    auto grassMaterials = unlitMaterialPackBuilder.addMaterialMulti(
        hexGrass.takeMaterials());
    auto barrelMaterials = unlitMaterialPackBuilder.addMaterialMulti(
        unlitBarrel.takeMaterials());
    auto mountainMaterials = unlitMaterialPackBuilder.addMaterialMulti(
        unlitMountain.takeMaterials());
    auto unlitMaterialPack = unlitMaterialPackBuilder.build();

    ShaderBuilder unlitStaticShaderBuilder{};
    unlitStaticShaderBuilder.addStage(ShaderStage::Vertex,
                                      "shaders/unlit/shader.vert");
    unlitStaticShaderBuilder.addStage(ShaderStage::Fragment,
                                      "shaders/unlit/shader.frag");
    auto unlitStaticShader = unlitStaticShaderBuilder.build();

    ShaderBuilder unlitDynamicShaderBuilder{};
    unlitDynamicShaderBuilder.addStage(ShaderStage::Vertex,
                                       "shaders/dynamic/unlit/shader.vert");
    unlitDynamicShaderBuilder.addStage(ShaderStage::Fragment,
                                       "shaders/dynamic/unlit/shader.frag");
    auto unlitDynamicShader = unlitDynamicShaderBuilder.build();

    auto unlitDrawPack = unlitMeshPack.createDrawPack(unlitMaterialPack);
    auto unlitStaticStage = Stage(std::move(unlitDrawPack));
    unlitStaticStage.setShader(unlitStaticShader);

    DynamicDrawPack<UnlitVertex, UnlitMaterial> dynamicUnlitDrawPack(
        unlitMeshPack, unlitMaterialPack);
    auto dynamicUnlitQueue = dynamicUnlitDrawPack.getDrawQueue();
    auto dynamicUnlitStage = DynamicStage(std::move(dynamicUnlitDrawPack));
    dynamicUnlitStage.setShader(unlitDynamicShader);

    MeshPackBuilder<ColoredVertex> coloredMeshPackBuilder{};
    auto coloredCubeMesh =
        coloredMeshPackBuilder.addMesh(createCube<ColoredVertex>());
    auto coloredMeshPack = coloredMeshPackBuilder.build();

    ShaderBuilder coloredStaticShaderBuilder{};
    coloredStaticShaderBuilder.addStage(ShaderStage::Vertex,
                                        "shaders/colored/shader.vert");
    coloredStaticShaderBuilder.addStage(ShaderStage::Fragment,
                                        "shaders/colored/shader.frag");
    auto coloredStaticShader = coloredStaticShaderBuilder.build();

    ShaderBuilder coloredDynamicShaderBuilder{};
    coloredDynamicShaderBuilder.addStage(ShaderStage::Vertex,
                                         "shaders/dynamic/colored/shader.vert");
    coloredDynamicShaderBuilder.addStage(ShaderStage::Fragment,
                                         "shaders/dynamic/colored/shader.frag");
    auto coloredDynamicShader = coloredDynamicShaderBuilder.build();

    auto coloredDrawPackBuilder = coloredMeshPack.createDrawPack(emptyMaterialPack);
    auto coloredStaticStage = Stage(std::move(coloredDrawPackBuilder));
    coloredStaticStage.setShader(coloredStaticShader);

    DynamicDrawPack<ColoredVertex, EmptyMaterial> dynamicColoredDrawPack(
        coloredMeshPack, emptyMaterialPack);
    auto dynamicColoredQueue = dynamicColoredDrawPack.getDrawQueue();
    auto dynamicColoredStage = DynamicStage(std::move(dynamicColoredDrawPack));
    dynamicColoredStage.setShader(coloredDynamicShader);

    auto pipeline =
        Pipeline(std::move(coloredStaticStage), std::move(unlitStaticStage),
                 std::move(dynamicColoredStage), std::move(dynamicUnlitStage));

    float smoothSpeed = 5.0f;
    glm::vec3 cameraPos = glm::vec3(0.0f, 8.0f, 20.0f);
    glm::vec3 cameraOffset = glm::vec3(0.0f, 8.0f, 20.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
    CameraMatrices cameraMatrices{};
    cameraMatrices.view = glm::lookAt(cameraPos, cameraOffset, worldUp);
    cameraMatrices.projection =
        glm::perspective(glm::radians(45.0f), SCREEN_WIDTH / SCREEN_HEIGHT, 1e-1f, 1e3f);


    DrawCommandBuilder unlitCommandbuilder(unlitMeshPack, unlitMaterialPack);
    auto cubeUnlitPartial_1 =
        unlitCommandbuilder.getCommandPartial(unlitCubeMesh, unlitMaterial_1);

    auto hexGrassPartial = unlitCommandbuilder.getCommandPartial(
        grassMeshes[0], grassMaterials[0]);

    auto barrelPartial = unlitCommandbuilder.getCommandPartial(
        barrelMeshes[0], barrelMaterials[0]);

    auto mountainPartial = unlitCommandbuilder.getCommandPartial(
        mountainMeshes[0], mountainMaterials[0]);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    ECS ecs(RenderingQueues{std::move(dynamicUnlitQueue),
                            std::move(dynamicColoredQueue)});

    EntityID player = ecs.createEntity();

    ecs.addComponent(player, PositionComponent{0.f, 0.f, 0.f});
    ecs.addComponent(player, MovableComponent(14.f, 5.f));
    ecs.addComponent(player, HitBoxComponent(0.5f));
    ecs.addComponent(player, CollidingComponent{});
    ecs.addComponent(player, PlayerMovementComponent{});
    ecs.addComponent(player, RenderableComponent{cubeUnlitPartial_1});

    EntityID coin = ecs.createEntity();
    ecs.addComponent(coin, PositionComponent{0.f, 10.f, -0.5f});
    ecs.addComponent(coin, HitBoxComponent{0.3f});
    ecs.addComponent(coin, RenderableComponent{barrelPartial, glm::vec3(2.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)});
    ecs.addComponent(coin, CoinComponent{6});

    constexpr size_t N = 5;
    for (int q = -static_cast<int>(N); q <= static_cast<int>(N); q++) {
        int r1 = std::max(-static_cast<int>(N), -q - static_cast<int>(N));
        int r2 = std::min(static_cast<int>(N), -q + static_cast<int>(N));
        for (int r = r1; r <= r2; r++) {
            int s = -q - r;

            float x = 2.0f * 2.0f * q + 2.0f * r;
            float y = (2.3094f + 1.0f) * r;

            int dist = std::max({std::abs(q), std::abs(r), std::abs(s)});
            bool isOuter = (dist == static_cast<int>(N));

            EntityID floor = ecs.createEntity();

            ecs.addComponent(floor, PositionComponent{
                x,
                -y,
                -0.5f
            });

            if (isOuter) {
                ecs.addComponent(floor, HitBoxComponent{2.5f});
                ecs.addComponent(floor, CollidingComponent{});
                ecs.addComponent(floor, RenderableComponent{
                    mountainPartial,
                    glm::vec3(2.0f),
                    glm::radians(90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)
                });

                auto floorUnder = ecs.createEntity();
                ecs.addComponent(floorUnder, PositionComponent{
                    x,
                    -y,
                    -0.5f
                });
                ecs.addComponent(floorUnder, RenderableComponent{
                    hexGrassPartial,
                    glm::vec3(2.0f),
                    glm::radians(90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)
                });
            }
            else {
                ecs.addComponent(floor, RenderableComponent{
                    hexGrassPartial,
                    glm::vec3(2.0f),
                    glm::radians(90.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)
                });
            }
        }
    }

    ecs.nextStage(ECS::StageType::Sequential)
        .addSystem(collidingSystem)
        .addSystem(collisionResolutionSystem)
        .addSystem(playerMovementSystem)
        .addSystem(movementSystem)
        .addSystem(renderingSystem)
        .addSystem(debugSystem)
        .addSystem(removeEntitySystem);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ecs.update(deltaTime);

        updateImGui(window, ecs.entityStorage.getNumberOfEntities(), deltaTime);

        if (gInputHandler.isPressed(Key::Num_1)) cameraOffset.z += 10 * deltaTime;
        if (gInputHandler.isPressed(Key::Num_2)) cameraOffset.z -= 10 * deltaTime;

        gInputHandler.update();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto position = ecs.getComponent<PositionComponent>(player);
        auto [x, y, z] = *position;
        glm::vec3 targetCameraPos = glm::vec3(x, y, z) + cameraOffset;
        cameraPos = glm::mix(cameraPos, targetCameraPos, 1.0f - expf(-smoothSpeed * deltaTime));
        glm::vec3 lookTarget = cameraPos - cameraOffset;
        cameraMatrices.view = glm::lookAt(cameraPos, lookTarget, worldUp);

        pipeline.execute(cameraMatrices);
        renderImGui();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroyImGui();
    destroyVertexArrays();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
