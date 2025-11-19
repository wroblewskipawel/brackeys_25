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

class Uniform {
   public:
    [[nodiscard]] static const UniformLocations& getProgramUniformLocations(
        GLuint program) noexcept {
        return uniformLocations.at(program);
    }

   private:
    template <typename, typename, typename>
    friend class Shader;

    static void setProgramUniformLocations(GLuint program) noexcept {
        uniformLocations[program] = UniformLocations(program);
    }

    static void popProgramUniformLocations(GLuint program) noexcept {
        uniformLocations.erase(program);
    }

    inline static std::unordered_map<GLuint, UniformLocations> uniformLocations;
};
