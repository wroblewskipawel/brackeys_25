#pragma once

#include <filesystem>
#include <numeric>
#include <ranges>
#include <unordered_map>

#include "collections/unique_list.h"
#include "collections/unique_list/vector_list.h"
#include "graphics/assets/gltf.h"
#include "graphics/resources/material.h"
#include "graphics/storage/animation.h"
#include "graphics/storage/mesh.h"

template <typename VertexIndex, typename MaterialIndex>
struct DocumentIndices {
    std::vector<VertexIndex> meshIndices;
    std::vector<MaterialIndex> materialIndices;
    std::vector<uint32_t> animationIndices;
    std::vector<uint32_t> modelIndices;
    std::unordered_map<std::string, size_t> modelNameMap;
};

template <typename, typename>
struct DocumentIndicesStorage;

template <typename... Vertices, typename... Materials>
struct DocumentIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>> {
    template <typename Vertex>
    using VertexIndex =
        VectorListIndex<MeshDataHandle<Vertex>,
                        VectorList<MeshDataHandle<Vertices>...>>;

    template <typename Material>
    using MaterialIndex =
        VectorListIndex<MaterialBuilder<Material>,
                        VectorList<MaterialBuilder<Materials>...>>;

    template <typename Vertex, typename Material>
    using Indices =
        DocumentIndices<VertexIndex<Vertex>, MaterialIndex<Material>>;

    template <typename Vertex, typename Material>
    using IndicesMap =
        std::unordered_map<std::filesystem::path, Indices<Vertex, Material>>;

    // For the Product it would be more intuitive to keep the same ordering as
    // in the resulting lsit product types, here to get the IndicesMap<Vertex,
    // Material> we have to first declare the Materials list and then Vertex
    // list
    using MapStorageBuilder =
        typename Product<IndicesMap, UniqueTypeListBuilder<Materials...>,
                         UniqueTypeListBuilder<Vertices...>>::Type;
    using MapStorage = typename MapStorageBuilder::UniqueTypeList;

    template <typename Vertex, typename Material>
    auto& getIndexMap() noexcept {
        return indicesMapStorage.get<IndicesMap<Vertex, Material>>();
    }

    MapStorage indicesMapStorage;
};

template <typename, typename>
class DocumentBundle;

template <typename... Vertices, typename... Materials>
class DocumentBundle<TypeList<Vertices...>, TypeList<Materials...>> {
   public:
    using IndexStorage =
        DocumentIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>>;

    template <typename Vertex, typename Material>
    using Indices = typename IndexStorage::template Indices<Vertex, Material>;

    template <typename Material>
    using MaterialIndex =
        typename IndexStorage::template MaterialIndex<Material>;

    template <typename Vertex>
    using VertexIndex = typename IndexStorage::template VertexIndex<Vertex>;

    DocumentBundle() = default;

    template <typename Vertex, typename Material>
    void pushDocument(const std::filesystem::path& filePath) {
        auto document = DocumentReader<Vertex, Material>(filePath);
        // Here it was required to use the DocumentIndices type explicitly, as
        // using its Indices type allias defined in the scope caused compilation
        // errors, try investigate why
        auto documentIndices =
            DocumentIndices<VertexIndex<Vertex>, MaterialIndex<Material>>{
                .meshIndices = appendMeshes(document.takeMeshes()),
                .materialIndices = appendMaterials(document.takeMaterials()),
                .animationIndices = appendAnimations(document.takeAnimations()),
                .modelIndices = appendModelIndices(document.takeIndices()),
                .modelNameMap = document.takeModelNameMap(),
            };
        auto& documentMap = documenIndexMap.getIndexMap<Vertex, Material>();
        documentMap.emplace(filePath, std::move(documentIndices));
    };

    const auto& getMeshes() const noexcept { return meshTypesStorage; }

    const auto& getMaterials() const noexcept { return materialTypesStorage; }

   private:
    auto appendAnimations(std::vector<AnimationHandle>&& animations) noexcept {
        auto indices = std::vector<uint32_t>(animations.size());
        auto firstIndex = static_cast<uint32_t>(animationStorage.size());
        std::iota(indices.begin(), indices.end(), firstIndex);
        animationStorage.insert(animationStorage.end(),
                                std::make_move_iterator(animations.begin()),
                                std::make_move_iterator(animations.end()));
        return indices;
    }

    auto appendModelIndices(std::vector<ModelIndices>&& modelIndices) noexcept {
        auto indices = std::vector<uint32_t>(modelIndices.size());
        auto firstIndex = static_cast<uint32_t>(modelIndicesStorage.size());
        std::iota(indices.begin(), indices.end(), firstIndex);
        modelIndicesStorage.insert(
            modelIndicesStorage.end(),
            std::make_move_iterator(modelIndices.begin()),
            std::make_move_iterator(modelIndices.end()));
        return indices;
    }

    template <typename Material>
    auto appendMaterials(
        std::vector<MaterialBuilder<Material>>&& materials) noexcept {
        auto indices = std::vector<MaterialIndex<Material>>();
        for (auto&& material : std::move(materials)) {
            indices.emplace_back(
                materialTypesStorage.insert<MaterialBuilder<Material>>(
                    std::move(material)));
        }
        return indices;
    }

    template <typename Vertex>
    auto appendMeshes(std::vector<MeshDataHandle<Vertex>>&& meshes) noexcept {
        auto indices = std::vector<VertexIndex<Vertex>>();
        for (auto&& mesh : std::move(meshes)) {
            indices.emplace_back(
                meshTypesStorage.insert<MeshDataHandle<Vertex>>(
                    std::move(mesh)));
        }
        return indices;
    }

    std::vector<ModelIndices> modelIndicesStorage;
    std::vector<AnimationHandle> animationStorage;
    VectorList<MeshDataHandle<Vertices>...> meshTypesStorage;
    VectorList<MaterialBuilder<Materials>...> materialTypesStorage;
    IndexStorage documenIndexMap;
};
