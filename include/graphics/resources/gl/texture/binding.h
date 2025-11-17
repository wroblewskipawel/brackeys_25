#pragma once

#include <glad/glad.h>

#include <array>
#include <cstdint>
#include <magic_enum.hpp>
#include <vector>
#include <iostream>

enum class TextureBinding: uint8_t {
    Array,
    Texture2D,
};

inline size_t getMaxTextureUnits() {
    static GLint maxUnits = 0;
    if (maxUnits == 0) {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
    }
    return static_cast<size_t>(maxUnits);
};

class TextureUnitState {
   public:
    TextureUnitState(const TextureUnitState&) = delete;
    TextureUnitState& operator=(const TextureUnitState&) = delete;

    TextureUnitState(TextureUnitState&&) = delete;
    TextureUnitState& operator=(TextureUnitState&&) = delete;

    ~TextureUnitState() = default;

    template <TextureBinding Binding>
    static void bindTexture(GLuint texture, GLuint unitIndex) noexcept {
        auto& targetState = getUnitTargetState<Binding>();
        if (unitIndex >= targetState.size()) {
            std::println(std::cerr,
                         "TextureUnitState::bindTexture: Invalid unitIndex");
            std::abort();
        }
        if (targetState[unitIndex] != texture) {
            glBindTextureUnit(unitIndex, texture);
            targetState[unitIndex] = texture;
        }
    }

   private:
    static auto& getUnitState() noexcept {
        static TextureUnitState textureUnitState{};
        return textureUnitState;
    }

    template <TextureBinding Binding>
    static auto& getUnitTargetState() noexcept {
        auto& unitState = getUnitState();
        return unitState.bindings[magic_enum::enum_integer(Binding)];
    }

    TextureUnitState() noexcept {
        initializeBindingArray<TextureBinding::Array>();
        initializeBindingArray<TextureBinding::Texture2D>();
    }

    template <TextureBinding Binding>
    void initializeBindingArray() noexcept {
        auto& bindingUnits = bindings[magic_enum::enum_integer(Binding)];
        bindingUnits.resize(getMaxTextureUnits(), 0);
    }

    std::array<std::vector<GLuint>, magic_enum::enum_count<TextureBinding>()>
        bindings;
};
