#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "graphics/resources/buffer/ring.h"
#include "graphics/resources/gl/buffer/stream.h"
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
        auto instanceAllocations = streamBuffer.get().get().pushData(
            std::forward<Instances>(instanceData));
        pushDrawCalls(model, instanceAllocations);
        return *this;
    }

    void clear() noexcept {
        // We never remove entries from the map, making the draw pack owner of
        // the shared resources (MeshPack, MaterialPack) and thus extending it
        // lifetime til the DrawPack gets destroyed,
        // This is not desirable and may lead to situations where resources that
        // are no longer intented to be used, aren't released when expected
        // TODO: Resolve
        for (auto& [_, drawCalls] : drawCallMap) {
            drawCalls.clear();
        }
    }

   private:
    friend class DynamicStage<Vertex, Material, Instance>;

    struct Draw {
        DrawInfo drawInfo;
        BufferAllocation<Instance> instanceAllocation;
    };

    using DrawCallMap =
        std::unordered_map<PackHandles<Vertex, Material>, std::vector<Draw>>;

    auto& getDrawCallVector(const Model& model) noexcept {
        auto packHandles = model.getPackHandles();
        auto drawCallVectorIt = drawCallMap.find(packHandles);
        if (drawCallVectorIt == drawCallMap.end()) {
            drawCallMap.emplace(std::piecewise_construct,
                                std::forward_as_tuple(packHandles.copy()),
                                std::forward_as_tuple(std::vector<Draw>{}));
        }
        return drawCallMap.find(packHandles)->second;
    };

    void draw(const UniformLocations& uniformLocations) {
        for (auto& [packHandles, drawCalls] : drawCallMap) {
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
                    (void*)(draw.drawInfo.meshOffsets.indexOffset * sizeof(GLuint)),
                    instanceAllocation.numInstances,
                    instanceAllocation.bufferOffset);
            }
        }
    }

    void pushDrawCalls(
        const Model& model,
        std::vector<BufferAllocation<Instance>> instanceAllocations) noexcept {
        auto& drawCalls = getDrawCallVector(model);
        auto allocationsBegin = instanceAllocations.begin();
        if (!drawCalls.empty() && drawCalls.back().instanceAllocation.tryJoin(
                                      instanceAllocations.front())) {
            allocationsBegin += 1;
        }
        for (const auto& instanceAllocation : std::ranges::subrange(
                 allocationsBegin, instanceAllocations.end())) {
            drawCalls.emplace_back(
                Draw(DrawInfo(model), instanceAllocation));
        }
    }

    StreamHandle<Instance> streamBuffer;
    DrawCallMap drawCallMap;
};
