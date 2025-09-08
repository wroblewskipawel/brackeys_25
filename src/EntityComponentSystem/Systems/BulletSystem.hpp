#pragma once
#include "../ECS.hpp"

inline void bulletSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<BulletComponent>().andHas<MovableComponent>().get();
    for (const auto& entity : entities) {
        auto& [dx, dy, speed, acceleration] = *ecs.getComponent<MovableComponent>(entity);
        auto& [angle, distance, traveledDistance] = *ecs.getComponent<BulletComponent>(entity);

        if (dx == 0.0f && dy == 0.0f) {
            dx = std::cos(glm::radians(angle)) * speed;
            dy = std::sin(glm::radians(angle)) * speed;
        }
        else {
            dx += std::cos(glm::radians(angle)) * acceleration * deltaTime;
            dy += std::sin(glm::radians(angle)) * acceleration * deltaTime;
        }

        traveledDistance += std::sqrt(((dx * deltaTime) * (dx * deltaTime)) + ((dy * deltaTime) * (dy * deltaTime)));
        if (traveledDistance >= distance) {
            ecs.addComponent(entity, RemoveComponent{});
        }
    }
}
