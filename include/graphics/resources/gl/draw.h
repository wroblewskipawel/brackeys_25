#pragma once

#include <glad/glad.h>

#include <functional>
#include <glm/glm.hpp>
#include <ranges>
#include <type_traits>
#include <vector>

#include "concepts/range.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"

template <typename Vertex, typename Material, typename Instance>
class StaticDrawMap;

template <typename Vertex, typename Material, typename Instance>
class StaticBatchBuilder;

template <typename Vertex, typename Material, typename Instance>
class StaticBatch;

template <typename Vertex, typename Material, typename Instance>
class DynamicDrawMap;

template <typename Vertex, typename Material, typename Instance>
class AnimatedDrawMap;

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
    typename Pack::PackHandlesView;
};

template <DrawPackType Pack>
class DrawCallMap {
   public:
    using Draw = typename Pack::Draw;
    using PackHandlesView = typename Pack::PackHandlesView;

    DrawCallMap() = default;

    DrawCallMap(const DrawCallMap&) = delete;
    DrawCallMap& operator=(const DrawCallMap&) = delete;

    DrawCallMap(DrawCallMap&&) = default;
    DrawCallMap& operator=(DrawCallMap&&) = default;

    void clear() noexcept { drawCallMap.clear(); }

    const auto& getDrawCalls() const noexcept { return drawCallMap; }

    void pushDrawCall(const PackHandlesView& handlesView,
                      Draw&& drawCall) noexcept {
        auto& drawCallsVector = getDrawCallVector(handlesView);
        if (!drawCallsVector.empty()) {
            auto& lastDraw = drawCallsVector.back();
            if (lastDraw.canJoin(drawCall)) {
                lastDraw.join(drawCall);
                return;
            }
        }
        drawCallsVector.emplace_back(std::move(drawCall));
    }

    template <typename Range>
        requires RefConstRange<Range, Draw>
    void pushDrawCalls(const PackHandlesView& handlesView,
                       Range&& drawCalls) noexcept {
        auto drawCallsBegin = drawCalls.begin();
        auto& drawCallsVector = getDrawCallVector(handlesView);
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
    auto& getDrawCallVector(const PackHandlesView& handlesView) noexcept {
        auto drawCallVectorIt = drawCallMap.find(handlesView);
        if (drawCallVectorIt == drawCallMap.end()) {
            drawCallMap.emplace(std::piecewise_construct,
                                std::forward_as_tuple(handlesView.getOwned()),
                                std::forward_as_tuple(std::vector<Draw>{}));
        }
        return drawCallMap.find(handlesView)->second;
    };

    typename PackMapTypes<PackHandlesView, std::vector<Draw>>::PackMap
        drawCallMap{};
};
