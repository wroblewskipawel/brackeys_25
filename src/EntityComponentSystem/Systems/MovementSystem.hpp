#pragma once
#include "../ECS.hpp"

inline void movementSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<MovableComponent>().andHas<PositionComponent>().get();
    for (const auto& entity : entities) {
        auto mComponent = ecs.getComponent<MovableComponent>(entity);
        auto pComponent = ecs.getComponent<PositionComponent>(entity);

        auto& [dx, dy, speed, acceleration] = *mComponent;
        auto& [x, y, z] = *pComponent;

        float actSpeed = std::sqrt((dx * dx) + (dy * dy));
        if (actSpeed > speed) {
            dx *= speed / actSpeed;
            dy *= speed / actSpeed;
        }
        dx *= 1 - (2*deltaTime);
        dy *= 1 - (2*deltaTime);

        x += dx * deltaTime;
        y += dy * deltaTime;
    }
}
