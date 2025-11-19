#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "graphics/resources/gl/shader/uniform.h"

enum class ShaderStage : GLenum {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER
};

struct CameraMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

template <typename, typename, typename>
class ShaderBuilder;

template <typename, typename, typename>
class Shader {
   public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept {
        program = other.program;
        other.program = 0;
    };

    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            glDeleteProgram(program);
            program = other.program;
            other.program = 0;
        }
        return *this;
    };

    ~Shader() noexcept { 
        Uniform::popProgramUniformLocations(program);
        glDeleteProgram(program);
    }

   private:
    template <typename, typename, typename>
    friend class ShaderBuilder;
    template <typename, typename, typename>
    friend class StaticStage;
    template <typename, typename, typename>
    friend class DynamicStage;
    template <typename, typename, typename>
    friend class AnimatedStage;

    static Shader invalid() noexcept { return Shader(0); }
    
    Shader(GLuint program) noexcept : program(program) {
        Uniform::setProgramUniformLocations(program);
    }
    
    GLuint program;
};

template <typename Vertex, typename Material, typename Instance>
class ShaderBuilder {
   public:
    ShaderBuilder(const ShaderBuilder&) = delete;
    ShaderBuilder& operator=(const ShaderBuilder&) = delete;

    ShaderBuilder(ShaderBuilder&&) noexcept = default;
    ShaderBuilder& operator=(ShaderBuilder&&) noexcept = default;

    ~ShaderBuilder() noexcept = default;

    ShaderBuilder& addStage(ShaderStage stage,
                            const std::filesystem::path& filepath) noexcept {
        stages.emplace(stage, filepath);
        return *this;
    }

    auto build() const noexcept {
        auto stages = buildStages();
        if (stages.empty()) {
            return Shader<Vertex, Material, Instance>::invalid();
        }

        GLuint program = glCreateProgram();
        for (const auto& shader : stages) {
            glAttachShader(program, shader);
        }
        glLinkProgram(program);

        for (const auto& shader : stages) {
            glDeleteShader(shader);
        }

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
            std::println(std::cerr, "Failed to link program\n {}", infoLog);
            glDeleteProgram(program);
            return Shader<Vertex, Material, Instance>::invalid();
        }
        return Shader<Vertex, Material, Instance>(program);
    }

   private:
    friend class Window;

    const static GLsizei infoLogLen = 512;
    inline static GLchar infoLog[infoLogLen];

    ShaderBuilder() = default;

    auto buildStages() const noexcept {
        auto shaders = std::vector<GLuint>{};
        for (const auto& [stage, filepath] : stages) {
            auto shader = buildStage(stage, filepath);
            if (shader != 0) {
                shaders.emplace_back(shader);
            } else {
                for (const auto& shader : shaders) {
                    glDeleteShader(shader);
                }
                shaders.clear();
                break;
            }
        }
        return shaders;
    }

    auto buildStage(ShaderStage stage,
                    const std::filesystem::path& filepath) const noexcept {
        GLuint shader = glCreateShader(static_cast<GLenum>(stage));
        auto source = loadSource(filepath);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
            std::println(std::cerr,
                         "Failed to compile shader [filepath: {}]\n{}",
                         filepath.string(), infoLog);
            glDeleteShader(shader);
            shader = 0;
        }
        return shader;
    }

    std::string loadSource(const std::filesystem::path& filepath) const noexcept {
        std::ifstream fs{};
        std::ostringstream os{};
        fs.exceptions(std::ios::failbit | std::ios::badbit);
        os.exceptions(std::ios::failbit | std::ios::badbit);
        try {
            fs.open(filepath);
            os << fs.rdbuf();
            fs.close();
        } catch (std::exception& e) {
            throw;
        }
        return os.str();
    }

    std::unordered_map<ShaderStage, std::filesystem::path> stages;
};
