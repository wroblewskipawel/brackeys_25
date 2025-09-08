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
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/bundle.h"
#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/resources/material.h"
#include "graphics/resources/mesh.h"

constexpr size_t jointMatrixBufferBinding = 1;

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
        auto documentBundle = DocumentBundle<MeshesList, MaterialList>();

        documentBundle.pushDocument<UnlitVertex, UnlitMaterial>(
            "assets/WaterBottle/glTF/WaterBottle.gltf");

        documentBundle.pushDocument<UnlitAnimatedVertex, UnlitMaterial>(
            "assets/CesiumMan/glTF/CesiumMan.gltf");

        auto resourceBundle = ResourceBundle(documentBundle);

        auto waterBottle = resourceBundle.getModel<UnlitVertex, UnlitMaterial>(
            "assets/WaterBottle/glTF/WaterBottle.gltf", "WaterBottle");
        auto cesiumMan =
            resourceBundle.getModel<UnlitAnimatedVertex, UnlitMaterial>(
                "assets/CesiumMan/glTF/CesiumMan.gltf", "Cesium_Man");

        ShaderBuilder unlitShaderBuilder{};
        unlitShaderBuilder.addStage(ShaderStage::Vertex,
                                    "shaders/unlit/shader.vert");
        unlitShaderBuilder.addStage(ShaderStage::Fragment,
                                    "shaders/unlit/shader.frag");
        auto unlitShader = unlitShaderBuilder.build();

        auto unlitDrawPack =
            resourceBundle
                .getDrawPackBuilder<UnlitVertex, UnlitMaterial, glm::mat4>();
        unlitDrawPack.addDraw(
            waterBottle,
            glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)),
                glm::vec3(6.0f)));

        auto unlitStage = Stage(std::move(unlitDrawPack));
        unlitStage.setShader(unlitShader);

        ShaderBuilder unlitAnimatedShaderBuilder{};
        unlitAnimatedShaderBuilder.addStage(
            ShaderStage::Vertex, "shaders/unlit_animated/shader.vert");
        unlitAnimatedShaderBuilder.addStage(
            ShaderStage::Fragment, "shaders/unlit_animated/shader.frag");
        auto unlitAnimatedShader = unlitAnimatedShaderBuilder.build();

        auto unlitAnimatedDrawPack =
            resourceBundle.getDrawPackBuilder<UnlitAnimatedVertex,
                                              UnlitMaterial, glm::mat4>();
        unlitAnimatedDrawPack.addDraw(
            cesiumMan, glm::scale(glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(0.0f, 0.0f, -1.0f)),
                                  glm::vec3(2.0f)));
        ;
        auto unlitAnimatedStage = Stage(std::move(unlitAnimatedDrawPack));
        unlitAnimatedStage.setShader(unlitAnimatedShader);

        auto pipeline =
            Pipeline(std::move(unlitStage), std::move(unlitAnimatedStage));

        CameraMatrices cameraMatrices{};
        cameraMatrices.view =
            glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f),
                        glm::vec3(0.0f, 0.0f, 1.0f));
        cameraMatrices.projection =
            glm::perspective(glm::radians(45.0f), 480.0f / 640.0f, 1e-1f, 1e3f);

        auto animations =
            resourceBundle
                .getModelAnimations<UnlitAnimatedVertex, UnlitMaterial>(
                    "assets/CesiumMan/glTF/CesiumMan.gltf", "Cesium_Man");
        auto animationPlayer = AnimationPlayer(animations[0]);
        animationPlayer.loopAnimation(true);

        auto jointMatrices = animationPlayer.getJointTransforms();

        auto jointMatrixBufferBuilder =
            std140::UniformArrayBuilder<glm::mat4>();
        jointMatrixBufferBuilder.pushMulti(jointMatrices);
        auto jointMatrixBuffer = jointMatrixBufferBuilder.build();

        jointMatrixBuffer.bind(GL_SHADER_STORAGE_BUFFER,
                               jointMatrixBufferBinding);

        std::chrono::steady_clock clock{};
        auto lastFrameTime = clock.now();

        while (!glfwWindowShouldClose(window)) {
            auto currentFrameTime = clock.now();
            auto deltaTime =
                std::chrono::duration<float>(currentFrameTime - lastFrameTime)
                    .count();
            lastFrameTime = currentFrameTime;

            animationPlayer.update(deltaTime);
            auto jointMatrices = animationPlayer.getJointTransforms();

            jointMatrixBuffer.updateRange(jointMatrices, 0);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("IDK");
            ImGui::Text("Hello World!");
            ImGui::End();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            pipeline.execute(cameraMatrices);

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
