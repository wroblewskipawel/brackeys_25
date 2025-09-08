#pragma once

#include "collections/slot_map/static.h"

template <typename Vertices, typename Materials>
class ResourceBundle;

template <typename Vertices, typename Materials>
using ResourceBundleHandle =
    StaticHandle<ResourceBundle<Vertices, Materials>, Shared>;

template <typename Vertices, typename Materials>
inline ResourceBundleHandle<Vertices, Materials> registerResourceBundle(
    ResourceBundle<Vertices, Materials>&& bundle) noexcept {
    return registerResource<ResourceBundle<Vertices, Materials>, Shared>(
        std::move(bundle));
}

template <typename Key, typename Vertices, typename Materials>
inline const PinRef<ResourceBundle<Vertices, Materials>> getResourceBundleByKey(
    const Key& key) noexcept {
    return getKey<Key, ResourceBundle<Vertices, Materials>, Shared>(key);
}

template <typename Key, typename Vertices, typename Materials>
inline ResourceBundleHandle<Vertices, Materials> tryGetOwnedResourceBundleByKey(
    const Key& key) noexcept {
    return tryGetOwned<Key, ResourceBundle<Vertices, Materials>, Shared>(key);
}

namespace unsafe {
template <typename Key, typename Vertices, typename Materials>
inline PinRef<ResourceBundle<Vertices, Materials>> getResourceBundleByKey(
    const Key& key) noexcept {
    return getKey<Key, ResourceBundle<Vertices, Materials>, Shared>(key);
}

}  // namespace unsafe
