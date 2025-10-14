#pragma once

#include "collections/slot_map/static.h"

template <typename Type>
class StreamBuffer;

template <typename Type>
using StreamHandle = StaticHandle<StreamBuffer<Type>, Shared>;

template <typename Type>
inline StreamHandle<Type> registerStreamBuffer(
    StreamBuffer<Type>&& stream) noexcept {
    return registerResource<StreamBuffer<Type>, Shared>(
        std::move(stream));
}

template <typename Key, typename Type>
inline const PinRef<StreamHandle<Type>> getStreamBufferByKey(
    const Key& key) noexcept {
    return getKey<Key, StreamHandle<Type>, Shared>(key);
}

template <typename Key, typename Type>
inline StreamHandle<Type> tryGetOwnedStreamBufferByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, StreamHandle<Type>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Type>
inline PinRef<StreamHandle<Type>> getStreamBufferByKey(
    const Key& key) noexcept {
    return getKey<Key, StreamHandle<Type>, Shared>(key);
}

}  // namespace unsafe
