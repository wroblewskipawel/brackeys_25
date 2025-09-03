#pragma once

#include "collections/static.h"

template <typename Vertex>
struct MeshData;

template <typename Vertex>
using MeshDataHandle = StaticHandle<MeshData<Vertex>, Shared>;

template <typename Vertex>
inline MeshDataHandle<Vertex> registerMeshData(
    MeshData<Vertex>&& mesh) noexcept {
    return registerResource<MeshData<Vertex>, Shared>(std::move(mesh));
}
