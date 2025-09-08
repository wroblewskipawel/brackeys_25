#pragma once

#include "collections/unique_list.h"
#include "graphics/assets/bundle.h"
#include "graphics/resources/gl/material.h"
#include "graphics/resources/gl/mesh.h"
#include "graphics/resources/gl/model.h"
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

    template <typename Vertex>
    auto getPackHandle() const noexcept {
        return packList.get<MeshPackHandle<Vertex>>().copy();
    }

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

    template <typename Material>
    auto getPackHandle() const noexcept {
        return packList.get<MaterialPackHandle<Material>>().copy();
    }

   private:
    MaterialPackHandleList<Materials...> packList;
};

template <typename, typename>
class ResourceBundle;

template <typename... Vertices, typename... Materials>
class ResourceBundle<TypeList<Vertices...>, TypeList<Materials...>> {
   public:
    using IndexStorage =
        DocumentIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>>;

    template <typename Vertex, typename Material>
    using Ref = typename IndexStorage::template Ref<Vertex, Material>;

    ResourceBundle(
        const DocumentBundle<TypeList<Vertices...>, TypeList<Materials...>>&
            documentBundle) noexcept
        : animations(copyVector(documentBundle.getAnimations())),
          meshPacks(documentBundle.getMeshes()),
          materialPacks(documentBundle.getMaterials()),
          documenIndexMap(documentBundle.getIndicesMap()) {}

    template <typename Vertex, typename Material>
    auto getModel(const std::filesystem::path& documentPath,
                  const std::string& modelName) const noexcept {
        const auto modelIndices =
            documenIndexMap.getModelIndices<Vertex, Material>(documentPath,
                                                              modelName);
        return tryGetModel(modelIndices);
    }

    template <typename Vertex, typename Material>
    auto getModel(const std::filesystem::path& documentPath,
                  size_t modelIndex) const noexcept {
        const auto modelIndices =
            documenIndexMap.getModelIndices<Vertex, Material>(documentPath,
                                                              modelIndex);
        return tryGetModel(modelIndices);
    }

    // Add constexpr check if the Vertex type is AnimatedVertex type
    template <typename Vertex, typename Material>
    auto getModelAnimations(const std::filesystem::path& documentPath,
                            const std::string& modelName) const noexcept {
        const auto modelIndices =
            documenIndexMap.getModelIndices<Vertex, Material>(documentPath,
                                                              modelName);
        return tryGetAnimations(modelIndices);
    }

    template <typename Vertex, typename Material>
    auto getModelAnimations(const std::filesystem::path& documentPath,
                            size_t modelIndex) const noexcept {
        const auto modelIndices =
            documenIndexMap.getModelIndices<Vertex, Material>(documentPath,
                                                              modelIndex);
        return tryGetAnimations(modelIndices);
    }

    template <typename Vertex, typename Material, typename Instance>
    DrawPackBuilder<Vertex, Material, Instance> getDrawPackBuilder()
        const noexcept {
        return DrawPackBuilder<Vertex, Material, Instance>(
            meshPacks.getPackHandle<Vertex>(),
            materialPacks.getPackHandle<Material>());
    }

   private:
    template <typename Vertex, typename Material>
    auto tryGetModel(const Ref<Vertex, Material>& modelRef) const noexcept {
        auto model = Model<Vertex, Material>::getInvalid();
        if (modelRef.isValid()) {
            model.mesh.packItemIndex = modelRef.get().meshIndex;
            model.mesh.packHandle = meshPacks.getPackHandle<Vertex>();
            model.material.packItemIndex = modelRef.get().materialIndex;
            model.material.packHandle = materialPacks.getPackHandle<Material>();
        }
        return std::move(model);
    }

    template <typename Vertex, typename Material>
    auto tryGetAnimations(
        const Ref<Vertex, Material>& modelRef) const noexcept {
        auto modelAnimations = std::vector<AnimationHandle>();
        if (modelRef.isValid()) {
            for (const auto& animationIndex : modelRef.get().animationIndices) {
                modelAnimations.emplace_back(animations[animationIndex].copy());
            }
        }
        return std::move(modelAnimations);
    }

    std::vector<AnimationHandle> animations;
    MeshPackList<Vertices...> meshPacks;
    MaterialPackList<Materials...> materialPacks;
    IndexStorage documenIndexMap;
};

template <typename... Vertices, typename... Materials>
ResourceBundle(
    const DocumentBundle<TypeList<Vertices...>, TypeList<Materials...>>&)
    -> ResourceBundle<TypeList<Vertices...>, TypeList<Materials...>>;
