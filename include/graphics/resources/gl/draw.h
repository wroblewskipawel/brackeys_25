#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "concepts/range.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
#include "graphics/resources/gl/shader.h"
#include "graphics/resources/gl/vertex_array.h"
#include "graphics/storage/gl/material.h"

template <typename Vertex, typename Material, typename Instance>
class StaticPackBuilder;

template <typename Vertex, typename Material, typename Instance>
class StaticPack;

template <typename Vertex, typename Material, typename Instance>
class DynamicPack;

template <typename Vertex, typename Material, typename Instance>
class AnimatedPack;

template <typename Vertex, typename Material, typename Instance>
class Stage;

struct DrawInfo {
    template <typename Vertex, typename Material>
    DrawInfo(const Model<Vertex, Material>& model) noexcept
        : meshOffsets{model.getMeshOffsets()},
          materialIndex{model.getMaterialIndex()} {}

    DrawInfo(const DrawInfo&) = default;
    DrawInfo& operator=(const DrawInfo&) = default;

    DrawInfo(DrawInfo&&) = default;
    DrawInfo& operator=(DrawInfo&&) = default;

    MeshOffsets meshOffsets;
    uint32_t materialIndex;

    friend bool operator==(const DrawInfo& lhs, const DrawInfo& rhs) noexcept {
        return lhs.meshOffsets == rhs.meshOffsets &&
               lhs.materialIndex == rhs.materialIndex;
    }
};
namespace std {
template <>
struct hash<DrawInfo> {
    std::size_t operator()(const DrawInfo& drawInfo) const noexcept {
        std::size_t h1 = std::hash<MeshOffsets>{}(drawInfo.meshOffsets);
        std::size_t h2 = std::hash<uint32_t>{}(drawInfo.materialIndex);
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

template <typename Pack>
concept DrawPackType = requires {
    typename Pack::Draw;
    typename Pack::Model;
};

template <DrawPackType Pack>
class DrawCallMap {
   public:
    using Draw = typename Pack::Draw;
    using Model = typename Pack::Model;

    DrawCallMap() = default;

    DrawCallMap(const DrawCallMap&) = delete;
    DrawCallMap& operator=(const DrawCallMap&) = delete;

    DrawCallMap(DrawCallMap&&) = default;
    DrawCallMap& operator=(DrawCallMap&&) = default;

    void clear() noexcept { drawCallMap.clear(); }

    const auto& getDrawCalls() const noexcept { return drawCallMap; }

    template <typename Range>
        requires RefConstRange<Range, Draw>
    void pushDrawCalls(const Model& model, Range&& drawCalls) noexcept {
        auto drawCallsBegin = drawCalls.begin();
        auto& drawCallsVector = getDrawCallVector(model);
        if (!drawCallsVector.empty()) {
            auto& lastDraw = drawCallsVector.back();
            if (lastDraw.canJoin(*drawCallsBegin)) {
                lastDraw.join(*drawCallsBegin);
                drawCallsBegin += 1;
            }
        }
        for (const auto& drawCall :
             std::ranges::subrange(drawCallsBegin, drawCalls.end())) {
            drawCallsVector.emplace_back(drawCall);
        }
    }

   private:
    auto& getDrawCallVector(const Model& model) noexcept {
        auto packHandles = model.getPackHandlesView();
        auto drawCallVectorIt = drawCallMap.find(packHandles);
        if (drawCallVectorIt == drawCallMap.end()) {
            drawCallMap.emplace(std::piecewise_construct,
                                std::forward_as_tuple(packHandles.getOwned()),
                                std::forward_as_tuple(std::vector<Draw>{}));
        }
        return drawCallMap.find(packHandles)->second;
    };

    Model::template PackUnorderedMap<std::vector<Draw>> drawCallMap{};
};
