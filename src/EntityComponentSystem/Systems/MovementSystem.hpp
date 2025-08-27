#ifndef MOVEMENTSYSTEM_HPP
#define MOVEMENTSYSTEM_HPP

#include "../ECS.hpp"
#include "../Components/MovableComponent.hpp"
#include "../../InputHandler/InputHandler.hpp"

inline void movementSystem(ECS& ecs, const float& deltaTime) {
    auto& movables = ecs.getStorage<MovableComponent>();
    for (size_t i = 0; i < movables.getAll().size(); ++i) {
        auto& [x, y, speedX, speedY] = movables.getAll()[i];

        if (gInputHandler.isPressed(Key::W)) y += speedY * deltaTime;
        if (gInputHandler.isPressed(Key::S)) y -= speedY * deltaTime;
        if (gInputHandler.isPressed(Key::A)) x -= speedX * deltaTime;
        if (gInputHandler.isPressed(Key::D)) x += speedX * deltaTime;
    }
}

#endif //MOVEMENTSYSTEM_HPP
