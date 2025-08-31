#pragma once
#include "../ECS.hpp"
#include "../../InputHandler/InputHandler.hpp"

inline void playerMovementSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<PlayerMovementComponent>().andHas<MovableComponent>().get();
    for (const auto& entity : entities) {
        auto mComponent = ecs.getComponent<MovableComponent>(entity);

        auto& [dx, dy, speed, acceleration] = *mComponent;

        if (gInputHandler.isPressed(Key::W) ^ gInputHandler.isPressed(Key::S)) {
            if (gInputHandler.isPressed(Key::W)) {
                dy += -acceleration * speed * deltaTime;
            }
            if (gInputHandler.isPressed(Key::S)) {
                dy += acceleration * speed * deltaTime;
            }
        }
        else
            dy *= 1 - (acceleration * deltaTime);

        if (gInputHandler.isPressed(Key::A) ^ gInputHandler.isPressed(Key::D)) {
            if (gInputHandler.isPressed(Key::A)) {
                dx += acceleration * speed * deltaTime;
            }
            if (gInputHandler.isPressed(Key::D)) {
                dx += -acceleration * speed * deltaTime;
            }
        }
        else
            dx *= 1 - (acceleration * deltaTime);
    }
}
