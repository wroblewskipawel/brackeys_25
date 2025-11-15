#pragma once

#include <map>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include "collections/slot_map.h"
#include "collections/unique_list.h"
#include "collections/unique_list/vector_list.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/shader/uniform.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/mesh.h"
#include "utility/hash.h"


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
    PackHandles(MeshPackHandle<Vertex>&& meshPackHandle,
                MaterialPackHandle<Material>&& materialPackHandle) noexcept
        : meshPackHandle(std::move(meshPackHandle)),
          materialPackHandle(std::move(materialPackHandle)) {}

    PackHandles(const PackHandles&) = delete;
    PackHandles& operator=(const PackHandles&) = delete;

    PackHandles(PackHandles&&) = default;
    PackHandles& operator=(PackHandles&&) = default;

    void bind(const UniformLocations& uniformLocations) const noexcept {
        MeshPack<Vertex>::bind(meshPackHandle);
        if constexpr (!EmptyMaterialType<Material>) {
            MaterialPack<Material>::bind(materialPackHandle, uniformLocations);
        }
    };

    PackHandles copy() const noexcept {
        return PackHandles(meshPackHandle.copy(), materialPackHandle.copy());
    }

    MeshPackHandle<Vertex> meshPackHandle;
    MaterialPackHandle<Material> materialPackHandle;
};

template <typename Vertex, typename Material>
struct PackHandlesView {
    PackHandlesView(
        const MeshPackHandle<Vertex>& meshPackHandle,
        const MaterialPackHandle<Material>& materialPackHandle) noexcept
        : meshPackHandle(meshPackHandle),
          materialPackHandle(materialPackHandle) {}

    PackHandlesView(const PackHandles<Vertex, Material>& packHandles) noexcept
        : meshPackHandle(packHandles.meshPackHandle),
          materialPackHandle(packHandles.materialPackHandle) {}

    PackHandlesView(const PackHandlesView&) = delete;
    PackHandlesView& operator=(const PackHandlesView&) = delete;

    PackHandlesView(PackHandlesView&&) = delete;
    PackHandlesView& operator=(PackHandlesView&&) = delete;

    friend bool operator==(const PackHandlesView& lhs,
                           const PackHandlesView& rhs) noexcept {
        return lhs.meshPackHandle == rhs.meshPackHandle &&
               lhs.materialPackHandle == rhs.materialPackHandle;
    }

    auto getOwned() const noexcept {
        return PackHandles<Vertex, Material>(meshPackHandle.copy(),
                                             materialPackHandle.copy());
    }

    void bind(const UniformLocations& uniformLocations) const noexcept {
        MeshPack<Vertex>::bind(meshPackHandle);
        if constexpr (!EmptyMaterialType<Material>) {
            MaterialPack<Material>::bind(materialPackHandle, uniformLocations);
        }
    };

    const MeshPackHandle<Vertex>& meshPackHandle;
    const MaterialPackHandle<Material>& materialPackHandle;
};

template <typename Pack>
concept PackHandleViewType = requires(Pack p) {
    p.meshPackHandle;
    p.materialPackHandle;
};

template <PackHandleViewType Handles>
std::size_t hashHandles(const Handles& handles) noexcept {
    std::size_t h1 = hashValue(handles.meshPackHandle);
    std::size_t h2 = hashValue(handles.materialPackHandle);
    return h1 ^ (h2 << 1);
}

template <PackHandleViewType Lhs, PackHandleViewType Rhs = Lhs>
bool compareHandlesEqual(const Lhs& lhs, const Rhs& rhs) noexcept {
    return lhs.meshPackHandle == rhs.meshPackHandle &&
           lhs.materialPackHandle == rhs.materialPackHandle;
}

template <PackHandleViewType Lhs, PackHandleViewType Rhs = Lhs>
bool compareHandlesLess(const Lhs& lhs, const Rhs& rhs) noexcept {
    return std::tie(lhs.meshPackHandle, lhs.materialPackHandle) <
           std::tie(rhs.meshPackHandle, rhs.materialPackHandle);
}

template <typename Vertex, typename Material>
struct PackHandlesHasher {
    using is_transparent = void;

    using PackHandles = PackHandles<Vertex, Material>;
    using PackHandlesView = PackHandlesView<Vertex, Material>;

    size_t operator()(const PackHandles& handle) const {
        return hashHandles(handle);
    }

    size_t operator()(const PackHandlesView& handle) const {
        return hashHandles(handle);
    }
};

template <typename Vertex, typename Material>
struct PackHandlesEqual {
    using is_transparent = void;

    using PackHandles = PackHandles<Vertex, Material>;
    using PackHandlesView = PackHandlesView<Vertex, Material>;

    bool operator()(const PackHandles& lhs, const PackHandles& rhs) const {
        return compareHandlesEqual(lhs, rhs);
    }

    bool operator()(const PackHandlesView& lhs,
                    const PackHandlesView& rhs) const {
        return compareHandlesEqual(lhs, rhs);
    }

    bool operator()(const PackHandlesView& lhs, const PackHandles& rhs) const {
        return compareHandlesEqual(lhs, rhs);
    }

    bool operator()(const PackHandles& lhs, const PackHandlesView& rhs) const {
        return compareHandlesEqual(lhs, rhs);
    }
};

template <typename Vertex, typename Material>
struct PackHandlesLess {
    using is_transparent = void;

    using PackHandles = PackHandles<Vertex, Material>;
    using PackHandlesView = PackHandlesView<Vertex, Material>;

    bool operator()(const PackHandles& lhs, const PackHandles& rhs) const {
        return compareHandlesLess(lhs, rhs);
    }

    bool operator()(const PackHandlesView& lhs,
                    const PackHandlesView& rhs) const {
        return compareHandlesLess(lhs, rhs);
    }

    bool operator()(const PackHandlesView& lhs, const PackHandles& rhs) const {
        return compareHandlesLess(lhs, rhs);
    }

    bool operator()(const PackHandles& lhs, const PackHandlesView& rhs) const {
        return compareHandlesLess(lhs, rhs);
    }
};

template <typename Vertex, typename Material, typename Item>
using PackUnorderedMap = std::unordered_map<PackHandles<Vertex, Material>, Item,
                                            PackHandlesHasher<Vertex, Material>,
                                            PackHandlesEqual<Vertex, Material>>;

template <typename Vertex, typename Material, typename Item>
using PackMap = std::map<PackHandles<Vertex, Material>, Item,
                         PackHandlesLess<Vertex, Material>>;

template <typename, typename>
struct PackMapTypes;

template <typename Vertex, typename Material, typename Item>
struct PackMapTypes<PackHandlesView<Vertex, Material>, Item> {
    using PackUnorderedMap = PackUnorderedMap<Vertex, Material, Item>;
    using PackMap = PackMap<Vertex, Material, Item>;
};

template <typename Vertex, typename Material, typename Item>
struct PackMapTypes<PackHandles<Vertex, Material>, Item> {
    using PackUnorderedMap = PackUnorderedMap<Vertex, Material, Item>;
    using PackMap = PackMap<Vertex, Material, Item>;
};

template <typename Vertex>
using MeshHandle = PackItemIndex<MeshPackHandle<Vertex>>;

template <typename Material>
using MaterialHandle = PackItemIndex<MaterialPackHandle<Material>>;

template <typename Vertex, typename Material>
struct Model {
    using MeshHandle = MeshHandle<Vertex>;
    using MaterialHandle = MaterialHandle<Material>;
    using PackHandlesView = PackHandlesView<Vertex, Material>;

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

    PackHandlesView getPackHandlesView() const noexcept {
        return PackHandlesView(mesh.packHandle, material.packHandle);
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
