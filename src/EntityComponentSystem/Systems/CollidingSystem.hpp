#pragma once
#include "../ECS.hpp"

inline bool collide(const float& aX, const float& aY, const float& aR,
                    const float& bX, const float& bY, const float& bR) {
    return (bX - aX) * (bX - aX) + (bY - aY) * (bY - aY) < (aR + bR) * (aR + bR);
}

constexpr float repulsive_force = 3.f;

inline void collidingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<CollidingComponent>().andHas<PositionComponent>().get();

    for (auto it1 = entities.begin(); it1 != entities.end(); ++it1) {
        auto it2 = it1;
        ++it2;

        for (; it2 != entities.end(); ++it2) {
            const auto& entityA = *it1;
            const auto& entityB = *it2;
            auto ApComponent = ecs.getComponent<PositionComponent>(entityA);
            auto AcComponent = ecs.getComponent<CollidingComponent>(entityA);
            auto& [Ax, Ay, Az] = *ApComponent;
            auto& [Ar] = *AcComponent;

            auto BpComponent = ecs.getComponent<PositionComponent>(entityB);
            auto BcComponent = ecs.getComponent<CollidingComponent>(entityB);
            auto& [Bx, By, Bz] = *BpComponent;
            auto& [Br] = *BcComponent;

            if (collide(Ax, Ay, Ar, Bx, By, Br)) {
                float dx = Bx - Ax;
                float dy = By - Ay;
                float distSq = dx*dx + dy*dy;
                float dist = std::sqrt(distSq);

                if (dist == 0.f) {
                    dx = 1.f;
                    dy = 0.f;
                    dist = 1.f;
                }

                float overlap = (Ar + Br - dist);

                float nx = dx / dist;
                float ny = dy / dist;

                float pushA = 0.5f * overlap;
                float pushB = 0.5f * overlap;

                auto AmComponent = ecs.getComponent<MovableComponent>(entityA);
                if (AmComponent != nullptr) {
                    Ax -= nx * pushA;
                    Ay -= ny * pushA;
                }

                auto BmComponent = ecs.getComponent<MovableComponent>(entityB);
                if (BmComponent != nullptr) {
                    Bx += nx * pushB;
                    By += ny * pushB;
                }
            }
        }
    }
}
