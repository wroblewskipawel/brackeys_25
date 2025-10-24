#pragma once

#include "collections/slot_map/static.h"

template <typename Vertex, typename Material, typename Instance>
class StaticPack;

template <typename Vertex, typename Material, typename Instance>
using StaticPackHandle =
    StaticHandle<StaticPack<Vertex, Material, Instance>, Shared>;

template <typename Vertex, typename Material, typename Instance>
inline StaticPackHandle<Vertex, Material, Instance> registerStaticPack(
    StaticPack<Vertex, Material, Instance>&& pack) noexcept {
    return registerResource<StaticPack<Vertex, Material, Instance>, Shared>(
        std::move(pack));
}

template <typename Key, typename Vertex, typename Material, typename Instance>
inline const PinRef<StaticPack<Vertex, Material, Instance>> getStaticPackByKey(
    const Key& key) noexcept {
    return getKey<Key, StaticPack<Vertex, Material, Instance>, Shared>(key);
}

template <typename Key, typename Vertex, typename Material, typename Instance>
inline StaticPackHandle<Vertex, Material, Instance> tryGetOwnedStaticPackByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, StaticPack<Vertex, Material, Instance>, Shared>(
        key);
}

namespace unsafe {
template <typename Key, typename Vertex, typename Material, typename Instance>
inline PinRef<StaticPack<Vertex, Material, Instance>> getStaticPackByKey(
    const Key& key) noexcept {
    return getKey<Key, StaticPack<Vertex, Material, Instance>, Shared>(key);
}

}  // namespace unsafe
