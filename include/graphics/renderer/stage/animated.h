#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "concepts/range.h"
#include "graphics/resources/gl/buffer/stream/list.h"
#include "graphics/resources/gl/draw/animated.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"

template <typename Vertex, typename Material, typename Instance>
class AnimatedStage {
   public:
    using Shader = Shader<Vertex, Material, Instance>;
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
                Uniform::getProgramUniformLocations(shaderProgram);
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

template <typename, typename>
struct AnimatedModelStage;

template <typename Vertex, typename Material, typename Instance>
struct AnimatedModelStage<Model<Vertex, Material>, Instance> {
    using Type = AnimatedStage<Vertex, Material, Instance>;
};
