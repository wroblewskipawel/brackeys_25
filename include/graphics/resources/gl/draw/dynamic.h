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

template <typename Vertex, typename Material, typename Instance>
class DynamicPack {
   public:
    using Model = Model<Vertex, Material>;

    DynamicPack(StreamHandle<Instance>&& streamBuffer) noexcept
        : streamBuffer(std::move(streamBuffer)) {}

    DynamicPack(const DynamicPack&) = delete;
    DynamicPack& operator=(const DynamicPack&) = delete;

    DynamicPack(DynamicPack&& other) noexcept
        : streamBuffer(std::move(other.streamBuffer)),
          drawCallMap(std::move(other.drawCallMap)) {
        other.streamBuffer = StreamHandle<Instance>::getInvalid();
    };

    DynamicPack& operator=(DynamicPack&& other) noexcept {
        if (this != &other) {
            drawCallMap = std::move(other.drawCallMap);
            streamBuffer = other.streamBuffer;
            other.streamBuffer = StreamHandle<Instance>::getInvalid();
        }
        return *this;
    };

    DynamicPack& addDraw(const Model& model, const Instance& instanceData) {
        return addDraw(model, std::views::single(instanceData));
    }

    template <typename Instances>
        requires RefConstRange<Instances, Instance>
    DynamicPack& addDraw(const Model& model, Instances&& instanceData) {
        drawCallMap.pushDrawCalls(
            model, getDrawCalls(model, std::forward<Instances>(instanceData)));
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
    };

   private:
    friend class DynamicStage<Vertex, Material, Instance>;

    void draw(const UniformLocations& uniformLocations) {
        for (auto& [packHandles, drawCalls] : drawCallMap.getDrawCalls()) {
            if (drawCalls.empty()) continue;
            packHandles.bind();
            auto& stream = streamBuffer.get().get();
            for (const auto& draw : drawCalls) {
                auto& instanceAllocation = draw.instanceAllocation;
                auto instanceBuffer = stream.getBuffer(instanceAllocation);
                VertexArray<Vertex, Instance>::getVertexArray()
                    .bindBuffer<BindingIndex::InstanceAttributes>(BindingInfo{
                        .buffer = instanceBuffer.get(),
                        .offset = 0,
                    });
                if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
                    glUniform1ui(
                        uniformLocations.materialIndex,
                        static_cast<GLuint>(draw.drawInfo.materialIndex));
                }
                glDrawElementsInstancedBaseInstance(
                    GL_TRIANGLES, draw.drawInfo.meshOffsets.indexCount,
                    GL_UNSIGNED_INT,
                    (void*)(draw.drawInfo.meshOffsets.indexOffset *
                            sizeof(GLuint)),
                    instanceAllocation.numInstances,
                    instanceAllocation.bufferOffset);
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
    DrawCallMap<DynamicPack> drawCallMap;
};
