#pragma once

#include "collections/slot_map.h"

template<typename Vertex>
struct MeshData;

template <typename Vertex>
class MeshPackBuilder;

template<typename Vertex>
using MeshDataHandle = Handle<MeshData<Vertex>>;

template<typename Vertex>
class MeshDataStorage {
    private:
    friend class MeshData<Vertex>;
    friend class MeshPackBuilder<Vertex>;
    
    inline static SlotMap<MeshData<Vertex>> meshStorage = {};
};
