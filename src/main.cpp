#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

#include "debug.h"
#include "gltf.h"
#include "material.h"
#include "mesh.h"
#include "model.h"
#include "renderer.h"
#include "shader.h"
#include "std140.h"

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

    ShaderBuilder unlitShaderBuilder{};
    unlitShaderBuilder.addStage(ShaderStage::Vertex,
                                "shaders/unlit/shader.vert");
    unlitShaderBuilder.addStage(ShaderStage::Fragment,
                                "shaders/unlit/shader.frag");
    auto unlitShader = unlitShaderBuilder.build();

    auto unlitDrawPack = unlitMeshPack.createDrawPack(unlitMaterialPack);
    unlitDrawPack.addDraw(unlitCubeMesh, unlitMaterial_1, glm::mat4(1.0f));
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
    auto coloredStage = Stage(std::move(coloredDrawPackBuilder));
    coloredStage.setShader(coloredShader);

    auto pipeline = Pipeline(std::move(coloredStage), std::move(unlitStage));

    CameraMatrices cameraMatrices{};
    cameraMatrices.view =
        glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    cameraMatrices.projection =
        glm::perspective(glm::radians(45.0f), 480.0f / 640.0f, 1e-1f, 1e3f);

    while (!glfwWindowShouldClose(window)) {
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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    destroyVertexArrays();

    glfwTerminate();
    return 0;
}