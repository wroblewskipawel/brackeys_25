#pragma once

#include "collections/static.h"

class TextureData;

using TextureDataHandle = StaticHandle<TextureData, Shared>;

inline TextureDataHandle registerTextureData(TextureData&& texture) noexcept {
    return registerResource<TextureData, Shared>(std::move(texture));
}
