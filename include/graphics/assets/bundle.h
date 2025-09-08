#pragma once

#include <filesystem>
#include <numeric>
#include <ranges>
#include <unordered_map>

#include "collections/map_vector.h"
#include "collections/pin_ref.h"
#include "collections/unique_list.h"
#include "collections/unique_list/vector_list.h"
#include "graphics/assets/gltf.h"
#include "graphics/resources/material.h"
#include "graphics/storage/animation.h"
#include "graphics/storage/mesh.h"

template <typename, typename>
struct DocumentIndicesStorage;

template <typename... Vertices, typename... Materials>
struct DocumentIndicesStorage<TypeList<Vertices...>, TypeList<Materials...>> {
    template <typename Vertex, typename Material>
    using Ref = PinRef<ModelIndices<Vertex, Material>>;

    template <typename Vertex, typename Material>
    using Indices = NamedVector<ModelIndices<Vertex, Material>>;

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

    template <typename Vertex, typename Material>
    const auto& getIndexMap() const noexcept {
        return indicesMapStorage.get<IndicesMap<Vertex, Material>>();
    }

    template <typename Vertex, typename Material>
    const Ref<Vertex, Material> getModelIndices(
        const std::filesystem::path& documentPath,
        const std::string& modelName) const noexcept {
        auto& documentIndices = getIndexMap<Vertex, Material>();
        auto result = documentIndices.find(documentPath);
        if (result != documentIndices.end()) {
            return result->second.tryGet(modelName);
        }
        return Ref<Vertex, Material>::null();
    }

    template <typename Vertex, typename Material>
    const Ref<Vertex, Material> getModelIndices(
        const std::filesystem::path& documentPath,
        size_t modelIndex) const noexcept {
        auto& documentIndices = getIndexMap<Vertex, Material>();
        auto result = documentIndices.find(documentPath);
        if (result != documentIndices.end()) {
            return result->second.getAtIndex(modelIndex);
        }
        return Ref<Vertex, Material>::null();
    }

    MapStorage indicesMapStorage;
};

struct IndicesOffsets {
    size_t meshOffset;
    size_t materialOffset;
    size_t animationOffset;
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

    DocumentBundle() = default;

    template <typename Vertex, typename Material>
    void pushDocument(const std::filesystem::path& filePath) {
        auto document = DocumentReader<Vertex, Material>(filePath);
        auto indicesOffsets = IndicesOffsets{
            .meshOffset = appendMeshes(document.takeMeshes()),
            .materialOffset = appendMaterials(document.takeMaterials()),
            .animationOffset = appendAnimations(document.takeAnimations()),
        };
        registerDocumentIndices(filePath, document.takeIndices(),
                                indicesOffsets);
    };

    const auto& getAnimations() const noexcept { return animationStorage; }

    const auto& getMeshes() const noexcept { return meshTypesStorage; }

    const auto& getMaterials() const noexcept { return materialTypesStorage; }

    const auto& getIndicesMap() const noexcept { return documenIndexMap; }

   private:
    auto appendAnimations(std::vector<AnimationHandle>&& animations) noexcept {
        auto firstIndex = static_cast<uint32_t>(animationStorage.size());
        animationStorage.insert(animationStorage.end(),
                                std::make_move_iterator(animations.begin()),
                                std::make_move_iterator(animations.end()));
        return firstIndex;
    }

    template <typename Material>
    auto appendMaterials(
        std::vector<MaterialBuilder<Material>>&& materials) noexcept {
        auto firstIndex =
            materialTypesStorage.size<MaterialBuilder<Material>>();
        for (auto&& material : std::move(materials)) {
            materialTypesStorage.insert<MaterialBuilder<Material>>(
                std::move(material));
        }
        return firstIndex;
    }

    template <typename Vertex>
    auto appendMeshes(std::vector<MeshDataHandle<Vertex>>&& meshes) noexcept {
        auto firstIndex = meshTypesStorage.size<MeshDataHandle<Vertex>>();
        for (auto&& mesh : std::move(meshes)) {
            meshTypesStorage.insert<MeshDataHandle<Vertex>>(std::move(mesh));
        }
        return firstIndex;
    }

    template <typename Vertex, typename Material>
    auto registerDocumentIndices(const std::filesystem::path& filePath,
                                 Indices<Vertex, Material>&& indices,
                                 IndicesOffsets offsets) noexcept {
        for (auto& meshIndices : indices.getItems()) {
            meshIndices.meshIndex += offsets.meshOffset;
            meshIndices.materialIndex += offsets.materialOffset;
            for (auto& animationIndex : meshIndices.animationIndices) {
                animationIndex += offsets.animationOffset;
            }
        }

        auto& documentMap = documenIndexMap.getIndexMap<Vertex, Material>();
        documentMap.emplace(filePath, std::move(indices));
    }

    std::vector<AnimationHandle> animationStorage;
    VectorList<MeshDataHandle<Vertices>...> meshTypesStorage;
    VectorList<MaterialBuilder<Materials>...> materialTypesStorage;
    IndexStorage documenIndexMap;
};
