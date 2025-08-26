#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <vector>

#include "mesh.h"
#include "shader.h"

template <typename... Stages>
class Pipeline;

template <typename Vertex, typename... Stages>
class Pipeline<Vertex, Stages...> {
   public:
    Pipeline(Stage<Vertex>&& stage, Stages&&... stages)
        : stage(std::forward<Stage>(stage)),
          stages(std::forward<Stages>(stages)...) {}

    void execute(const CameraMatrices& cameraMatrices) {
        stages.execute(cameraMatrices);
        stage.execute(cameraMatrices);
    }

   private:
    Stage<Vertex> stage;
    Pipeline<Stages...> stages;
};

template <typename Vertex>
class Pipeline<Vertex> {
   public:
    Pipeline(Stage<Vertex>&& stage)
        : stage(std::forward<Stage<Vertex>>(stage)) {}

    void execute(const CameraMatrices& cameraMatrices) {
        stage.execute(cameraMatrices);
    }

   private:
    Stage<Vertex> stage;
};

template <typename Vertex>
class Stage {
   public:
    Stage(DrawPackBuilder<Vertex>&& builder) : drawPack(builder.build()) {}

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
        }
        drawPack.draw();
    }

    GLuint shaderProgram{0};
    DrawPack<Vertex> drawPack;
};
