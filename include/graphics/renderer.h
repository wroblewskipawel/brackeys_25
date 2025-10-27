#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <vector>

#include "collections/unique_list.h"
#include "concepts/range.h"
#include "graphics/resources/gl/buffer/stream/list.h"
#include "graphics/resources/gl/draw/animated.h"
#include "graphics/resources/gl/draw/dynamic.h"
#include "graphics/resources/gl/draw/static.h"
#include "graphics/resources/gl/draw/static/batch.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"

template <typename...>
class Pipeline;

template <typename Vertex, typename Material, typename Instance>
class StaticStage {
   public:
    using StaticBatch = StaticBatch<Vertex, Material, Instance>;
    using StaticBatchHandle = typename StaticBatch::Handle;

    StaticStage() = default;

    template <typename... InstanceTypes, typename... StorageTypes>
    StaticStage(const StreamList<InstanceTypes...>& instanceStreams,
                const StreamList<StorageTypes...>& storageStreams)
        : StaticStage() {}

    StaticStage& setShader(const Shader& shader) {
        shaderProgram = shader.program;
        return *this;
    }

    StaticStage& clear() noexcept {
        staticDrawMap.clear();
        return *this;
    }

    StaticStage& addDraw(const StaticBatchHandle& packHandle,
                         const glm::mat4& instanceOffset = glm::mat4(1.0f)) {
        staticDrawMap.addDraw(packHandle, instanceOffset);
        return *this;
    }

   private:
    template <typename...>
    friend class Pipeline;

    void execute(const CameraMatrices& cameraMatrices) {
        if (shaderProgram) {
            glUseProgram(shaderProgram);
            const auto& locations =
                Shader::getProgramUniformLocations(shaderProgram);
            glUniformMatrix4fv(locations.viewMatrix, 1, GL_FALSE,
                               glm::value_ptr(cameraMatrices.view));
            glUniformMatrix4fv(locations.projectionMatrix, 1, GL_FALSE,
                               glm::value_ptr(cameraMatrices.projection));
            staticDrawMap.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    StaticDrawMap<Vertex, Material, Instance> staticDrawMap;
};

template <typename Vertex, typename Material, typename Instance>
class DynamicStage {
   public:
    using Model = Model<Vertex, Material>;

    DynamicStage(const StreamHandle<Instance>& streamBuffer)
        : dynamicDrawMap(streamBuffer.copy()) {}

    template <typename... InstanceTypes, typename... StorageTypes>
    DynamicStage(const StreamList<InstanceTypes...>& instanceStreams,
                 const StreamList<StorageTypes...>& storageStreams)
        : DynamicStage(instanceStreams.template getStreamHandle<Instance>()) {}

    DynamicStage& setShader(const Shader& shader) {
        shaderProgram = shader.program;
        return *this;
    }

    DynamicStage& clear() noexcept {
        dynamicDrawMap.clear();
        return *this;
    }

    DynamicStage& addDraw(const Model& model, const Instance& instance) {
        dynamicDrawMap.addDraw(model, instance);
        return *this;
    }

    template <typename Instances>
        requires RefConstRange<Instances, Instance>
    DynamicStage& addDraw(const Model& model, Instances&& range) {
        dynamicDrawMap.addDraw(model, std::forward<Instances>(range));
        return *this;
    }

   private:
    template <typename...>
    friend class Pipeline;

    void execute(const CameraMatrices& cameraMatrices) {
        if (shaderProgram) {
            glUseProgram(shaderProgram);
            const auto& locations =
                Shader::getProgramUniformLocations(shaderProgram);
            glUniformMatrix4fv(locations.viewMatrix, 1, GL_FALSE,
                               glm::value_ptr(cameraMatrices.view));
            glUniformMatrix4fv(locations.projectionMatrix, 1, GL_FALSE,
                               glm::value_ptr(cameraMatrices.projection));
            dynamicDrawMap.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    DynamicDrawMap<Vertex, Material, Instance> dynamicDrawMap;
};

template <typename Vertex, typename Material, typename Instance>
class AnimatedStage {
   public:
    using Model = Model<Vertex, Material>;

    AnimatedStage(const StreamHandle<Instance>& instanceStreamBuffer,
                  const StreamHandle<glm::mat4>& jointStreamBuffer)
        : animatedDrawMap(instanceStreamBuffer, jointStreamBuffer) {}

    template <typename... InstanceTypes, typename... StorageTypes>
    AnimatedStage(const StreamList<InstanceTypes...>& instanceStreams,
                  const StreamList<StorageTypes...>& storageStreams)
        : AnimatedStage(instanceStreams.template getStreamHandle<Instance>(),
                        storageStreams.template getStreamHandle<glm::mat4>()) {}

    AnimatedStage& setShader(const Shader& shader) {
        shaderProgram = shader.program;
        return *this;
    }

    AnimatedStage& clear() noexcept {
        animatedDrawMap.clear();
        return *this;
    }

    AnimatedStage& addDraw(const Model& model, const Instance& instance,
                           const AnimationPlayer& sampler) {
        animatedDrawMap.addDraw(model, instance, sampler);
        return *this;
    }

    template <typename Instances, typename Samplers>
        requires RefConstRange<Instances, Instance> &&
                 RefConstRange<Samplers, AnimationPlayer>
    AnimatedStage& addDraw(const Model& model, Instances&& instances,
                           Samplers&& samplers) {
        animatedDrawMap.addDraw(model, std::forward<Instances>(instances),
                                std::forward<Samplers>(samplers));
        return *this;
    }

   private:
    template <typename...>
    friend class Pipeline;

    void execute(const CameraMatrices& cameraMatrices) {
        if (shaderProgram) {
            glUseProgram(shaderProgram);
            const auto& locations =
                Shader::getProgramUniformLocations(shaderProgram);
            glUniformMatrix4fv(locations.viewMatrix, 1, GL_FALSE,
                               glm::value_ptr(cameraMatrices.view));
            glUniformMatrix4fv(locations.projectionMatrix, 1, GL_FALSE,
                               glm::value_ptr(cameraMatrices.projection));
            animatedDrawMap.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    AnimatedDrawMap<Vertex, Material, Instance> animatedDrawMap;
};

template <typename... Stages>
class Pipeline {
   public:
    template <typename... Args>
    Pipeline(Args&&... args) noexcept : stages(std::forward<Args>(args)...){};

    void execute(const CameraMatrices& cameraMatrices) noexcept {
        (execute<Stages>(cameraMatrices), ...);
    }

    void clear() noexcept { (clear<Stages>(), ...); }

    template <typename Stage>
    auto& getStage() const noexcept {
        return stages.template get<Stage>();
    }

    template <typename Stage>
    auto& getStage() noexcept {
        return stages.template get<Stage>();
    }

   private:
    using StageList = UniqueTypeList<Stages...>;

    template <typename Stage>
    void execute(const CameraMatrices& cameraMatrices) noexcept {
        stages.template get<Stage>().execute(cameraMatrices);
    }

    template <typename Stage>
    void clear() noexcept {
        stages.template get<Stage>().clear();
    }

    StageList stages;
};

template <typename, typename>
struct AnimatedModelStage;

template <typename Vertex, typename Material, typename Instance>
struct AnimatedModelStage<Model<Vertex, Material>, Instance> {
    using Type = AnimatedStage<Vertex, Material, Instance>;
};

template <typename, typename>
struct DynamicModelStage;

template <typename Vertex, typename Material, typename Instance>
struct DynamicModelStage<Model<Vertex, Material>, Instance> {
    using Type = DynamicStage<Vertex, Material, Instance>;
};

template <typename, typename>
struct StaticModelStage;

template <typename Vertex, typename Material, typename Instance>
struct StaticModelStage<Model<Vertex, Material>, Instance> {
    using Type = StaticStage<Vertex, Material, Instance>;
};

template <template <typename, typename> typename, typename, typename, typename>
struct StageListBuilder;

template <template <typename, typename> typename ModelStage,
          typename... Vertices, typename... Materials, typename... Instances>
struct StageListBuilder<ModelStage, TypeList<Vertices...>,
                        TypeList<Materials...>, TypeList<Instances...>> {
   private:
    using ModelTypeList =
        typename Product<Model, UniqueTypeListBuilder<Materials...>,
                         UniqueTypeListBuilder<Vertices...>>::Type;

    using ModelStageList =
        typename Product<ModelStage, UniqueTypeListBuilder<Instances...>,
                         ModelTypeList>::Type;

    using Builder = typename Unwrap<ModelStageList>::Type;

   public:
    using StageList = typename Builder::TypeList;
};

template <typename, typename, typename>
struct PipelineBuilder;

template <typename... Static, typename... Dynamic, typename... Animated>
struct PipelineBuilder<TypeList<Static...>, TypeList<Dynamic...>,
                       TypeList<Animated...>> {
    using Type = Pipeline<Static..., Dynamic..., Animated...>;
};

template <typename... Vertices>
struct AnimatedList {
   private:
    using AnimatedVertices =
        typename Filter<IsAnimatedVertex,
                        UniqueTypeListBuilder<Vertices...>>::Type;

   public:
    using Type = typename AnimatedVertices::TypeList;
};

template <typename, typename, typename, typename, template <typename> typename>
class Renderer;

template <typename... Vertices, typename... Materials, typename... Instances,
          typename... Storage, template <typename> typename MaterialData>
class Renderer<TypeList<Vertices...>, TypeList<Materials...>,
               TypeList<Instances...>, TypeList<Storage...>, MaterialData> {
   public:
    using InstanceStreams = StreamList<Instances...>;
    using StorageStreams = StreamList<Storage...>;

    Renderer(InstanceStreams&& instanceStreamsList,
             StorageStreams&& storageStreamsList) noexcept
        : instanceStreams{std::move(instanceStreamsList)},
          storageStreams{std::move(storageStreamsList)},
          pipeline{instanceStreams, storageStreams} {}

    void beginFrame() noexcept {
        instanceStreams.beginGeneration();
        storageStreams.beginGeneration();

        pipeline.clear();
    }

    void endFrame(const CameraMatrices& cameraMatrices) noexcept {
        instanceStreams.endGeneration();
        storageStreams.endGeneration();

        pipeline.execute(cameraMatrices);
    }

    template <template <typename, typename, typename> typename Stage,
              typename Vertex, typename Material, typename Instance>
    auto& setShader(const Shader& shader) noexcept {
        pipeline
            .template getStage<
                Stage<Vertex, MaterialData<Material>, Instance>>()
            .setShader(shader);
        return *this;
    };

    template <typename Vertex, typename Material, typename Instance>
    auto& addDraw(
        const StaticBatchHandle<Vertex, Material, Instance>& packHandle,
        const glm::mat4& instanceOffset = glm::mat4(1.0f)) {
        pipeline.template getStage<StaticStage<Vertex, Material, Instance>>()
            .addDraw(packHandle, instanceOffset);
        return *this;
    }

    template <typename Vertex, typename Material, typename Instance>
    auto& addDraw(const Model<Vertex, Material>& model,
                  const Instance& instance) {
        pipeline.template getStage<DynamicStage<Vertex, Material, Instance>>()
            .addDraw(model, instance);
        return *this;
    }

    template <typename Vertex, typename Material, typename Instance>
    auto& addDraw(const Model<Vertex, Material>& model,
                  const Instance& instance, const AnimationPlayer& sampler) {
        pipeline.template getStage<AnimatedStage<Vertex, Material, Instance>>()
            .addDraw(model, instance, sampler);
        return *this;
    }

   private:
    using AnimatedList = typename AnimatedList<Vertices...>::Type;
    using VerticesList = TypeList<Vertices...>;
    using MaterialsList = TypeList<MaterialData<Materials>...>;
    using InstancesList = TypeList<Instances...>;

    using StaticStages =
        typename StageListBuilder<StaticModelStage, VerticesList, MaterialsList,
                                  InstancesList>::StageList;
    using DynamicStages =
        typename StageListBuilder<DynamicModelStage, VerticesList,
                                  MaterialsList, InstancesList>::StageList;
    using AnimatedStages =
        typename StageListBuilder<AnimatedModelStage, AnimatedList,
                                  MaterialsList, InstancesList>::StageList;

    using Pipeline = typename PipelineBuilder<StaticStages, DynamicStages,
                                              AnimatedStages>::Type;

    InstanceStreams instanceStreams;
    StorageStreams storageStreams;
    Pipeline pipeline;
};
