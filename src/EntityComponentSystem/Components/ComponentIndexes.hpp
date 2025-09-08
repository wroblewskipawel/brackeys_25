#pragma once

#include "HitBoxComponent.hpp"
#include "MovableComponent.hpp"
#include "PlayerMovementComponent.hpp"
#include "CollidingComponent.hpp"
#include "PositionComponent.hpp"
#include "RenderableComponent.hpp"
#include "CoinComponent.hpp"
#include "RemoveComponent.hpp"
#include "FollowPlayerComponent.hpp"
#include "BulletComponent.hpp"

enum class ComponentType : size_t {
    MovableComponent = 0,
    RenderableUnlit,
    RenderableColored,
    HitBoxComponent,
    PositionComponent,
    PlayerMovementComponent,
    CollidingComponent,
    CoinComponent,
    RemoveComponent,
    FollowPlayerComponent,
    BulletComponent,
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

template <>
struct ComponentToType<CoinComponent> {
    static constexpr ComponentType index = ComponentType::CoinComponent;
};

template <>
struct ComponentToType<RemoveComponent> {
    static constexpr ComponentType index = ComponentType::RemoveComponent;
};

template <>
struct ComponentToType<FollowPlayerComponent> {
    static constexpr ComponentType index = ComponentType::FollowPlayerComponent;
};

template <>
struct ComponentToType<BulletComponent> {
    static constexpr ComponentType index = ComponentType::BulletComponent;
};

constexpr size_t COMPONENT_COUNT = static_cast<size_t>(ComponentType::COUNT);
