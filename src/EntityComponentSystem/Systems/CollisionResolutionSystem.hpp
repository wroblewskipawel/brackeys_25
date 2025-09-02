#pragma once
#include "../ECS.hpp"

inline void collisionResolutionSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<HitBoxComponent>().andHas<PositionComponent>().get();

    for (auto entity : entities) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
        auto col = ecs.getComponent<HitBoxComponent>(entity);

        for (auto other : col->collidedWith) {


            if (entity < other) {
                auto posB = ecs.getComponent<PositionComponent>(other);
                auto colB = ecs.getComponent<HitBoxComponent>(other);

                float dx = posB->x - pos->x;
                float dy = posB->y - pos->y;
                float distSq = dx*dx + dy*dy;
                float dist = std::sqrt(distSq);

                if (dist == 0.f) { dx = 1.f; dy = 0.f; dist = 1.f; }

                float overlap = (col->r + colB->r - dist);
                float nx = dx / dist;
                float ny = dy / dist;

                float pushA = 0.5f * overlap;
                float pushB = 0.5f * overlap;

                if (ecs.entityStorage.hasComponent<CollidingComponent>(entity) && ecs.entityStorage.hasComponent<CollidingComponent>(other)) {
                    if (ecs.entityStorage.hasComponent<MovableComponent>(entity)) {
                        pos->x -= nx * pushA;
                        pos->y -= ny * pushA;
                    }

                    if (ecs.entityStorage.hasComponent<MovableComponent>(other)) {
                        posB->x += nx * pushB;
                        posB->y += ny * pushB;
                    }
                }
            }
        }
        col->collidedWith.clear();
    }
}
