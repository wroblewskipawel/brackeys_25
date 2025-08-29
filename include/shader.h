#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

enum class ShaderStage : GLenum {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER
};

struct CameraMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

struct UniformLocations {
    GLint materialIndex{-1};
    GLint materialPack{-1};
    GLint modelMatrix{-1};
    GLint viewMatrix{-1};
    GLint projectionMatrix{-1};
};

class ShaderBuilder;

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

    ~Shader() noexcept { glDeleteProgram(program); }

   private:
    template <typename Vertex, typename Material>
    friend class Stage;
    friend class ShaderBuilder;

    static Shader invalid() noexcept { return Shader(0); }

    inline static std::unordered_map<GLuint, UniformLocations> uniformLocations;

    const UniformLocations& getUniformLocations() const noexcept {
        return uniformLocations.at(program);
    }

    static const UniformLocations& getProgramUniformLocations(
        GLuint program) noexcept {
        return uniformLocations.at(program);
    }

    Shader(GLuint program) noexcept : program(program) {
        UniformLocations locations;
        locations.modelMatrix = glGetUniformLocation(program, "model");
        locations.viewMatrix = glGetUniformLocation(program, "view");
        locations.projectionMatrix =
            glGetUniformLocation(program, "projection");
        locations.materialIndex = glGetUniformLocation(program, "material");
        locations.materialPack = glGetUniformLocation(program, "materialPack");
        uniformLocations[program] = locations;
    }

    GLuint program;
};

class ShaderBuilder {
   public:
    ShaderBuilder() = default;

    ShaderBuilder(const ShaderBuilder&) = delete;
    ShaderBuilder& operator=(const ShaderBuilder&) = delete;

    ShaderBuilder(ShaderBuilder&&) = default;
    ShaderBuilder& operator=(ShaderBuilder&&) = default;

    ~ShaderBuilder() noexcept {
        for (const auto& [stage, shader] : stages) {
            glDeleteShader(shader);
        }
    }

    ShaderBuilder& addStage(ShaderStage stage,
                            const std::filesystem::path& filepath) noexcept {
        GLuint shader = glCreateShader(static_cast<GLenum>(stage));
        auto source = loadSource(filepath);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
            std::printf("Failed to compile shader [%s]\n%s", source.c_str(),
                        infoLog);
            glDeleteShader(shader);
        } else {
            stages.find(stage) != stages.end() ? glDeleteShader(stages[stage])
                                               : void();
            stages[stage] = shader;
        }

        return *this;
    }

    Shader build() const noexcept {
        GLuint program = glCreateProgram();
        for (const auto& [stage, shader] : stages) {
            glAttachShader(program, shader);
        }

        glLinkProgram(program);
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
            std::printf("Failed to link program\n%s", infoLog);
            glDeleteProgram(program);
            return Shader::invalid();
        }
        return Shader(program);
    }

   private:
    const static GLsizei infoLogLen = 512;
    inline static GLchar infoLog[infoLogLen];

    std::string loadSource(const std::filesystem::path& filepath) {
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

    std::unordered_map<ShaderStage, GLuint> stages;
};
