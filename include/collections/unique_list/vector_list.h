#pragma once

template <typename... Types>
class VectorListBuilder;

template <typename... Types>
class VectorList;

// Number of various "Index" type is becoming a bit much...
template <typename, typename>
struct VectorListIndex {
    uint32_t itemIndex;
    uint32_t listIndex;
};

template <typename... Types>
class VectorList {
   public:
    template <typename Type>
    using Index = VectorListIndex<Type, VectorList>;

    VectorList() : listIndex(nextListIndex++) {};

    VectorList(const VectorList&) = delete;
    VectorList& operator=(const VectorList&) = delete;

    VectorList(VectorList&&) = default;
    VectorList& operator=(VectorList&&) = default;

    template <typename Type>
    Index<Type> insert(Type&& handle) noexcept {
        auto& typeStorage = vectorStorage.get<std::vector<Type>>();
        auto handleIndex = typeStorage.size();
        typeStorage.emplace_back(std::move(handle));
        return Index<Type>{
            .itemIndex = static_cast<uint32_t>(handleIndex),
            .listIndex = listIndex,
        };
    }

    template <typename Type>
    Type get(Index<Type> index) const noexcept {
        auto handle = Type::getInvalid();
        if (isIndexValid(index)) {
            const auto& typeStorage = vectorStorage.get<std::vector<Type>>();
            handle = typeStorage[index.itemIndex].copy();
        }
        return handle;
    }

    template <typename Type>
    void pop(Index<Type> index) noexcept {
        if (isIndexValid(index)) {
            const auto& typeStorage = vectorStorage.get<std::vector<Type>>();
            typeStorage[index.itemIndex] = Type::getInvalid();
        }
    }

    template <typename Type>
    size_t size() const noexcept {
        return vectorStorage.get<std::vector<Type>>().size();
    }

    template <typename Type>
    std::vector<Type>& getStorage() noexcept {
        return vectorStorage.get<std::vector<Type>>();
    }

    template <typename Type>
    const std::vector<Type>& getStorage() const noexcept {
        return vectorStorage.get<std::vector<Type>>();
    }

   private:
    using StorageBuilder =
        typename Wrap<std::vector, UniqueTypeListBuilder<Types...>>::Type;
    using Storage = typename StorageBuilder::UniqueTypeList;

    friend class VectorListBuilder<Types...>;

    inline static uint32_t nextListIndex = 0;

    template <typename Type>
    bool isIndexValid(Index<Type> index) const noexcept {
        const auto& typeStorage = vectorStorage.get<std::vector<Type>>();
        return index.listIndex == listIndex &&
               index.itemIndex < typeStorage.size();
    }

    uint32_t listIndex;
    Storage vectorStorage;
};

template <typename... Types>
class VectorListBuilder {
   public:
    template <typename Next>
    auto withType() const noexcept {
        return VectorListBuilder<Next, Types...>();
    }

    VectorList<Types...> build() const noexcept {
        return VectorList<Types...>();
    }
};
