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
            if (distanceXY > 50.0f) {
                continue;
            }
        }

        if (auto* coloredMesh = ecs.getComponent<RenderableColored>(entity)) {
            glm::vec3 scaleVec = coloredMesh->scale;;
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
            modelMatrix = glm::rotate(modelMatrix, coloredMesh->rotation, coloredMesh->rotation_along);
            modelMatrix = glm::scale(modelMatrix, scaleVec);
            auto draw = coloredMesh->withTransform(modelMatrix);
            renderingQueues.coloredQueue->emplace_back(draw);
            continue;
        }

        if (auto* unlitMesh = ecs.getComponent<RenderableUnlit>(entity)) {
            glm::vec3 scaleVec = unlitMesh->scale;;
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
            modelMatrix = glm::rotate(modelMatrix, unlitMesh->rotation, unlitMesh->rotation_along);
            modelMatrix = glm::scale(modelMatrix, scaleVec);
            auto draw = unlitMesh->withTransform(modelMatrix);
            renderingQueues.unlitQueue->emplace_back(draw);
            continue;
        }
    }
}
