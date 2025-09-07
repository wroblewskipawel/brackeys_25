#pragma once

#include "collections/slot_map/static.h"

template <typename Vertex>
struct MeshData;

template <typename Vertex>
using MeshDataHandle = StaticHandle<MeshData<Vertex>, Shared>;

template <typename Vertex>
inline MeshDataHandle<Vertex> registerMeshData(
    MeshData<Vertex>&& mesh) noexcept {
    return registerResource<MeshData<Vertex>, Shared>(std::move(mesh));
}

template <typename Key, typename Vertex>
inline const PinRef<MeshData<Vertex>> getMeshDataByKey(
    const Key& key) noexcept {
    return getKey<Key, MeshData<Vertex>, Shared>(key);
}

template <typename Key, typename Vertex>
inline MeshDataHandle<Vertex> tryGetOwnedMeshDataByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, MeshData<Vertex>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Vertex>
inline PinRef<MeshData<Vertex>> getMeshDataByKey(const Key& key) noexcept {
    return getKey<Key, MeshData<Vertex>, Shared>(key);
}
}  // namespace unsafe
