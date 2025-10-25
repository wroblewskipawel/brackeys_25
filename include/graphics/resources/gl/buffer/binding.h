#pragma once

#include <glad/glad.h>

#include <array>
#include <limits>
#include <magic_enum.hpp>
#include <vector>

enum class BufferBindings {
    Storage,
    Uniform,
};

template <auto Binding>
inline constexpr GLenum getBufferBindingTarget() {
    static_assert(std::is_same_v<decltype(Binding), BufferBindings>);
    if constexpr (Binding == BufferBindings::Storage) {
        return GL_SHADER_STORAGE_BUFFER;
    } else if constexpr (Binding == BufferBindings::Uniform) {
        return GL_UNIFORM_BUFFER;
    } else {
        static_assert(false, "Unsupported BufferBinding variatnt");
    }
}

template <auto Binding>
inline constexpr size_t getMaxBindingIndices() {
    static_assert(std::is_same_v<decltype(Binding), BufferBindings>);
    GLenum targetName = GL_NONE;
    GLint maxBindings = 0;
    if constexpr (Binding == BufferBindings::Storage) {
        targetName = GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS;
    } else if constexpr (Binding == BufferBindings::Uniform) {
        targetName = GL_MAX_UNIFORM_BUFFER_BINDINGS;
    } else {
        static_assert(false, "Unsupported BufferBinding variatnt");
    }
    glGetIntegerv(targetName, &maxBindings);
    return static_cast<size_t>(maxBindings);
}

class BindingState {
   public:
    BindingState(const BindingState&) = delete;
    BindingState& operator=(const BindingState&) = delete;

    BindingState(BindingState&&) = delete;
    BindingState& operator=(BindingState&&) = delete;

    ~BindingState() = default;

    template <auto Binding>
    static void bindBuffer(GLuint buffer, GLuint bindingIndex) noexcept {
        auto& targetState = getBindingTargetState<Binding>();
        if (bindingIndex >= targetState.size()) {
            std::println(std::cerr,
                         "BindingState::bindBuffer: Invalid bindingIndex for "
                         "buffer binding target {}",
                         magic_enum::enum_name(Binding));
            std::abort();
        }
        if (targetState[bindingIndex] != buffer) {
            glBindBufferBase(getBufferBindingTarget<Binding>(), bindingIndex,
                             buffer);
            targetState[bindingIndex] = buffer;
        }
    }

   private:
    static auto& getBindingState() noexcept {
        static BindingState bindingState{};
        return bindingState;
    }

    template <auto Binding>
    static auto& getBindingTargetState() noexcept {
        static_assert(std::is_same_v<decltype(Binding), BufferBindings>);
        auto& bindingState = getBindingState();
        return bindingState.bindings[magic_enum::enum_integer(Binding)];
    }

    BindingState() noexcept {
        initializeBindingArray<BufferBindings::Storage>();
        initializeBindingArray<BufferBindings::Uniform>();
    }

    template <auto Binding>
    void initializeBindingArray() noexcept {
        static_assert(std::is_same_v<decltype(Binding), BufferBindings>);
        auto& bindingIndices = bindings[magic_enum::enum_integer(Binding)];
        bindingIndices.resize(getMaxBindingIndices<Binding>(), 0);
    }

    std::array<std::vector<GLuint>, magic_enum::enum_count<BufferBindings>()>
        bindings;
};
