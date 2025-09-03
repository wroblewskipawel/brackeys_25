#pragma once

#include "collections/static.h"

template <typename Vertex>
class MeshPack;

template <typename Vertex>
using MeshPackHandle = StaticHandle<MeshPack<Vertex>, Shared>;

template <typename Vertex>
inline MeshPackHandle<Vertex> registerMeshPack(
    MeshPack<Vertex>&& mesh) noexcept {
    return registerResource<MeshPack<Vertex>, Shared>(std::move(mesh));
}
