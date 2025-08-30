#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <iostream>

#include "ImGui/ImGui.hpp"
#include "InputHandler/InputHandler.hpp"
#include "MusicManager/MusicManager.hpp"
#include "gltf.h"
#include "mesh.h"
#include "model.h"
#include "renderer.h"
#include "shader.h"

#include "EntityComponentSystem/ECS.hpp"
#include "EntityComponentSystem/Components/MovableComponent.hpp"
#include "EntityComponentSystem/Systems/MovementSystem.hpp"


void debugSystem(ECS& ecs, const float& deltaTime) {
    auto& movables = ecs.getEntitiesWithComponent<MovableComponent>().get();
    std::cout << "Debug: " << movables.size() << " movables tracked.\n";
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

    glfwSetKeyCallback(window, keyCallback);
    setupImGui(window);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    ECS ecs;

    EntityID player = ecs.createEntity();
    EntityID enemy  = ecs.createEntity();

    ecs.addComponent(player, MovableComponent{0,0,1.0f,1.0f});
    ecs.addComponent(enemy, MovableComponent{5,5,1.0f,1.0f});

    ecs.nextStage(ECS::StageType::Parallel)
          .addSystem(movementSystem)
       .nextStage(ECS::StageType::Sequential)
          .addSystem(debugSystem);

    DocumentReader<UnlitVertex> unlitDocument{
        "assets/WaterBottle/glTF/WaterBottle.gltf"};

    MeshPackBuilder<UnlitVertex> unlitMeshPackBuilder{};
    auto unlitCubeMesh =
        unlitMeshPackBuilder.addMesh(createCube<UnlitVertex>());
    auto documentMeshes =
        unlitMeshPackBuilder.addMeshMulti(unlitDocument.takeMeshes());
    auto unlitMeshPack = unlitMeshPackBuilder.build();

    ShaderBuilder unlitShaderBuilder{};
    unlitShaderBuilder.addStage(ShaderStage::Vertex,
                                "shaders/unlit/shader.vert");
    unlitShaderBuilder.addStage(ShaderStage::Fragment,
                                "shaders/unlit/shader.frag");
    auto unlitShader = unlitShaderBuilder.build();

    auto unlitDrawPack = unlitMeshPack.createDrawPack();
    unlitDrawPack.addDraw(unlitCubeMesh, glm::mat4(1.0f));
    unlitDrawPack.addDraw(
        documentMeshes[0],
        glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)),
            glm::vec3(3.0f)));
    unlitDrawPack.addDraw(
        documentMeshes[0],
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)),
                   glm::vec3(3.0f)));
    ;
    auto unlitStage = Stage(std::move(unlitDrawPack));
    unlitStage.setShader(unlitShader);

    MeshPackBuilder<ColoredVertex> coloredMeshPackBuilder{};
    auto coloredCubeMesh =
        coloredMeshPackBuilder.addMesh(createCube<ColoredVertex>());
    auto coloredMeshPack = coloredMeshPackBuilder.build();

    ShaderBuilder coloredShaderBuilder{};
    coloredShaderBuilder.addStage(ShaderStage::Vertex,
                                  "shaders/colored/shader.vert");
    coloredShaderBuilder.addStage(ShaderStage::Fragment,
                                  "shaders/colored/shader.frag");
    auto coloredShader = coloredShaderBuilder.build();

    auto coloredDrawPackBuilder = coloredMeshPack.createDrawPack();
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -2.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -2.0f, -2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, -2.0f, 2.0f)));
    coloredDrawPackBuilder.addDraw(
        coloredCubeMesh,
        glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, -2.0f, -2.0f)));
    auto coloredStage = Stage(std::move(coloredDrawPackBuilder));
    coloredStage.setShader(coloredShader);

    auto pipeline = Pipeline(std::move(coloredStage), std::move(unlitStage));

    CameraMatrices cameraMatrices{};
    cameraMatrices.view =
        glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    cameraMatrices.projection =
        glm::perspective(glm::radians(45.0f), 480.0f / 640.0f, 1e-1f, 1e3f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClearDepth(1.0f);
    glDepthFunc(GL_LEQUAL);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ecs.update(deltaTime);

        updateImGui(window);

        if (gInputHandler.isClicked(Key::R)) gMusicManager.play(SoundID::Coin);
        if (gInputHandler.isPressed(Key::Space))
        gMusicManager.play(SoundID::Explosion);
        
        gInputHandler.update();
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
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
