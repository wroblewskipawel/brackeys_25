#pragma once

#include "collections/unique_list.h"
#include "graphics/assets/bundle.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/storage/gl/material.h"
#include "graphics/storage/gl/mesh.h"

template <template <typename> typename HandleType, typename... Types>
using HandleList = UniqueTypeList<HandleType<Types>...>;

template <typename... Vertices>
using MeshPackHandleList = HandleList<MeshPackHandle, Vertices...>;

template <typename... Vertices>
using MeshDataList = VectorList<MeshDataHandle<Vertices>...>;

template <typename Vertices, typename PackHandleList, typename PackDataList>
void loadMeshPacks(Vertices remaining, PackHandleList& packHandles,
                   const PackDataList& packData) noexcept;

template <typename... PackVertices, typename... DataVertices>
void loadMeshPacks(TypeList<> remaining,
                   MeshPackHandleList<PackVertices...>& handleList,
                   const MeshDataList<DataVertices...>& dataList) noexcept {}

template <typename Vertex, typename... Vertices, typename... PackVertices,
          typename... DataVertices>
void loadMeshPacks(TypeList<Vertex, Vertices...> remaining,
                   MeshPackHandleList<PackVertices...>& handleList,
                   const MeshDataList<DataVertices...>& dataList) noexcept {
    auto packBuilder = MeshPackBuilder<Vertex>();
    packBuilder.addMeshMulti(dataList.getStorage<MeshDataHandle<Vertex>>());
    auto& packHandle = handleList.get<MeshPackHandle<Vertex>>();
    packHandle = packBuilder.build();

    loadMeshPacks(TypeList<Vertices...>{}, handleList, dataList);
}

template <typename... Vertices>
class MeshPackList {
   public:
    template <typename... Data>
    MeshPackList(const MeshDataList<Data...>& packData) noexcept {
        loadMeshPacks(TypeList<Vertices...>{}, packList, packData);
    };

   private:
    MeshPackHandleList<Vertices...> packList;
};

template <typename... Materials>
using MaterialPackHandleList = HandleList<MaterialPackHandle, Materials...>;

template <typename... Materials>
using MaterialBuilderList = VectorList<MaterialBuilder<Materials>...>;

template <typename Materials, typename PackHandleList, typename PackDataList>
void loadMaterialPacks(Materials remaining, PackHandleList& packHandles,
                       const PackDataList& packData) noexcept;

template <typename... PackMaterials, typename... DataMaterials>
void loadMaterialPacks(
    TypeList<> remaining, MaterialPackHandleList<PackMaterials...>& handleList,
    const MaterialBuilderList<DataMaterials...>& dataList) noexcept {}

template <typename Material, typename... Materials, typename... PackMaterials,
          typename... DataMaterials>
void loadMaterialPacks(
    TypeList<Material, Materials...> remaining,
    MaterialPackHandleList<PackMaterials...>& handleList,
    const MaterialBuilderList<DataMaterials...>& dataList) noexcept {
    auto packBuilder = MaterialPackBuilder<Material>();
    packBuilder.addMaterialMulti(
        dataList.getStorage<MaterialBuilder<Material>>());
    auto& packHandle = handleList.get<MaterialPackHandle<Material>>();
    packHandle = packBuilder.build();

    loadMaterialPacks(TypeList<Materials...>{}, handleList, dataList);
}

template <typename... Materials>
class MaterialPackList {
   public:
    template <typename... Data>
    MaterialPackList(const MaterialBuilderList<Data...>& packData) noexcept {
        loadMaterialPacks(TypeList<Materials...>{}, packList, packData);
    };

   private:
    MaterialPackHandleList<Materials...> packList;
};

template <typename, typename>
class ResourceBundle;

template <typename... Vertices, typename... Materials>
class ResourceBundle<TypeList<Vertices...>, TypeList<Materials...>> {
   public:
    ResourceBundle(
        const DocumentBundle<TypeList<Vertices...>, TypeList<Materials...>>&
            documentBundle) noexcept
        : meshPacks(documentBundle.getMeshes()),
          materialPacks(documentBundle.getMaterials()) {}

   private:
    MeshPackList<Vertices...> meshPacks;
    MaterialPackList<Materials...> materialPacks;
};

template <typename... Vertices, typename... Materials>
ResourceBundle(
    const DocumentBundle<TypeList<Vertices...>, TypeList<Materials...>>&)
    -> ResourceBundle<TypeList<Vertices...>, TypeList<Materials...>>;
