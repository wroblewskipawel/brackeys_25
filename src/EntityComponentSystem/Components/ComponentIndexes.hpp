#pragma once

#include "MovableComponent.hpp"
#include "RenderableComponent.hpp"

enum class ComponentType : size_t {
    MovableComponent = 0,
    RenderableUnlit = 1,
    RenderableColored = 2,
    COUNT
};

template <typename T>
struct ComponentToType;

template <>
struct ComponentToType<MovableComponent> {
    static constexpr ComponentType index = ComponentType::MovableComponent;
};

template <>
struct ComponentToType<RenderableUnlit> {
    static constexpr ComponentType index = ComponentType::RenderableUnlit;
};
template <>
struct ComponentToType<RenderableColored> {
    static constexpr ComponentType index = ComponentType::RenderableColored;
};

constexpr size_t COMPONENT_COUNT = static_cast<size_t>(ComponentType::COUNT);
