#pragma once

#include "collections/slot_map/static.h"

template <typename Vertex, typename Material, typename Instance>
class StaticBatch;

template <typename Vertex, typename Material, typename Instance>
using StaticBatchHandle =
    StaticHandle<StaticBatch<Vertex, Material, Instance>, Shared>;

template <typename Vertex, typename Material, typename Instance>
inline StaticBatchHandle<Vertex, Material, Instance> registerStaticBatch(
    StaticBatch<Vertex, Material, Instance>&& pack) noexcept {
    return registerResource<StaticBatch<Vertex, Material, Instance>, Shared>(
        std::move(pack));
}

template <typename Key, typename Vertex, typename Material, typename Instance>
inline const PinRef<StaticBatch<Vertex, Material, Instance>>
getStaticBatchByKey(const Key& key) noexcept {
    return getKey<Key, StaticBatch<Vertex, Material, Instance>, Shared>(key);
}

template <typename Key, typename Vertex, typename Material, typename Instance>
inline StaticBatchHandle<Vertex, Material, Instance>
tryGetOwnedStaticBatchByKey(const Key& key) noexcept {
    return tryGetOwned<Key, StaticBatch<Vertex, Material, Instance>, Shared>(
        key);
}

namespace unsafe {
template <typename Key, typename Vertex, typename Material, typename Instance>
inline PinRef<StaticBatch<Vertex, Material, Instance>> getStaticBatchByKey(
    const Key& key) noexcept {
    return getKey<Key, StaticBatch<Vertex, Material, Instance>, Shared>(key);
}

}  // namespace unsafe
