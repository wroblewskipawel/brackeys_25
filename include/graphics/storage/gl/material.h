#pragma once

#include "collections/static.h"

template <typename Material>
class MaterialPack;

template <typename Material>
using MaterialPackHandle = StaticHandle<MaterialPack<Material>, Shared>;

template <typename Material>
inline MaterialPackHandle<Material> registerMaterialPack(
    MaterialPack<Material>&& mesh) noexcept {
    return registerResource<MaterialPack<Material>, Shared>(std::move(mesh));
}
