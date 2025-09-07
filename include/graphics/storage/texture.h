#pragma once

#include "collections/slot_map.h"
#include "collections/slot_map/static.h"

class TextureData;

using TextureDataHandle = StaticHandle<TextureData, Shared>;

inline TextureDataHandle registerTextureData(TextureData&& texture) noexcept {
    return registerResource<TextureData, Shared>(std::move(texture));
}

template <typename Key>
inline const PinRef<TextureData> getTextureDataByKey(const Key& key) noexcept {
    return getKey<Key, TextureData, Shared>(key);
}

template <typename Key>
inline TextureDataHandle tryGetOwnedTextureDataByKey(const Key& key) noexcept {
    return tryGetOwned<Key, TextureData, Shared>(key);
}

namespace unsafe {
template <typename Key>
inline PinRef<TextureData> getTextureDataByKey(const Key& key) noexcept {
    return getKey<Key, TextureData, Shared>(key);
}
}  // namespace unsafe
