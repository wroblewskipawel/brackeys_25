#pragma once

#include <glad/glad.h>

struct ArrayUnlitMaterialLocations {
    GLint albedoSampler{-1};

    ArrayUnlitMaterialLocations() = default;

    ArrayUnlitMaterialLocations(GLuint program) {
        albedoSampler = glGetUniformLocation(program, "arrayUnlit.albedo");
    }
};

struct ArrayMaterial {
    ArrayUnlitMaterialLocations unlitMaterial;

    ArrayMaterial() = default;

    ArrayMaterial(GLuint program) : unlitMaterial{program} {}
};

struct UniformLocations {
    GLint materialIndex{-1};
    GLint materialPack{-1};
    GLint modelMatrix{-1};
    GLint instanceOffset{-1};
    GLint viewMatrix{-1};
    GLint projectionMatrix{-1};
    GLint jointMatrixCount{-1};
    GLint jointMatrixOffset{-1};

    ArrayMaterial arrayMaterials;

    UniformLocations() = default;

    UniformLocations(GLuint program) : arrayMaterials{program} {
        modelMatrix = glGetUniformLocation(program, "model");
        instanceOffset = glGetUniformLocation(program, "offset");
        viewMatrix = glGetUniformLocation(program, "view");
        projectionMatrix = glGetUniformLocation(program, "projection");
        materialIndex = glGetUniformLocation(program, "material");
        materialPack = glGetUniformLocation(program, "materialPack");
        jointMatrixCount = glGetUniformLocation(program, "jointMatrixCount");
        jointMatrixOffset = glGetUniformLocation(program, "jointMatrixOffset");
    }
};
