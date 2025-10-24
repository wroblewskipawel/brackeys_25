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
#include "graphics/resources/gl/buffer/std140.h"
#include "graphics/resources/gl/buffer/stream.h"
#include "graphics/resources/gl/buffer/stream/list.h"
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
#include "graphics/window.h"
#include "graphics/window/imgui/widget.h"

using MaterialList = TypeList<EmptyMaterial, UnlitMaterial>;
using MeshesList = TypeList<ColoredVertex, UnlitVertex, UnlitAnimatedVertex>;
using InstancesList = TypeList<glm::vec3, glm::vec4, glm::mat4>;
using StorageList = TypeList<glm::mat4>;

int main(void) {
    Window window{};

    auto widgets = WidgetListBuilder<>{}.append(FpsDisplay(0.98f)).build();

    auto instanceStreamList = StreamListBuilder<>{}
                                  .append(StreamBufferConfig<glm::mat4>{
                                      .pageSize = 512,
                                  })
                                  .append(StreamBufferConfig<glm::vec4>{
                                      .pageSize = 512,
                                  })
                                  .append(StreamBufferConfig<glm::vec3>{
                                      .pageSize = 512,
                                  })
                                  .build();

    auto storageStreamList = StreamListBuilder<>{}
                                 .append(StreamBufferConfig<glm::mat4>{
                                     .pageSize = 512,
                                 })
                                 .build();

    auto renderer =
        Renderer<MeshesList, MaterialList, InstancesList, StorageList>(
            std::move(instanceStreamList), std::move(storageStreamList));

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
    auto coloredCube = resourceBundle.getModel<ColoredVertex, EmptyMaterial>(
        "cubes", "coloredCube");

    ShaderBuilder unlitShaderBuilder{};
    unlitShaderBuilder.addStage(ShaderStage::Vertex,
                                "shaders/unlit/shader.vert");
    unlitShaderBuilder.addStage(ShaderStage::Fragment,
                                "shaders/unlit/shader.frag");
    auto unlitShader = unlitShaderBuilder.build();

    auto unliStaticStage = StaticStage<UnlitVertex, UnlitMaterial, glm::mat4>();
    unliStaticStage.setShader(unlitShader);

    ShaderBuilder unlitAnimatedShaderBuilder{};
    unlitAnimatedShaderBuilder.addStage(ShaderStage::Vertex,
                                        "shaders/unlit_animated/shader.vert");
    unlitAnimatedShaderBuilder.addStage(ShaderStage::Fragment,
                                        "shaders/unlit_animated/shader.frag");
    auto unlitAnimatedShader = unlitAnimatedShaderBuilder.build();

    auto unlitAnimatedStage =
        AnimatedStage<UnlitAnimatedVertex, UnlitMaterial, glm::mat4>(
            instanceStreamHandle, jointMatrixStreamHandle);
    unlitAnimatedStage.setShader(unlitAnimatedShader);

    auto unlitDynamicStage =
        DynamicStage<UnlitVertex, UnlitMaterial, glm::mat4>(
            instanceStreamHandle);
    unlitDynamicStage.setShader(unlitShader);

    ShaderBuilder coloredShaderBuilder{};
    coloredShaderBuilder.addStage(ShaderStage::Vertex,
                                  "shaders/colored/shader.vert");
    coloredShaderBuilder.addStage(ShaderStage::Fragment,
                                  "shaders/colored/shader.frag");
    auto coloredShader = coloredShaderBuilder.build();

    auto coloredStaticStage =
        StaticStage<ColoredVertex, EmptyMaterial, glm::mat4>();
    coloredStaticStage.setShader(coloredShader);

    auto pipeline =
        Pipeline(std::move(coloredStaticStage), std::move(unliStaticStage),
                 std::move(unlitDynamicStage), std::move(unlitAnimatedStage));

    CameraMatrices cameraMatrices{};
    cameraMatrices.view =
        glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    cameraMatrices.projection =
        glm::perspective(glm::radians(45.0f), 480.0f / 640.0f, 1e-1f, 1e3f);

    auto animations =
        resourceBundle.getModelAnimations<UnlitAnimatedVertex, UnlitMaterial>(
            "gltf", "Cesium_Man");
    auto animationPlayer_1 = AnimationPlayer(animations[0]);
    animationPlayer_1.loopAnimation(true);
    auto animationPlayer_2 = AnimationPlayer(animations[0]);
    animationPlayer_2.loopAnimation(true);
    auto animationPlayer_3 = AnimationPlayer(animations[0]);
    animationPlayer_3.loopAnimation(true);

    auto unlitStaticBatch =
        resourceBundle
            .getStaticBatchBuilder<UnlitVertex, UnlitMaterial, glm::mat4>()
            .addDraw(unlitCube_1, glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(2.0f, 0.0f, 2.0f)))
            .addDraw(unlitCube_2, glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(0.0f, 0.0f, -2.0f)))
            .build();

    auto coloredStaticBatch =
        resourceBundle
            .getStaticBatchBuilder<ColoredVertex, EmptyMaterial, glm::mat4>()
            .addDraw(coloredCube, glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(2.0f, 0.0f, -2.0f)))
            .addDraw(coloredCube, glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(-2.0f, 0.0f, 2.0f)))
            .addDraw(coloredCube, glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(-2.0f, 0.0f, -2.0f)))
            .build();

    std::chrono::steady_clock clock{};
    auto lastFrameTime = clock.now();

    float accumulatedTime = 0.0f;

    while (!window.shouldClose()) {
        auto frame = Frame(window);

        auto currentFrameTime = clock.now();
        auto deltaTime =
            std::chrono::duration<float>(currentFrameTime - lastFrameTime)
                .count();
        lastFrameTime = currentFrameTime;
        accumulatedTime += deltaTime;

        auto& instanceStream = instanceStreamHandle.get().get();
        auto& jointStream = jointMatrixStreamHandle.get().get();

        auto& unlitColoredStage = pipeline.getStage<
            StaticStage<ColoredVertex, EmptyMaterial, glm::mat4>>();
        auto& unlitStaticStage =
            pipeline
                .getStage<StaticStage<UnlitVertex, UnlitMaterial, glm::mat4>>();
        auto& dynamicStage = pipeline.getStage<
            DynamicStage<UnlitVertex, UnlitMaterial, glm::mat4>>();
        auto& animatedStage = pipeline.getStage<
            AnimatedStage<UnlitAnimatedVertex, UnlitMaterial, glm::mat4>>();

        unlitColoredStage.addDraw(coloredStaticBatch);
        unlitStaticStage.addDraw(unlitStaticBatch);

        animationPlayer_1.update(deltaTime);
        animationPlayer_2.update(deltaTime / 2.0f);
        animationPlayer_3.update(deltaTime / 4.0f);

        instanceStream.beginGeneration();
        jointStream.beginGeneration();

        dynamicStage.addDraw(
            waterBottle,
            glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(-2.0f, 0.0f, 0.0f)),
                                   5.0f * 3.14f * accumulatedTime,
                                   glm::vec3(0.0f, 0.0f, 1.0f)),
                       glm::vec3(6.0f)));

        dynamicStage.addDraw(
            waterBottle,
            glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(2.0f, 0.0f, 0.0f)),
                                   -5.0f * 3.14f * accumulatedTime,
                                   glm::vec3(0.0f, 0.0f, 1.0f)),
                       glm::vec3(6.0f)));

        animatedStage.addDraw(
            cesiumMan,
            glm::scale(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
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
            glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(0.0f, 0.0f, 2.5f)),
                                   5.0f * 3.14f * accumulatedTime,
                                   glm::vec3(0.0f, 1.0f, 0.0f)),
                       glm::vec3(4.0f)));

        animatedStage.addDraw(
            cesiumMan,
            glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f),
                                                  glm::vec3(0.0f, 2.0f, -1.0f)),
                                   3.15f / 2.0f * accumulatedTime,
                                   glm::vec3(0.0f, 0.0f, 1.0f)),
                       glm::vec3(2.0f)),
            animationPlayer_3);

        instanceStream.endGeneration();
        jointStream.endGeneration();

        pipeline.execute(cameraMatrices);

        widgets.update(deltaTime);
        frame.draw(widgets);

        unlitColoredStage.clear();
        unlitStaticStage.clear();
        dynamicStage.clear();
        animatedStage.clear();
    }
    return 0;
}
