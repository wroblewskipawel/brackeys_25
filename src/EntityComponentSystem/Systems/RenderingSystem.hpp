#pragma once

#include <glm/glm.hpp>

#include "../ECS.hpp"
#include "../Components/MovableComponent.hpp"
#include "../Components/RenderableComponent.hpp"

inline void renderingSystem(ECS& ecs, const float& deltaTime, RenderingQueues& renderingQueues) {
    auto entities = ecs.getEntitiesWithComponent<PositionComponent>().get();
    auto player = ecs.getEntitiesWithComponent<PlayerMovementComponent>().get();

    PositionComponent* playerPos = nullptr;
    if (player.size() == 1) {
        playerPos = ecs.getComponent<PositionComponent>(*player.begin());
    }

    for (const auto& entity : entities) {
        auto position = ecs.getComponent<PositionComponent>(entity);
        auto& [x, y, z] = *position;

        if (playerPos) {
            float dx = x - playerPos->x;
            float dy = y - playerPos->y;
            float distanceXY = std::sqrt(dx * dx + dy * dy);
            if (distanceXY > 20.0f) {
                continue;
            }
        }

        auto* coloredMesh = ecs.getComponent<RenderableColored>(entity);
        if (coloredMesh != nullptr) {
            auto transform = glm::vec3(x, y, z);
            auto draw = coloredMesh->withTransform(glm::translate(glm::mat4(1.0), transform));
            renderingQueues.coloredQueue->emplace_back(draw);
            continue;
        }

        auto* unlitMesh = ecs.getComponent<RenderableUnlit>(entity);
        if (unlitMesh) {
            auto transform = glm::vec3(x, y, z);
            auto draw = unlitMesh->withTransform(glm::translate(glm::mat4(1.0), transform));
            renderingQueues.unlitQueue->emplace_back(draw);
            continue;
        }
    }
}
