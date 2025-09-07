#pragma once

#include "collections/slot_map/static.h"

template <typename Vertex>
class MeshPack;

template <typename Vertex>
using MeshPackHandle = StaticHandle<MeshPack<Vertex>, Shared>;

template <typename Vertex>
inline MeshPackHandle<Vertex> registerMeshPack(
    MeshPack<Vertex>&& mesh) noexcept {
    return registerResource<MeshPack<Vertex>, Shared>(std::move(mesh));
}

template <typename Key, typename Vertex>
inline const PinRef<MeshPack<Vertex>> getMeshPackByKey(
    const Key& key) noexcept {
    return getKey<Key, MeshPack<Vertex>, Shared>(key);
}

template <typename Key, typename Vertex>
inline MeshPackHandle<Vertex> tryGetOwnedMeshPackByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, MeshPack<Vertex>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Vertex>
inline PinRef<MeshPack<Vertex>> getMeshPackByKey(const Key& key) noexcept {
    return getKey<Key, MeshPack<Vertex>, Shared>(key);
}
}  // namespace unsafe
