#pragma once

#include "collections/slot_map/static.h"

template <typename Material>
class MaterialPack;

template <typename Material>
using MaterialPackHandle = StaticHandle<MaterialPack<Material>, Shared>;

template <typename Material>
inline MaterialPackHandle<Material> registerMaterialPack(
    MaterialPack<Material>&& mesh) noexcept {
    return registerResource<MaterialPack<Material>, Shared>(std::move(mesh));
}

template <typename Key, typename Material>
inline const PinRef<MaterialPack<Material>> getMaterialPackByKey(
    const Key& key) noexcept {
    return getKey<Key, MaterialPack<Material>, Shared>(key);
}

template <typename Key, typename Material>
inline MaterialPackHandle<Material> tryGetOwnedMaterialPackByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, MaterialPack<Material>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Material>
inline PinRef<MaterialPack<Material>> getMaterialPackByKey(
    const Key& key) noexcept {
    return getKey<Key, MaterialPack<Material>, Shared>(key);
}

}  // namespace unsafe
