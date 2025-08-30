#pragma once

#include "draw.h"
#include "material.h"
#include "mesh.h"

template <typename Vertex, typename Material>
struct RenderableComponent {
    DrawCommandPartial<Vertex, Material> partial;

    DrawCommand<Vertex, Material> withTransform(const glm::mat4& modelMatrix) {
        return partial.withTransform(modelMatrix);
    }
};

using RenderableUnlit = RenderableComponent<UnlitVertex, UnlitMaterial>;
using RenderableColored = RenderableComponent<ColoredVertex, EmptyMaterial>;
