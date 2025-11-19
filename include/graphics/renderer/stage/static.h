#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "graphics/resources/gl/buffer/stream/list.h"
#include "graphics/resources/gl/draw/static.h"
#include "graphics/resources/gl/draw/static/batch.h"
#include "graphics/resources/gl/shader.h"

template <typename Vertex, typename Material, typename Instance>
class StaticStage {
   public:
    using Shader = Shader<Vertex, Material, Instance>;
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
                Uniform::getProgramUniformLocations(shaderProgram);
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

template <typename, typename>
struct StaticModelStage;

template <typename Vertex, typename Material, typename Instance>
struct StaticModelStage<Model<Vertex, Material>, Instance> {
    using Type = StaticStage<Vertex, Material, Instance>;
};
