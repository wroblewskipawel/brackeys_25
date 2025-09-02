#pragma once

#include <glm/glm.hpp>
#include "draw.h"
#include "material.h"
#include "mesh.h"

template <typename Vertex, typename Material>
struct RenderableComponent {
    DrawCommandPartial<Vertex, Material> partial;
    glm::vec3 scale = glm::vec3(1.0f);

    DrawCommand<Vertex, Material> withTransform(const glm::mat4& modelMatrix) {
        return partial.withTransform(modelMatrix);
    }
};

using RenderableUnlit = RenderableComponent<UnlitVertex, UnlitMaterial>;
using RenderableColored = RenderableComponent<ColoredVertex, EmptyMaterial>;
