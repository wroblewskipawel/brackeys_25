#pragma once

#include <type_traits>
#include <variant>

#include "collections/slot_map.h"
#include "collections/unique_list.h"
#include "collections/unique_list/vector_list.h"
#include "graphics/resources/gl/draw.h"
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

template <typename Vertex>
using MeshHandle = PackItemIndex<MeshPackHandle<Vertex>>;

template <typename Material>
using MaterialHandle = PackItemIndex<MaterialPackHandle<Material>>;

template <typename Vertex, typename Material>
struct Model {
    using MeshHandle = MeshHandle<Vertex>;
    using MaterialHandle = MaterialHandle<Material>;

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

    friend bool operator==(const Model& lhs, const Model& rhs) noexcept {
        return lhs.mesh == rhs.mesh && lhs.material == rhs.material;
    }

    friend bool operator<(const Model& lhs, const Model& rhs) noexcept {
        return std::tie(lhs.mesh, lhs.material) <
               std::tie(rhs.mesh, rhs.material);
    }
};

template <typename Vertex, typename Material, typename Instance>
struct DrawDataInstanced {
    Model<Vertex, Material> model;
    std::vector<Instance> instanceData;
};
