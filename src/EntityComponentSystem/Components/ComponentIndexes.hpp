#ifndef COMPONENTINDEXES_HPP
#define COMPONENTINDEXES_HPP

#include "MovableComponent.hpp"

enum class ComponentType : size_t {
    MovableComponent = 0,
    COUNT
};

template<typename T>
struct ComponentToType;

template<> struct ComponentToType<MovableComponent> {static constexpr ComponentType index = ComponentType::MovableComponent; };

constexpr size_t COMPONENT_COUNT = static_cast<size_t>(ComponentType::COUNT);

#endif //COMPONENTINDEXES_HPP
