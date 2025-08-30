#pragma once

#include <gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include "material.h"
#include "mesh.h"

template <typename Vertex, typename Material>
struct DrawCommand {
    Mesh mesh;
    size_t materialIndex;
    glm::mat4 modelMatrix;
};

template <typename Vertex, typename Material>
struct DrawCommandPartial {
    Mesh mesh;
    size_t materialIndex;

    DrawCommand<Vertex, Material> withTransform(const glm::mat4& modelMatrix) {
        return DrawCommand<Vertex, Material>{mesh, materialIndex, modelMatrix};
    }
};

template <typename Vertex, typename Material>
struct DrawCommandBuilder {
   public:
    DrawCommandBuilder(const MeshPack<Vertex>& meshPack,
                       const MaterialPack<Material>& materialPack)
        : meshPack{meshPack}, materialPack{materialPack} {}

    DrawCommandPartial<Vertex, Material> getCommandPartial(
        MeshHandle<Vertex> meshHadle, MaterialHandle<Material> materialHandle) {
        return DrawCommandPartial<Vertex, Material> {
            meshPack.getMesh(meshHadle), materialHandle.uniformIndex
        };
    }

   private:
    const MeshPack<Vertex>& meshPack;
    const MaterialPack<Material>& materialPack;
};

template <typename Vertex, typename Material>
class DynamicDrawPack {
   public:
    using DrawQueue = std::vector<DrawCommand<Vertex, Material>>;

    DynamicDrawPack(const MeshPack<Vertex>& meshPack,
                    const MaterialPack<Material>& materialPack)
        : materialPackRef{materialPack.getPackRef()},
          buffers{meshPack.buffers},
          vao{getVertexArray<Vertex>()},
          drawQueue{std::make_shared<DrawQueue>()} {};

    std::shared_ptr<DrawQueue> getDrawQueue() const { return drawQueue; }

    void draw(const UniformLocations& uniformLocations) {
        glBindVertexArray(vao);
        glVertexArrayVertexBuffer(vao, 0, buffers.vbo, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(vao, buffers.ebo);
        MaterialPack<Material>::bind(materialPackRef);
        for (const auto& drawCommand : *drawQueue) {
            glUniform1ui(uniformLocations.materialIndex,
                         static_cast<GLuint>(drawCommand.materialIndex));
            glUniformMatrix4fv(uniformLocations.modelMatrix, 1, GL_FALSE,
                               glm::value_ptr(drawCommand.modelMatrix));
            glDrawElements(
                GL_TRIANGLES, drawCommand.mesh.indexCount, GL_UNSIGNED_INT,
                (void*)(drawCommand.mesh.indexOffset * sizeof(GLuint)));
        }
        drawQueue->clear();
    }

   private:
    std::shared_ptr<DrawQueue> drawQueue;

    MaterialPackRef<Material> materialPackRef;
    MeshBuffers buffers;
    GLuint vao;
};
