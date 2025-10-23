#pragma once

#include <type_traits>
#include <variant>

#include "collections/slot_map.h"
#include "collections/unique_list.h"
#include "collections/unique_list/vector_list.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/mesh.h"

template <typename PackHandle>
struct PackItemIndex {
    uint32_t packItemIndex;
    PackHandle packHandle;

    static PackItemIndex getInvalid() noexcept {
        return PackItemIndex{
            .packItemIndex = handle::invalidValue,
            .packHandle = PackHandle::getInvalid(),
        };
    }

    bool isValid() const noexcept { return !packHandle.isInvalid(); }

    PackItemIndex copy() const noexcept {
        return PackItemIndex{.packItemIndex = packItemIndex,
                             .packHandle = packHandle.copy()};
    }

    friend bool operator==(const PackItemIndex& lhs,
                           const PackItemIndex& rhs) noexcept {
        return lhs.packItemIndex == rhs.packItemIndex &&
               lhs.packHandle == rhs.packHandle;
    }

    friend bool operator<(const PackItemIndex& lhs,
                          const PackItemIndex& rhs) noexcept {
        return std::tie(lhs.packHandle, lhs.packItemIndex) <
               std::tie(rhs.packHandle, rhs.packItemIndex);
    }
};

template <typename Vertex, typename Material>
struct PackHandles {
    // Here owned handles are created via .copy() call
    // this causes incrementation of reference count, which most often that
    // now would not be necessary - this structure main intention is to
    // index hash map for the the Model draw call insertion.
    // Copying the handles would be only necessary if this is the first
    // model using given pack handles being added to the draw call map
    // TODO: Introduce model->packHandles map index matching mechanis that
    // do not require to copy the handles
    PackHandles(const MeshPackHandle<Vertex>& meshPackHandle,
                const MaterialPackHandle<Material>& materialPackHandle) noexcept
        : meshPackHandle(meshPackHandle.copy()),
          materialPackHandle(materialPackHandle.copy()) {}

    PackHandles(const PackHandles&) = delete;
    PackHandles& operator=(const PackHandles&) = delete;

    PackHandles(PackHandles&&) = default;
    PackHandles& operator=(PackHandles&&) = default;

    friend class std::hash<PackHandles>;

    friend bool operator==(const PackHandles& lhs,
                           const PackHandles& rhs) noexcept {
        return lhs.meshPackHandle == rhs.meshPackHandle &&
               lhs.materialPackHandle == rhs.materialPackHandle;
    }

    friend bool operator<(const PackHandles& lhs,
                          const PackHandles& rhs) noexcept {
        return std::tie(lhs.meshPackHandle, lhs.materialPackHandle) <
               std::tie(rhs.meshPackHandle, rhs.materialPackHandle);
    }

    void bind() const noexcept {
        MeshPack<Vertex>::bind(meshPackHandle);
        if constexpr (!std::is_same_v<Material, EmptyMaterial>) {
            MaterialPack<Material>::bind(materialPackHandle);
        }
    };

    PackHandles copy() const noexcept {
        return PackHandles(meshPackHandle, materialPackHandle);
    }

    MeshPackHandle<Vertex> meshPackHandle;
    MaterialPackHandle<Material> materialPackHandle;
};

namespace std {
template <typename Vertex, typename Material>
struct hash<PackHandles<Vertex, Material>> {
    std::size_t operator()(
        const PackHandles<Vertex, Material>& packHandles) const noexcept {
        std::size_t h1 =
            std::hash<MeshPackHandle<Vertex>>{}(packHandles.meshPackHandle);
        std::size_t h2 = std::hash<MaterialPackHandle<Material>>{}(
            packHandles.materialPackHandle);
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

template <typename Vertex>
using MeshHandle = PackItemIndex<MeshPackHandle<Vertex>>;

template <typename Material>
using MaterialHandle = PackItemIndex<MaterialPackHandle<Material>>;

template <typename Vertex, typename Material>
struct Model {
    using MeshHandle = MeshHandle<Vertex>;
    using MaterialHandle = MaterialHandle<Material>;
    using PackHandles = PackHandles<Vertex, Material>;

    MeshHandle mesh;
    MaterialHandle material;

    static Model getInvalid() noexcept {
        return Model{
            .mesh = MeshHandle::getInvalid(),
            .material = MaterialHandle::getInvalid(),
        };
    }

    bool isValid() const noexcept {
        if constexpr (std::is_same_v<Material, EmptyMaterial>) {
            return mesh.isValid();
        } else {
            return mesh.isValid() && material.isValid();
        }
    }

    PackHandles getPackHandles() const noexcept {
        return PackHandles(mesh.packHandle, material.packHandle);
    }

    MeshOffsets getMeshOffsets() const noexcept {
        return mesh.packHandle.get().get().getMeshOffsets(mesh.packItemIndex);
    }

    uint32_t getMaterialIndex() const noexcept {
        return material.packItemIndex;
    }

    friend bool operator==(const Model& lhs, const Model& rhs) noexcept {
        return lhs.mesh == rhs.mesh && lhs.material == rhs.material;
    }

    friend bool operator<(const Model& lhs, const Model& rhs) noexcept {
        return std::tie(lhs.mesh, lhs.material) <
               std::tie(rhs.mesh, rhs.material);
    }
};
