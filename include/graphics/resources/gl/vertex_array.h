#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <magic_enum.hpp>
#include <type_traits>
#include <vector>

#include "graphics/resources/mesh.h"

template <typename Vertex>
class MeshPack;

template <typename Vertex, typename Material, typename Instance>
class StaticBatch;

template <typename Instance, size_t InstanceCount>
class InstanceBuffer;

template <typename Vertex, typename Instance>
class VertexArray;

struct BindingInfo {
    GLuint buffer{0};
    GLuint offset{0};

    friend bool operator==(const BindingInfo&, const BindingInfo&) noexcept;
};

bool operator==(const BindingInfo& lhs, const BindingInfo& rhs) noexcept {
    return lhs.buffer == rhs.buffer && lhs.offset == rhs.offset;
}

class VertexArrayStorage {
   public:
    static void destroyVertexArrays() noexcept {
        glDeleteVertexArrays(vertexArrayStorage.size(),
                             vertexArrayStorage.data());
        currentVertexArray = 0;
    }

   private:
    template <typename Vertex, typename Instance>
    friend class VertexArray;

    inline static GLuint currentVertexArray = {0};
    inline static std::vector<GLuint> vertexArrayStorage;
};

enum class BindingIndex : GLuint {
    ElementBuffer = std::numeric_limits<GLuint>::max(),
    VertexAttributes = 0,
    InstanceAttributes = 1,
};

constexpr size_t bufferBindingCount = magic_enum::enum_count<BindingIndex>();

template <typename Vertex>
void setVertexAttributes(GLuint vao, GLuint& nextLocation);

template <typename Instance>
void setInstanceAttributes(GLuint vao, GLuint& nextLocation);

template <typename Vertex, typename Instance>
class VertexArray {
   public:
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) = delete;
    VertexArray& operator=(VertexArray&& other) = delete;

   private:
    friend class MeshPack<Vertex>;
    template <typename, typename, typename>
    friend class StaticBatch;
    template <typename, typename, typename>
    friend class DynamicDrawMap;
    template <typename, typename, typename>
    friend class AnimatedDrawMap;

    static VertexArray& getVertexArray() noexcept {
        static auto vertexArray = VertexArray<Vertex, Instance>();
        return vertexArray;
    }

    void bind() {
        if (VertexArrayStorage::currentVertexArray != vao) {
            glBindVertexArray(vao);
            VertexArrayStorage::currentVertexArray = vao;
        }
    }

    template <auto Binding>
    inline constexpr size_t getAttributeSize() {
        static_assert(std::is_same_v<decltype(Binding), BindingIndex>);
        if constexpr (Binding == BindingIndex::VertexAttributes) {
            return sizeof(Vertex);
        } else if constexpr (Binding == BindingIndex::InstanceAttributes) {
            return sizeof(Instance);
        } else {
            static_assert(false, "Invalid BindingIndex for non-element buffer");
        }
    }

    template <auto Binding>
    void bindBuffer(BindingInfo bufferBinding) {
        static_assert(std::is_same_v<decltype(Binding), BindingIndex>);
        if constexpr (Binding == BindingIndex::ElementBuffer) {
            if (currentElementBuffer != bufferBinding.buffer) {
                glVertexArrayElementBuffer(vao, bufferBinding.buffer);
                currentElementBuffer = bufferBinding.buffer;
            }
        } else {
            constexpr auto bindingIndex = magic_enum::enum_underlying(Binding);
            if (currentBufferBinding[bindingIndex] != bufferBinding) {
                glVertexArrayVertexBuffer(
                    vao, bindingIndex, bufferBinding.buffer,
                    bufferBinding.offset, getAttributeSize<Binding>());
                currentBufferBinding[bindingIndex] = bufferBinding;
            }
        }
    }

    VertexArray() {
        GLuint nextLocation{0};
        glCreateVertexArrays(1, &vao);
        setVertexAttributes<Vertex>(vao, nextLocation);
        setInstanceAttributes<Instance>(vao, nextLocation);

        VertexArrayStorage::vertexArrayStorage.emplace_back(vao);
    };

    GLuint vao{0};
    GLuint currentVertexBuffer{0};
    GLuint currentElementBuffer{0};
    BindingInfo currentBufferBinding[bufferBindingCount] = {};
};

template <>
inline void setInstanceAttributes<glm::mat4>(GLuint vao, GLuint& nextLocation) {
    constexpr auto bindingIndex =
        magic_enum::enum_integer(BindingIndex::InstanceAttributes);

    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              0 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              1 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              2 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              3 * sizeof(glm::vec4));
    glEnableVertexArrayAttrib(vao, nextLocation++);
    glVertexArrayBindingDivisor(vao, bindingIndex, 1);
}

template <>
inline void setVertexAttributes<ColoredVertex>(GLuint vao,
                                               GLuint& nextLocation) {
    constexpr auto bindingIndex =
        magic_enum::enum_integer(BindingIndex::VertexAttributes);

    glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                              offsetof(ColoredVertex, position));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);
    glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                              offsetof(ColoredVertex, color));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);
}

template <>
inline void setVertexAttributes<UnlitVertex>(GLuint vao, GLuint& nextLocation) {
    constexpr auto bindingIndex =
        magic_enum::enum_integer(BindingIndex::VertexAttributes);

    glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                              offsetof(UnlitVertex, position));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribFormat(vao, nextLocation, 2, GL_FLOAT, GL_FALSE,
                              offsetof(UnlitVertex, texCoord));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);
}

template <>
inline void setVertexAttributes<UnlitAnimatedVertex>(GLuint vao,
                                                     GLuint& nextLocation) {
    constexpr auto bindingIndex =
        magic_enum::enum_integer(BindingIndex::VertexAttributes);

    glVertexArrayAttribFormat(vao, nextLocation, 3, GL_FLOAT, GL_FALSE,
                              offsetof(UnlitAnimatedVertex, position));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribFormat(vao, nextLocation, 2, GL_FLOAT, GL_FALSE,
                              offsetof(UnlitAnimatedVertex, texCoord));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribIFormat(vao, nextLocation, 4, GL_UNSIGNED_INT,
                               offsetof(UnlitAnimatedVertex, joints));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);

    glVertexArrayAttribFormat(vao, nextLocation, 4, GL_FLOAT, GL_FALSE,
                              offsetof(UnlitAnimatedVertex, weights));
    glVertexArrayAttribBinding(vao, nextLocation, bindingIndex);
    glEnableVertexArrayAttrib(vao, nextLocation++);
}
