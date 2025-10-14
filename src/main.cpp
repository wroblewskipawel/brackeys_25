#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

#include "collections/unique_list.h"
#include "graphics/assets/bundle.h"
#include "graphics/assets/gltf.h"
#include "graphics/assets/model.h"
#include "graphics/debug.h"
#include "graphics/renderer.h"
#include "graphics/resources/animation.h"
#include "graphics/resources/gl/buffer/binding.h"
#include "graphics/resources/gl/buffer/ring.h"
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/bundle.h"
#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/draw/dynamic.h"
#include "graphics/resources/gl/draw/static.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/resources/material.h"
#include "graphics/resources/mesh.h"
#include "graphics/storage/gl/stream.h"

using MaterialList = TypeList<EmptyMaterial, UnlitMaterial>;
using MeshesList = TypeList<ColoredVertex, UnlitVertex, UnlitAnimatedVertex>;

int main(void) {
    GLFWwindow* window;
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    {
        auto instanceStreamHandle =
            registerStreamBuffer(StreamBuffer<glm::mat4>(256));
        auto jointMatrixStreamHandle =
            registerStreamBuffer(StreamBuffer<glm::mat4>(512));

        MaterialBuilder<UnlitMaterial> unlitMaterialBuilder_1{};
        unlitMaterialBuilder_1.setAlbedoTextureData(TextureData::loadFromFile(
            "assets/textures/tile_1.png", TextureFormat::RGB));

        MaterialBuilder<UnlitMaterial> unlitMaterialBuilder_2{};
        unlitMaterialBuilder_2.setAlbedoTextureData(TextureData::loadFromFile(
            "assets/textures/tile_2.png", TextureFormat::RGB));

        auto unlitMaterialBuilder_1Handle =
            registerMaterialBuilder(std::move(unlitMaterialBuilder_1));
        auto unlitMaterialBuilder_2Handle =
            registerMaterialBuilder(std::move(unlitMaterialBuilder_2));

        auto cubeMeshUnlit = getCubeMesh<UnlitVertex>();
        auto cubeMeshColored = getCubeMesh<ColoredVertex>();

        auto unlitCubeModel_1 = ModelDataBuilder<UnlitVertex, UnlitMaterial>{}
                                    .withMesh(cubeMeshUnlit)
                                    .withMaterial(unlitMaterialBuilder_1Handle)
                                    .withName("unlitCube_1")
                                    .build();
        auto unlitCubeModel_2 = ModelDataBuilder<UnlitVertex, UnlitMaterial>{}
                                    .withMesh(cubeMeshUnlit)
                                    .withMaterial(unlitMaterialBuilder_2Handle)
                                    .withName("unlitCube_2")
                                    .build();
        auto coloredCubeModel = ModelDataBuilder<ColoredVertex, EmptyMaterial>{}
                                    .withMesh(cubeMeshColored)
                                    .withName("coloredCube")
                                    .build();

        auto assetsBundle = AssetsBundle<MeshesList, MaterialList>();

        assetsBundle
            .pushDocument<UnlitVertex, UnlitMaterial>(
                "gltf", "assets/WaterBottle/glTF/WaterBottle.gltf")
            .pushDocument<UnlitAnimatedVertex, UnlitMaterial>(
                "gltf", "assets/CesiumMan/glTF/CesiumMan.gltf")
            .pushModel("cubes", unlitCubeModel_1)
            .pushModel("cubes", unlitCubeModel_2)
            .pushModel("cubes", coloredCubeModel);

        auto resourceBundle = ResourceBundle(assetsBundle);

        auto waterBottle = resourceBundle.getModel<UnlitVertex, UnlitMaterial>(
            "gltf", "WaterBottle");
        auto cesiumMan =
            resourceBundle.getModel<UnlitAnimatedVertex, UnlitMaterial>(
                "gltf", "Cesium_Man");
        auto unlitCube_1 = resourceBundle.getModel<UnlitVertex, UnlitMaterial>(
            "cubes", "unlitCube_1");
        auto unlitCube_2 = resourceBundle.getModel<UnlitVertex, UnlitMaterial>(
            "cubes", "unlitCube_2");
        auto coloredCube =
            resourceBundle.getModel<ColoredVertex, EmptyMaterial>(
                "cubes", "coloredCube");

        ShaderBuilder unlitShaderBuilder{};
        unlitShaderBuilder.addStage(ShaderStage::Vertex,
                                    "shaders/unlit/shader.vert");
        unlitShaderBuilder.addStage(ShaderStage::Fragment,
                                    "shaders/unlit/shader.frag");
        auto unlitShader = unlitShaderBuilder.build();

        auto unlitStaticPack =
            resourceBundle
                .getStaticPackBuilder<UnlitVertex, UnlitMaterial, glm::mat4>();
        unlitStaticPack.addDraw(
            unlitCube_1,
            glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 2.0f)));
        unlitStaticPack.addDraw(
            unlitCube_2,
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)));

        auto unlitStage = StaticStage(std::move(unlitStaticPack));
        unlitStage.setShader(unlitShader);

        ShaderBuilder unlitAnimatedShaderBuilder{};
        unlitAnimatedShaderBuilder.addStage(
            ShaderStage::Vertex, "shaders/unlit_animated/shader.vert");
        unlitAnimatedShaderBuilder.addStage(
            ShaderStage::Fragment, "shaders/unlit_animated/shader.frag");
        auto unlitAnimatedShader = unlitAnimatedShaderBuilder.build();

        auto unlitAnimatedStage = AnimatedStage(
            resourceBundle.getAnimatedPackBuilder<UnlitAnimatedVertex,
                                                  UnlitMaterial, glm::mat4>(
                instanceStreamHandle, jointMatrixStreamHandle));
        unlitAnimatedStage.setShader(unlitAnimatedShader);

        auto unlitDynamicStage = DynamicStage(
            resourceBundle.getDynamicPackBuilder<UnlitVertex, UnlitMaterial>(
                instanceStreamHandle));
        unlitDynamicStage.setShader(unlitShader);

        ShaderBuilder coloredShaderBuilder{};
        coloredShaderBuilder.addStage(ShaderStage::Vertex,
                                      "shaders/colored/shader.vert");
        coloredShaderBuilder.addStage(ShaderStage::Fragment,
                                      "shaders/colored/shader.frag");
        auto coloredShader = coloredShaderBuilder.build();

        auto coloredStaticPackBuilder =
            resourceBundle.getStaticPackBuilder<ColoredVertex, EmptyMaterial,
                                                glm::mat4>();
        coloredStaticPackBuilder.addDraw(
            coloredCube,
            glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -2.0f)));
        coloredStaticPackBuilder.addDraw(
            coloredCube,
            glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 2.0f)));
        coloredStaticPackBuilder.addDraw(
            coloredCube,
            glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, -2.0f)));
        auto coloredStage = StaticStage(std::move(coloredStaticPackBuilder));
        coloredStage.setShader(coloredShader);

        auto pipeline = Pipeline(std::move(coloredStage), std::move(unlitStage),
                                 std::move(unlitDynamicStage),
                                 std::move(unlitAnimatedStage));

        CameraMatrices cameraMatrices{};
        cameraMatrices.view =
            glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f),
                        glm::vec3(0.0f, 0.0f, 1.0f));
        cameraMatrices.projection =
            glm::perspective(glm::radians(45.0f), 480.0f / 640.0f, 1e-1f, 1e3f);

        auto animations =
            resourceBundle
                .getModelAnimations<UnlitAnimatedVertex, UnlitMaterial>(
                    "gltf", "Cesium_Man");
        auto animationPlayer_1 = AnimationPlayer(animations[0]);
        animationPlayer_1.loopAnimation(true);
        auto animationPlayer_2 = AnimationPlayer(animations[0]);
        animationPlayer_2.loopAnimation(true);
        auto animationPlayer_3 = AnimationPlayer(animations[0]);
        animationPlayer_3.loopAnimation(true);

        std::chrono::steady_clock clock{};
        auto lastFrameTime = clock.now();

        float accumulatedTime = 0.0f;

        while (!glfwWindowShouldClose(window)) {
            auto currentFrameTime = clock.now();
            auto deltaTime =
                std::chrono::duration<float>(currentFrameTime - lastFrameTime)
                    .count();
            lastFrameTime = currentFrameTime;
            accumulatedTime += deltaTime;

            auto& instanceStream = instanceStreamHandle.get().get();
            auto& jointStream = jointMatrixStreamHandle.get().get();

            auto& dynamicStage = pipeline.getStage<
                DynamicStage<UnlitVertex, UnlitMaterial, glm::mat4>>();
            auto& animatedStage = pipeline.getStage<AnimatedStage<
                UnlitAnimatedVertex, UnlitMaterial, glm::mat4>>();

            animationPlayer_1.update(deltaTime);
            animationPlayer_2.update(deltaTime / 2.0f);
            animationPlayer_3.update(deltaTime / 4.0f);

            instanceStream.beginGeneration();
            jointStream.beginGeneration();

            dynamicStage.addDraw(
                waterBottle,
                glm::scale(
                    glm::rotate(glm::translate(glm::mat4(1.0f),
                                               glm::vec3(-2.0f, 0.0f, 0.0f)),
                                5.0f * 3.14f * accumulatedTime,
                                glm::vec3(0.0f, 0.0f, 1.0f)),
                    glm::vec3(6.0f)));

            dynamicStage.addDraw(
                waterBottle,
                glm::scale(
                    glm::rotate(glm::translate(glm::mat4(1.0f),
                                               glm::vec3(2.0f, 0.0f, 0.0f)),
                                -5.0f * 3.14f * accumulatedTime,
                                glm::vec3(0.0f, 0.0f, 1.0f)),
                    glm::vec3(6.0f)));

            animatedStage.addDraw(
                cesiumMan,
                glm::scale(glm::translate(glm::mat4(1.0f),
                                          glm::vec3(0.0f, 0.0f, -1.0f)),
                           glm::vec3(2.0f)),
                animationPlayer_1);

            animatedStage.addDraw(
                cesiumMan,
                glm::scale(
                    glm::rotate(glm::translate(glm::mat4(1.0f),
                                               glm::vec3(0.0f, -2.0f, -1.0f)),
                                -3.15f / 2.0f * accumulatedTime,
                                glm::vec3(0.0f, 0.0f, 1.0f)),
                    glm::vec3(2.0f)),
                animationPlayer_2);

            dynamicStage.addDraw(
                waterBottle,
                glm::scale(
                    glm::rotate(glm::translate(glm::mat4(1.0f),
                                               glm::vec3(0.0f, 0.0f, 2.5f)),
                                5.0f * 3.14f * accumulatedTime,
                                glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::vec3(4.0f)));

            animatedStage.addDraw(
                cesiumMan,
                glm::scale(
                    glm::rotate(glm::translate(glm::mat4(1.0f),
                                               glm::vec3(0.0f, 2.0f, -1.0f)),
                                3.15f / 2.0f * accumulatedTime,
                                glm::vec3(0.0f, 0.0f, 1.0f)),
                    glm::vec3(2.0f)),
                animationPlayer_3);

            instanceStream.endGeneration();
            jointStream.endGeneration();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("IDK");
            ImGui::Text("Hello World!");

            // Smoothed FPS
            static float smoothedFPS = 0.0f;
            float currentFPS = 1.0f / deltaTime;
            const float smoothing =
                0.98f;  // closer to 1 = smoother, slower updates
            smoothedFPS =
                smoothing * smoothedFPS + (1.0f - smoothing) * currentFPS;

            ImGui::Text("FPS: %.1f", smoothedFPS);
            ImGui::End();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            pipeline.execute(cameraMatrices);

            dynamicStage.clear();
            animatedStage.clear();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    VertexArrayStorage::destroyVertexArrays();

    glfwTerminate();
    return 0;
}
