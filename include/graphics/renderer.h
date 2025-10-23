#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <vector>

#include "concepts/range.h"
#include "graphics/resources/gl/draw/animated.h"
#include "graphics/resources/gl/draw/dynamic.h"
#include "graphics/resources/gl/draw/static.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"

template <typename... Stages>
class Pipeline;

template <typename Stage, typename... Stages>
class Pipeline<Stage, Stages...> {
   public:
    Pipeline(Stage&& stage, Stages&&... stages)
        : stage(std::forward<Stage>(stage)),
          stages(std::forward<Stages>(stages)...) {}

    void execute(const CameraMatrices& cameraMatrices) {
        stage.execute(cameraMatrices);
        stages.execute(cameraMatrices);
    }

    template <typename Search>
    Search& getStage() {
        if constexpr (std::is_same_v<Search, Stage>) {
            return stage;
        } else {
            if constexpr (sizeof...(Stages) == 0) {
                static_assert(false, "Stage not present in Pipeline!");
            } else {
                return stages.getStage<Search>();
            }
        }
    }

   private:
    Stage stage;
    Pipeline<Stages...> stages;
};

template <>
class Pipeline<> {
   public:
    void execute(const CameraMatrices& cameraMatrices) {}
};

template <typename... Stages>
Pipeline(Stages&&...) -> Pipeline<std::decay_t<Stages>...>;

template <typename Vertex, typename Material, typename Instance>
class StaticStage {
   public:
    StaticStage(StaticPackBuilder<Vertex, Material, Instance>&& builder)
        : staticPack(builder.build()) {}

    StaticStage& setShader(const Shader& shader) {
        shaderProgram = shader.program;
        return *this;
    }

   private:
    template <typename... Stages>
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
            staticPack.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    StaticPack<Vertex, Material, Instance> staticPack;
};

template <typename Vertex, typename Material, typename Instance>
class DynamicStage {
   public:
    using Model = Model<Vertex, Material>;

    DynamicStage(const StreamHandle<Instance>& streamBuffer)
        : dynamicPack(streamBuffer.copy()) {}

    DynamicStage& setShader(const Shader& shader) {
        shaderProgram = shader.program;
        return *this;
    }

    DynamicStage& clear() noexcept {
        dynamicPack.clear();
        return *this;
    }

    DynamicStage& addDraw(const Model& model, const Instance& instance) {
        dynamicPack.addDraw(model, instance);
        return *this;
    }

    template <typename Instances>
        requires RefConstRange<Instances, Instance>
    DynamicStage& addDraw(const Model& model, Instances&& range) {
        dynamicPack.addDraw(model, std::forward<Instances>(range));
        return *this;
    }

   private:
    template <typename... Stages>
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
            dynamicPack.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    DynamicPack<Vertex, Material, Instance> dynamicPack;
};

template <typename Vertex, typename Material, typename Instance>
class AnimatedStage {
   public:
    using Model = Model<Vertex, Material>;

    AnimatedStage(const StreamHandle<Instance>& instanceStreamBuffer,
                  const StreamHandle<glm::mat4>& jointStreamBuffer)
        : animatedPack(instanceStreamBuffer, jointStreamBuffer) {}

    AnimatedStage& setShader(const Shader& shader) {
        shaderProgram = shader.program;
        return *this;
    }

    AnimatedStage& clear() noexcept {
        animatedPack.clear();
        return *this;
    }

    AnimatedStage& addDraw(const Model& model, const Instance& instance,
                           const AnimationPlayer& sampler) {
        animatedPack.addDraw(model, instance, sampler);
        return *this;
    }

    template <typename Instances, typename Samplers>
        requires RefConstRange<Instances, Instance> &&
                 RefConstRange<Samplers, AnimationPlayer>
    AnimatedStage& addDraw(const Model& model, Instances&& instances,
                           Samplers&& samplers) {
        animatedPack.addDraw(model, std::forward<Instances>(instances),
                             std::forward<Samplers>(samplers));
        return *this;
    }

   private:
    template <typename... Stages>
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
            animatedPack.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    AnimatedPack<Vertex, Material, Instance> animatedPack;
};
