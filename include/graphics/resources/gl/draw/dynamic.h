#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "concepts/range.h"
#include "graphics/resources/buffer/ring.h"
#include "graphics/resources/gl/buffer/stream.h"
#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/stream.h"

template <typename, typename, typename>
class DynamicStage;

template <typename Vertex, typename Material, typename Instance>
class DynamicDrawMap {
   public:
    using Model = Model<Vertex, Material>;
    using PackHandlesView = typename Model::PackHandlesView;

    DynamicDrawMap(StreamHandle<Instance>&& streamBuffer) noexcept
        : streamBuffer(std::move(streamBuffer)) {}

    DynamicDrawMap(const DynamicDrawMap&) = delete;
    DynamicDrawMap& operator=(const DynamicDrawMap&) = delete;

    DynamicDrawMap(DynamicDrawMap&& other) noexcept
        : streamBuffer(std::move(other.streamBuffer)),
          drawCallMap(std::move(other.drawCallMap)) {
        other.streamBuffer = StreamHandle<Instance>::getInvalid();
    };

    DynamicDrawMap& operator=(DynamicDrawMap&& other) noexcept {
        if (this != &other) {
            drawCallMap = std::move(other.drawCallMap);
            streamBuffer = other.streamBuffer;
            other.streamBuffer = StreamHandle<Instance>::getInvalid();
        }
        return *this;
    };

    DynamicDrawMap& addDraw(const Model& model, const Instance& instanceData) {
        return addDraw(model, std::views::single(instanceData));
    }

    template <typename Instances>
        requires RefConstRange<Instances, Instance>
    DynamicDrawMap& addDraw(const Model& model, Instances&& instanceData) {
        drawCallMap.pushDrawCalls(
            model.getPackHandlesView(),
            getDrawCalls(model, std::forward<Instances>(instanceData)));
        return *this;
    }

    void clear() noexcept { drawCallMap.clear(); }

    struct Draw {
        DrawInfo drawInfo;
        BufferAllocation<Instance> instanceAllocation;

        bool canJoin(const Draw& other) const noexcept {
            return instanceAllocation.canJoin(other.instanceAllocation);
        }

        void join(const Draw& other) noexcept {
            return instanceAllocation.join(other.instanceAllocation);
        }

        void execute(
            const UniformLocations& uniformLocations,
            const StreamBuffer<Instance>& instanceStream) const noexcept {
            VertexArray<Vertex, Instance>::getVertexArray()
                .template bindBuffer<BindingIndex::InstanceAttributes>(
                    BindingInfo{
                        .buffer =
                            instanceStream.getBuffer(instanceAllocation).get(),
                        .offset = 0,
                    });
            if constexpr (!EmptyMaterialType<Material>) {
                glUniform1ui(uniformLocations.materialIndex,
                             static_cast<GLuint>(drawInfo.materialIndex));
            }
            glDrawElementsInstancedBaseInstance(
                GL_TRIANGLES, drawInfo.meshOffsets.indexCount, GL_UNSIGNED_INT,
                (void*)(drawInfo.meshOffsets.indexOffset * sizeof(GLuint)),
                instanceAllocation.numInstances,
                instanceAllocation.bufferOffset);
        }
    };

   private:
    friend class DynamicStage<Vertex, Material, Instance>;

    void draw(const UniformLocations& uniformLocations) {
        auto& stream = streamBuffer.get().get();
        for (auto& [packHandles, drawCalls] : drawCallMap.getDrawCalls()) {
            if (drawCalls.empty()) continue;
            packHandles.bind();
            for (const auto& draw : drawCalls) {
                draw.execute(uniformLocations, stream);
            }
        }
    }

    template <typename Instances>
        requires RefConstRange<Instances, Instance>
    auto getDrawCalls(const Model& model, Instances&& instanceData) noexcept {
        auto instanceAllocations = streamBuffer.get().get().pushData(
            std::forward<Instances>(instanceData));

        auto drawInfo = DrawInfo(model);
        auto drawCalls = std::vector<Draw>{};
        drawCalls.reserve(instanceAllocations.size());

        for (const auto& allocation : instanceAllocations) {
            drawCalls.emplace_back(Draw(drawInfo, allocation));
        }
        return drawCalls;
    }

    StreamHandle<Instance> streamBuffer;
    DrawCallMap<DynamicDrawMap> drawCallMap;
};
