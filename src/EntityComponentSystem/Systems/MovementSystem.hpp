#ifndef MOVEMENTSYSTEM_HPP
#define MOVEMENTSYSTEM_HPP

#include "../ECS.hpp"
#include "../Components/MovableComponent.hpp"
#include "../../InputHandler/InputHandler.hpp"

inline void movementSystem(ECS& ecs, const float& deltaTime) {
    auto entities = ecs.getEntitiesWithComponent<MovableComponent>().get();
    for (const auto& entity : entities) {
        auto component = ecs.getComponent<MovableComponent>(entity);

        auto& [x, y, speedX, speedY] = *component;

        if (gInputHandler.isPressed(Key::W)) y += speedY * deltaTime;
        if (gInputHandler.isPressed(Key::S)) y -= speedY * deltaTime;
        if (gInputHandler.isPressed(Key::A)) x -= speedX * deltaTime;
        if (gInputHandler.isPressed(Key::D)) x += speedX * deltaTime;
    }
}

#endif //MOVEMENTSYSTEM_HPP
