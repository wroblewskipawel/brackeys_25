#pragma once

#include "HitBoxComponent.hpp"
#include "MovableComponent.hpp"
#include "PlayerMovementComponent.hpp"
#include "CollidingComponent.hpp"
#include "PositionComponent.hpp"
#include "RenderableComponent.hpp"

enum class ComponentType : size_t {
    MovableComponent = 0,
    RenderableUnlit,
    RenderableColored,
    HitBoxComponent,
    PositionComponent,
    PlayerMovementComponent,
    CollidingComponent,
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
struct ComponentToType<HitBoxComponent> {
    static constexpr ComponentType index = ComponentType::HitBoxComponent;
};

template <>
struct ComponentToType<PositionComponent> {
    static constexpr ComponentType index = ComponentType::PositionComponent;
};

template <>
struct ComponentToType<PlayerMovementComponent> {
    static constexpr ComponentType index = ComponentType::PlayerMovementComponent;
};

template <>
struct ComponentToType<CollidingComponent> {
    static constexpr ComponentType index = ComponentType::CollidingComponent;
};

constexpr size_t COMPONENT_COUNT = static_cast<size_t>(ComponentType::COUNT);
