#pragma once

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

#include "concepts/range.h"
#include "graphics/resources/gl/buffer/stream/list.h"
#include "graphics/resources/gl/draw/dynamic.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"

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

template <typename, typename>
struct DynamicModelStage;

template <typename Vertex, typename Material, typename Instance>
struct DynamicModelStage<Model<Vertex, Material>, Instance> {
    using Type = DynamicStage<Vertex, Material, Instance>;
};
