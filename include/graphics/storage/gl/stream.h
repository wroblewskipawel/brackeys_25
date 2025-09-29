#pragma once

#include "collections/slot_map/static.h"

template <typename Type, size_t BufferSize>
class StreamBuffer;

template <typename Type, size_t BufferSize>
using StreamHandle = StaticHandle<StreamBuffer<Type, BufferSize>, Shared>;

template <typename Type, size_t BufferSize>
inline StreamHandle<Type, BufferSize> registerStreamBuffer(
    StreamBuffer<Type, BufferSize>&& stream) noexcept {
    return registerResource<StreamBuffer<Type, BufferSize>, Shared>(
        std::move(stream));
}

template <typename Key, typename Type, size_t BufferSize>
inline const PinRef<StreamHandle<Type, BufferSize>> getStreamBufferByKey(
    const Key& key) noexcept {
    return getKey<Key, StreamHandle<Type, BufferSize>, Shared>(key);
}

template <typename Key, typename Type, size_t BufferSize>
inline StreamHandle<Type, BufferSize> tryGetOwnedStreamBufferByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, StreamHandle<Type, BufferSize>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Type, size_t BufferSize>
inline PinRef<StreamHandle<Type, BufferSize>> getStreamBufferByKey(
    const Key& key) noexcept {
    return getKey<Key, StreamHandle<Type, BufferSize>, Shared>(key);
}

}  // namespace unsafe
