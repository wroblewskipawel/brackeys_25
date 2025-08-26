#pragma once

#include <glm/glm.hpp>

#include "mesh.h"

MeshData<ColoredVertex> createCube() {
    std::vector<ColoredVertex> vertices{
        // Bottom-back-left
        {.position = {-0.5f, -0.5f, -0.5f},
         .color = {0.0f, 0.0f, 0.0f}},  // 0: black
        // Bottom-back-right
        {.position = {0.5f, -0.5f, -0.5f},
         .color = {1.0f, 0.0f, 0.0f}},  // 1: red
        // Top-back-left
        {.position = {-0.5f, 0.5f, -0.5f},
         .color = {0.0f, 1.0f, 0.0f}},  // 2: green
        // Top-back-right
        {.position = {0.5f, 0.5f, -0.5f},
         .color = {1.0f, 1.0f, 0.0f}},  // 3: yellow
        // Bottom-front-left
        {.position = {-0.5f, -0.5f, 0.5f},
         .color = {0.0f, 0.0f, 1.0f}},  // 4: blue
        // Bottom-front-right
        {.position = {0.5f, -0.5f, 0.5f},
         .color = {1.0f, 0.0f, 1.0f}},  // 5: magenta
        // Top-front-left
        {.position = {-0.5f, 0.5f, 0.5f},
         .color = {0.0f, 1.0f, 1.0f}},  // 6: cyan
        // Top-front-right
        {.position = {0.5f, 0.5f, 0.5f},
         .color = {1.0f, 1.0f, 1.0f}}  // 7: white
    };
    std::vector<GLuint> indices{// Front face (z = +0.5)
                                4, 5, 6, 5, 7, 6,
                                // Back face (z = -0.5)
                                0, 2, 1, 1, 2, 3,
                                // Left face (x = -0.5)
                                0, 4, 2, 2, 4, 6,
                                // Right face (x = +0.5)
                                1, 3, 5, 3, 7, 5,
                                // Bottom face (y = -0.5)
                                0, 1, 4, 1, 5, 4,
                                // Top face (y = +0.5)
                                2, 6, 3, 3, 6, 7};
    return {std::move(vertices), std::move(indices)};
}
