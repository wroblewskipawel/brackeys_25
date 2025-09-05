#pragma once

#include "collections/slot_map/static.h"

template <typename Instance, size_t InstanceCount>
class InstanceBuffer;

template <typename Instance, size_t InstanceCount>
using InstanceBufferHandle =
    StaticHandle<InstanceBuffer<Instance, InstanceCount>, Shared>;

template <typename Instance, size_t InstanceCount>
inline InstanceBufferHandle<Instance, InstanceCount> registerInstanceBuffer(
    InstanceBuffer<Instance, InstanceCount>&& inssanceBuffer) noexcept {
    return registerResource<InstanceBuffer<Instance, InstanceCount>, Shared>(
        std::move(inssanceBuffer));
}

template <typename Key, typename Instance, size_t InstanceCount>
inline const Ref<InstanceBuffer<Instance, InstanceCount>, Shared>
getInstanceBufferByKey(const Key& key) noexcept {
    return getKey<Key, InstanceBuffer<Instance, InstanceCount>, Shared>(key);
}

template <typename Key, typename Instance, size_t InstanceCount>
inline InstanceBufferHandle<Instance, InstanceCount>
tryGetOwnedInstanceBufferByKey(const Key& key) noexcept {
    return tryGetOwned<Key, InstanceBuffer<Instance, InstanceCount>, Shared>(
        key);
}

namespace unsafe {
template <typename Key, typename Instance, size_t InstanceCount>
inline Ref<InstanceBuffer<Instance, InstanceCount>, Shared>
getInstanceBufferByKey(const Key& key) noexcept {
    return getKey<Key, InstanceBuffer<Instance, InstanceCount>, Shared>(key);
}

}  // namespace unsafe
