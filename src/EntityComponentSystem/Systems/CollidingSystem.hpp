#pragma once
#include "../ECS.hpp"
#include "QuadTree.hpp"

inline bool collide(const float& aX, const float& aY, const float& aR,
                    const float& bX, const float& bY, const float& bR) {
    return (bX - aX) * (bX - aX) + (bY - aY) * (bY - aY) < (aR + bR) * (aR + bR);
}

constexpr float repulsive_force = 3.f;

inline void collidingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<HitBoxComponent>().andHas<PositionComponent>().get();

    AABB worldBounds{0.f, 0.f, 500.f, 500.f};
    QuadTree quadTree(worldBounds);

    for (auto entity : entities) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
#ifndef NDEBUG
        if (!quadTree.insert(entity, pos->x, pos->y)) {
            std::cout << "Entity " << entity << " out of world bounds" << std::endl;
        }
#else
        quadTree.insert(entity, pos->x, pos->y);
#endif
    }

    for (auto entity : entities) {
        auto pos = ecs.getComponent<PositionComponent>(entity);
        auto col = ecs.getComponent<HitBoxComponent>(entity);

        AABB range{pos->x, pos->y, col->r, col->r};
        std::vector<EntityID> candidates;
        quadTree.query(range, candidates);

        for (auto other : candidates) {
            if (other == entity) continue;

            auto posB = ecs.getComponent<PositionComponent>(other);
            auto colB = ecs.getComponent<HitBoxComponent>(other);

            if (collide(pos->x, pos->y, col->r, posB->x, posB->y, colB->r)) {
                col->collidedWith.push_back(other);
                colB->collidedWith.push_back(entity);
            }
        }
    }
}
