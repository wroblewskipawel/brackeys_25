#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <vector>

#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/mesh.h"
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

template <typename Vertex, typename Material>
class Stage {
   public:
    Stage(DrawPackBuilder<Vertex, Material>&& builder)
        : drawPack(builder.build()) {}

    Stage& setShader(const Shader& shader) {
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
            drawPack.draw(locations);
        }
    }

    GLuint shaderProgram{0};
    DrawPack<Vertex, Material> drawPack;
};
