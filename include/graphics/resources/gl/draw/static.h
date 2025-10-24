#pragma once

#include <glad/glad.h>

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
#include "graphics/storage/gl/draw/static/pack.h"
#include "graphics/storage/gl/material.h"

template <typename Vertex, typename Material, typename Instance>
class StaticDrawMap {
   public:
    using StaticPack = StaticPack<Vertex, Material, Instance>;
    using StaticPackHandle = typename StaticPack::Handle;
    using PackHandlesView = typename StaticPack::PackHandlesView;

    StaticDrawMap() = default;

    StaticDrawMap(const StaticDrawMap&) = delete;
    StaticDrawMap& operator=(const StaticDrawMap&) = delete;

    StaticDrawMap(StaticDrawMap&&) = default;
    StaticDrawMap& operator=(StaticDrawMap&& other) = default;

    struct Draw {
        StaticPackHandle staticPack;
        glm::mat4 instanceOffset;

        Draw(StaticPackHandle&& packHandle,
             const glm::mat4& instanceOffset) noexcept
            : staticPack(std::move(packHandle)),
              instanceOffset(instanceOffset) {};

        Draw(const Draw&) = delete;
        Draw& operator=(const Draw&) = delete;

        Draw(Draw&&) = default;
        Draw& operator=(Draw&&) = default;

        bool canJoin(const Draw& other) const noexcept { return false; }

        void join(const Draw& other) noexcept { std::unreachable(); }

        void execute(const UniformLocations& uniformLocations) const noexcept {
            glUniformMatrix4fv(uniformLocations.instanceOffset, 1, GL_FALSE,
                               glm::value_ptr(instanceOffset));
            staticPack.get().get().draw(uniformLocations);
        }
    };

    StaticDrawMap& addDraw(const StaticPackHandle& packHandle,
                           const glm::mat4& instanceOffset) {
        const auto& handlesView = packHandle.get().get().getPackHandlesView();
        drawCallMap.pushDrawCall(handlesView,
                                 Draw(packHandle.copy(), instanceOffset));
        return *this;
    }

    void clear() noexcept { drawCallMap.clear(); }

   private:
    friend class StaticStage<Vertex, Material, Instance>;

    void draw(const UniformLocations& uniformLocations) {
        for (auto& [packHandles, drawCalls] : drawCallMap.getDrawCalls()) {
            if (drawCalls.empty()) continue;
            packHandles.bind();
            for (const auto& draw : drawCalls) {
                draw.execute(uniformLocations);
            }
        }
    }

    DrawCallMap<StaticDrawMap> drawCallMap;
};
