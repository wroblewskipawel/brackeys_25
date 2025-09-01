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
#include "EntityComponentSystem/Systems/MovementSystem.hpp"
#include "EntityComponentSystem/Systems/PlayerMovementSystem.hpp"
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

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
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

    MaterialPackBuilder<EmptyMaterial> emptyMaterialPackBuilder{};
    auto emptyMaterial =
        emptyMaterialPackBuilder.addMaterial(MaterialBuilder<EmptyMaterial>{});
    auto emptyMaterialPack = emptyMaterialPackBuilder.build();

    MeshPackBuilder<UnlitVertex> unlitMeshPackBuilder{};
    auto unlitCubeMesh =
        unlitMeshPackBuilder.addMesh(createCube<UnlitVertex>());
    auto documentMeshes =
        unlitMeshPackBuilder.addMeshMulti(unlitDocument.takeMeshes());
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
    auto unlitMaterial_2 =
        unlitMaterialPackBuilder.addMaterial(unlitMaterialBuilder_2);
    auto documentMaterials = unlitMaterialPackBuilder.addMaterialMulti(
        unlitDocument.takeMaterials());
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
    // unlitDrawPack.addDraw(unlitCubeMesh, unlitMaterial_1, glm::mat4(1.0f));
    unlitDrawPack.addDraw(
        unlitCubeMesh, unlitMaterial_2,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f)));
    unlitDrawPack.addDraw(
        unlitCubeMesh, unlitMaterial_2,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)));
    unlitDrawPack.addDraw(
        documentMeshes[0], documentMaterials[0],
        glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)),
            glm::vec3(6.0f)));
    unlitDrawPack.addDraw(
        documentMeshes[0], documentMaterials[0],
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)),
                   glm::vec3(6.0f)));
    ;
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

    auto coloredDrawPackBuilder =
        coloredMeshPack.createDrawPack(emptyMaterialPack);
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -2.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -2.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, -2.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh, emptyMaterial,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, -2.0f, -2.0f)));
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
        glm::perspective(glm::radians(45.0f), 480.0f / 640.0f, 1e-1f, 1e3f);

    DrawCommandBuilder coloredCommandbuilder(coloredMeshPack,
                                             emptyMaterialPack);
    auto cubeColoredPartial =
        coloredCommandbuilder.getCommandPartial(coloredCubeMesh, emptyMaterial);

    DrawCommandBuilder unlitCommandbuilder(unlitMeshPack, unlitMaterialPack);
    auto cubeUnlitPartial_1 =
        unlitCommandbuilder.getCommandPartial(unlitCubeMesh, unlitMaterial_1);
    auto cubeUnlitPartial_2 =
        unlitCommandbuilder.getCommandPartial(unlitCubeMesh, unlitMaterial_2);

    auto documentUnlitPartial = unlitCommandbuilder.getCommandPartial(
        documentMeshes[0], documentMaterials[0]);

    auto cubePosition = glm::vec3(0.0);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    ECS ecs(RenderingQueues{std::move(dynamicUnlitQueue),
                            std::move(dynamicColoredQueue)});

    EntityID player = ecs.createEntity();

    ecs.addComponent(player, PositionComponent{0.f, 0.f, 0.f});
    ecs.addComponent(player, MovableComponent(7.f, 5.f));
    ecs.addComponent(player, CollidingComponent(0.5f));
    ecs.addComponent(player, PlayerMovementComponent{});
    ecs.addComponent(player, RenderableComponent{cubeUnlitPartial_1});

    for (size_t i = 0; i < 30; ++i)
    {
        for (size_t j = 0; j < 40; ++j)
        {
            EntityID ent = ecs.createEntity();
            ecs.addComponent(ent, PositionComponent{5.f + i, 6.f + j, 0.f});
            ecs.addComponent(ent, MovableComponent(1.f, 5.f));
            ecs.addComponent(ent, CollidingComponent(0.5f));
            ecs.addComponent(ent, RenderableComponent{cubeUnlitPartial_1});
        }
    }

    for (size_t i = 0; i < 10; ++i) {
        EntityID wall = ecs.createEntity();
        ecs.addComponent(wall, PositionComponent{2.5f + i, 5.f, 0.f});
        ecs.addComponent(wall, CollidingComponent(0.5f));
        ecs.addComponent(wall, RenderableComponent{cubeColoredPartial});
    }

    ecs.nextStage(ECS::StageType::Sequential)
        .addSystem(collidingSystem)
        .addSystem(playerMovementSystem)
        .addSystem(movementSystem)
        .addSystem(renderingSystem)
        .addSystem(debugSystem);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ecs.update(deltaTime);

        // auto* playerMovement = ecs.getComponent<MovableComponent>(player);
        // cubePosition.x = playerMovement->x;
        // cubePosition.y = playerMovement->y;

        // dynamicUnlitQueue->emplace_back(cubeUnlitPartial_1.withTransform(
        //     glm::translate(glm::mat4(1.0), cubePosition)));
        // dynamicUnlitQueue->emplace_back(cubeUnlitPartial_2.withTransform(
        //     glm::translate(glm::mat4(1.0), glm::vec3(-3.0, -3.0, -3.0))));
        // dynamicUnlitQueue->emplace_back(documentUnlitPartial.withTransform(
        //     glm::translate(glm::mat4(1.0), glm::vec3(-3.0, -5.0, -3.0))));

        updateImGui(window, ecs.entityStorage.getNumberOfEntities(), deltaTime);

        if (gInputHandler.isClicked(Key::R)) gMusicManager.play(SoundID::Coin);
        if (gInputHandler.isPressed(Key::Space))
            gMusicManager.play(SoundID::Explosion);

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
