#pragma once

#include <glad/glad.h>

#include <algorithm>
#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "graphics/resources/gl/buffer/static.h"
#include "graphics/resources/gl/draw.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/draw/static/batch.h"
#include "graphics/storage/gl/material.h"

template <typename Instance>
using InstanceDataMap = std::unordered_map<DrawInfo, std::vector<Instance>>;

template <typename Instance>
inline auto allocateStaticBuffer(const InstanceDataMap<Instance>& instanceMap) {
    size_t numInstances = std::ranges::fold_left(
        instanceMap, 0, [](auto acc, const auto& drawData) {
            return acc + drawData.second.size();
        });
    auto instanceData = std::vector<Instance>(numInstances);
    auto writeHead = instanceData.begin();
    for (const auto& [_, instanceData] : instanceMap) {
        std::ranges::copy(instanceData, writeHead);
        writeHead += instanceData.size();
    }
    return StaticBuffer<Instance>{instanceData};
}

template <typename Vertex, typename Material, typename Instance>
using PackDrawMap =
    std::unordered_map<PackHandles<Vertex, Material>, std::vector<Instance>>;

template <typename Vertex, typename Material, typename Instance>
class StaticBatch {
   public:
    using Handle = StaticBatchHandle<Vertex, Material, Instance>;
    using PackHandles = PackHandles<Vertex, Material>;
    using PackHandlesView = PackHandlesView<Vertex, Material>;
    using InstanceDataMap = InstanceDataMap<Instance>;

    StaticBatch(const StaticBatch&) = delete;
    StaticBatch& operator=(const StaticBatch&) = delete;

    StaticBatch(StaticBatch&&) = default;
    StaticBatch& operator=(StaticBatch&& other) = default;

   private:
    friend class StaticBatchBuilder<Vertex, Material, Instance>;
    friend class StaticDrawMap<Vertex, Material, Instance>;

    struct Draw {
        DrawInfo drawInfo;
        uint32_t numInstances;
        uint32_t baseInstance;

        void execute(const UniformLocations& uniformLocations) const noexcept {
            if constexpr (!EmptyMaterialType<Material>) {
                glUniform1ui(uniformLocations.materialIndex,
                             static_cast<GLuint>(drawInfo.materialIndex));
            }
            glDrawElementsInstancedBaseInstance(
                GL_TRIANGLES, drawInfo.meshOffsets.indexCount, GL_UNSIGNED_INT,
                (void*)(drawInfo.meshOffsets.indexOffset * sizeof(GLuint)),
                numInstances, baseInstance);
        }
    };

    StaticBatch(InstanceDataMap&& drawData, PackHandles&& packHandles) noexcept
        : packHandles{std::move(packHandles)},
          instanceBuffer{allocateStaticBuffer(drawData)} {
        drawCalls.reserve(drawData.size());
        size_t baseInstance = 0;
        for (const auto& [drawInfo, instances] : drawData) {
            drawCalls.emplace_back(
                Draw{.drawInfo = drawInfo,
                     .numInstances = static_cast<uint32_t>(instances.size()),
                     .baseInstance = static_cast<uint32_t>(baseInstance)});
            baseInstance += instances.size();
        }
    }

    void draw(const UniformLocations& uniformLocations) const noexcept {
        auto bufferInfo = instanceBuffer.getBufferInfo();
        VertexArray<Vertex, Instance>::getVertexArray()
            .template bindBuffer<BindingIndex::InstanceAttributes>(BindingInfo{
                .buffer = bufferInfo.buffer,
                .offset = 0,
            });
        for (const auto& drawCall : drawCalls) {
            drawCall.execute(uniformLocations);
        }
    }

    auto getPackHandlesView() const noexcept {
        return PackHandlesView(packHandles);
    }

    PackHandles packHandles;
    StaticBuffer<Instance> instanceBuffer;
    std::vector<Draw> drawCalls;
};

template <typename Vertex, typename Material, typename Instance>
class StaticBatchBuilder {
   public:
    using Model = Model<Vertex, Material>;
    using PackHandles = PackHandles<Vertex, Material>;
    using PackHandlesView = PackHandlesView<Vertex, Material>;
    using InstanceDataMap = InstanceDataMap<Instance>;
    using StaticBatchHandle = StaticBatchHandle<Vertex, Material, Instance>;

    StaticBatchBuilder() noexcept = default;

    StaticBatchBuilder& addDraw(const Model& model, Instance instanceData) {
        auto& drawData = getInstanceDataMap(model.getPackHandlesView());
        auto drawInfo = DrawInfo(model);
        auto drawDataIt = drawData.find(drawInfo);
        if (drawDataIt != drawData.end()) {
            drawDataIt->second.emplace_back(instanceData);
        } else {
            std::vector<Instance> instances{instanceData};
            drawData.emplace(std::piecewise_construct,
                             std::forward_as_tuple(drawInfo),
                             std::forward_as_tuple(std::move(instances)));
        }
        return *this;
    }

    StaticBatchBuilder& addDrawMulti(const Model& model,
                                     std::vector<Instance>&& instanceData) {
        auto& drawData = getInstanceDataMap(model.getPackHandlesView());
        auto drawInfo = DrawInfo(model);
        auto drawDataIt = drawData.find(drawInfo);
        if (drawDataIt != drawData.end()) {
            drawDataIt->second.insert(drawDataIt->second.end(),
                                      std::move(instanceData));
        } else {
            drawData.emplace(std::piecewise_construct,
                             std::forward_as_tuple(drawInfo),
                             std::forward_as_tuple(std::move(instanceData)));
        }
        return *this;
    }

    auto& getInstanceDataMap(const PackHandlesView& handlesView) noexcept {
        auto drawCallVectorIt = drawCallMap.find(handlesView);
        if (drawCallVectorIt == drawCallMap.end()) {
            drawCallMap.emplace(std::piecewise_construct,
                                std::forward_as_tuple(handlesView.getOwned()),
                                std::forward_as_tuple(InstanceDataMap{}));
        }
        return drawCallMap.find(handlesView)->second;
    };

    auto build() {
        auto batchHandles = std::vector<StaticBatchHandle>{};
        batchHandles.reserve(drawCallMap.size());
        while (!drawCallMap.empty()) {
            auto batchData = drawCallMap.extract(drawCallMap.begin());
            batchHandles.emplace_back(
                registerStaticBatch(StaticBatch<Vertex, Material, Instance>{
                    std::move(batchData.mapped()),
                    std::move(batchData.key())}));
        }
        return batchHandles;
    }

   private:
    typename PackMapTypes<PackHandles, InstanceDataMap>::PackUnorderedMap
        drawCallMap{};
};
