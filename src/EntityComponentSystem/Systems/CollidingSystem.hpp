#pragma once
#include "../ECS.hpp"
#include "QuadTree.hpp"

inline bool collide(const float& aX, const float& aY, const float& aR,
                    const float& bX, const float& bY, const float& bR) {
    return (bX - aX) * (bX - aX) + (bY - aY) * (bY - aY) < (aR + bR) * (aR + bR);
}

constexpr float repulsive_force = 3.f;

inline void collidingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<CollidingComponent>().andHas<PositionComponent>().get();

    AABB worldBounds{0.f, 0.f, 500.f, 500.f};
    QuadTree quadTree(worldBounds);

    for (auto entity : entities) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
        if (pos) {
            quadTree.insert(entity, pos->x, pos->y);
        }
    }

    for (auto entity : entities) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
        auto col = ecs.getComponent<CollidingComponent>(entity);

        AABB range{pos->x, pos->y, col->r, col->r};
        std::vector<EntityID> candidates;
        quadTree.query(range, candidates);

        for (auto other : candidates) {
            if (other == entity) continue;

            auto posB = ecs.getComponent<PositionComponent>(other);
            auto colB = ecs.getComponent<CollidingComponent>(other);

            if (collide(pos->x, pos->y, col->r, posB->x, posB->y, colB->r)) {
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

                if (auto mA = ecs.getComponent<MovableComponent>(entity)) {
                    pos->x -= nx * pushA;
                    pos->y -= ny * pushA;
                }

                if (auto mB = ecs.getComponent<MovableComponent>(other)) {
                    posB->x += nx * pushB;
                    posB->y += ny * pushB;
                }
            }
        }
    }
}
