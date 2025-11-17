#pragma once

#include <glad/glad.h>

#include "collections/unique_list.h"
#include "concepts/range.h"
#include "graphics/renderer/pipeline.h"
#include "graphics/renderer/stage/animated.h"
#include "graphics/renderer/stage/dynamic.h"
#include "graphics/renderer/stage/static.h"
#include "graphics/resources/gl/buffer/stream/list.h"
#include "graphics/resources/gl/draw/animated.h"
#include "graphics/resources/gl/draw/dynamic.h"
#include "graphics/resources/gl/draw/static.h"
#include "graphics/resources/gl/draw/static/batch.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"

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

template <typename, typename, typename, typename>
class Renderer;

template <typename... Vertices, typename... Materials, typename... Instances,
          typename... Storage>
class Renderer<TypeList<Vertices...>, TypeList<Materials...>,
               TypeList<Instances...>, TypeList<Storage...>> {
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
        pipeline.template getStage<Stage<Vertex, Material, Instance>>()
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
    using MaterialsList = TypeList<Materials...>;
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
