#pragma once

#include "collections/slot_map/static.h"

template <typename Material>
class MaterialBuilder;

template <typename Material>
using MaterialBuilderHandle = StaticHandle<MaterialBuilder<Material>, Shared>;

template <typename Material>
inline MaterialBuilderHandle<Material> registerMaterialBuilder(
    MaterialBuilder<Material>&& materialBuilder) noexcept {
    return registerResource<MaterialBuilder<Material>, Shared>(
        std::move(materialBuilder));
}

template <typename Key, typename Material>
inline const PinRef<MaterialBuilder<Material>> getMaterialBuilderByKey(
    const Key& key) noexcept {
    return getKey<Key, MaterialBuilder<Material>, Shared>(key);
}

template <typename Key, typename Material>
inline MaterialBuilderHandle<Material> tryGetOwnedMaterialBuilderByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, MaterialBuilder<Material>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Material>
inline PinRef<MaterialBuilder<Material>> getMaterialBuilderByKey(
    const Key& key) noexcept {
    return getKey<Key, MaterialBuilder<Material>, Shared>(key);
}

}  // namespace unsafe
