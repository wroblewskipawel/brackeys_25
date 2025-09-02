#pragma once
#include "../ECS.hpp"

inline void removeEntitySystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<RemoveComponent>().get();

    for (const auto& entity : entities) {
        ecs.removeEntity(entity);
    }
}
