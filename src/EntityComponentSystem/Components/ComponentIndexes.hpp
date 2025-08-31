#pragma once

#include "CollidingComponent.hpp"
#include "MovableComponent.hpp"
#include "PositionComponent.hpp"
#include "RenderableComponent.hpp"
#include "PlayerMovementComponent.hpp"

enum class ComponentType : size_t {
    MovableComponent = 0,
    RenderableUnlit,
    RenderableColored,
    CollidingComponent,
    PositionComponent,
    PlayerMovementComponent,
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

template <>
struct ComponentToType<CollidingComponent> {
    static constexpr ComponentType index = ComponentType::CollidingComponent;
};

template <>
struct ComponentToType<PositionComponent> {
    static constexpr ComponentType index = ComponentType::PositionComponent;
};

template <>
struct ComponentToType<PlayerMovementComponent> {
    static constexpr ComponentType index = ComponentType::PlayerMovementComponent;
};

constexpr size_t COMPONENT_COUNT = static_cast<size_t>(ComponentType::COUNT);
