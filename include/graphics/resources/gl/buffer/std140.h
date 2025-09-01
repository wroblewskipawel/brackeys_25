#pragma once

#include <glad/glad.h>

#include <array>
#include <glm/glm.hpp>
#include <iostream>
#include <type_traits>

namespace std140 {

template <typename Element>
struct ArrayElement {
    alignas(16) Element data;
};

template <typename ArrayType>
struct Array {
    static constexpr size_t ArrayExtent = std::extent_v<ArrayType, 0>;
    using ElementType = typename std::remove_all_extents_t<ArrayType>;

    std::array<ArrayElement<ElementType>, ArrayExtent> elements;

    ElementType& operator[](size_t index) noexcept {
        return elements[index].data;
    }

    const ElementType& operator[](size_t index) const noexcept {
        return elements[index].data;
    }
};

template <typename Item>
struct isGlType {
    static constexpr bool value =
        std::is_same_v<Item, GLuint> || std::is_same_v<Item, GLuint64> ||
        std::is_same_v<Item, GLint> || std::is_same_v<Item, GLint64>;
};

template <typename Item>
constexpr size_t alignment() {
    if constexpr (std::is_array_v<Item>) {
        return 16;
    } else if constexpr (std::is_aggregate_v<Item>) {
        if constexpr (sizeof(Item) <= 8) {
            return 8;
        } else {
            return 16;
        }
    } else if constexpr (std::is_arithmetic_v<Item> || isGlType<Item>::value) {
        if constexpr (sizeof(Item) <= 4) {
            return 4;
        } else {
            return 8;
        }
    } else {
        static_assert(!std::is_same_v<Item, Item>,
                      "Unsupported type for std140::alignment");
    }
}

template <typename Item>
struct std140Type {
    static constexpr auto getType() {
        if constexpr (std::is_array_v<Item>) {
            return Array<Item>{};
        } else {
            return Item{};
        }
    }
    using Type = decltype(getType());
};

template <typename... Items>
struct BlockData {};

template <>
struct BlockData<> {};

template <typename Item, typename... Items>
struct BlockData<Item, Items...> {
    BlockData(Item&& item, Items&&... items)
        : item{std::forward<Item>(item)}, rest{std::forward<Items>(items)...} {}

    alignas(alignment<Item>()) typename std140Type<Item>::Type item;
    BlockData<Items...> rest;
};

template <typename... Items>
BlockData(Items&&...) -> BlockData<std::decay_t<Items>...>;

template <size_t Index, typename... Items>
constexpr auto& getAtIndex(BlockData<Items...>& block) {
    if constexpr (sizeof...(Items) == 0) {
        static_assert(!std::is_same_v<BlockData<Items...>, BlockData<Items...>>,
                      "Index out of bounds in std140::getAtIndex");
    } else {
        if constexpr (Index == 0) {
            return block.item;
        } else {
            return getAtIndex<Index - 1>(block.rest);
        }
    }
}

template <typename Search, typename... Items>
constexpr Search& getUnique(BlockData<Items...>& block);

template <typename Search, typename Item, typename... Items>
constexpr Search& getUnique(BlockData<Item, Items...>& block) {
    if constexpr (std::is_same_v<Search, Item>) {
        return block.item;
    } else {
        if constexpr (sizeof...(Items) == 0) {
            static_assert(!std::is_same_v<Search, Search>,
                          "Type not found in std140::getUnique");
        }
        return getUnique<Search, Items...>(block.rest);
    }
}

template <typename... Items>
struct Block {
    Block(Items&&... items) : data{std::forward<Items>(items)...} {}

    template <size_t Index>
    auto& getAtIndex() {
        return getAtIndex<Index>(data);
    }

    template <typename T>
    T& getUnique() {
        return getUnique<T>(data);
    }

    alignas(16) BlockData<Items...> data;
};

template <typename Item>
class UniformArrayBuilder;

template <typename Item>
class UniformArray {
   public:
    UniformArray(const UniformArray&) = delete;
    UniformArray& operator=(const UniformArray&) = delete;

    UniformArray(UniformArray&& other)
        : uniformBlockBuffer(other.uniformBlockBuffer),
          numElements(other.numElements) {
        other.uniformBlockBuffer = 0;
        other.numElements = 0;
    };
    UniformArray& operator=(UniformArray&& other) {
        if (this != &other) {
            glDeleteBuffers(1, &uniformBlockBuffer);
            uniformBlockBuffer = other.uniformBlockBuffer;
            numElements = other.numElements;
            other.uniformBlockBuffer = 0;
            other.numElements = 0;
        }
        return *this;
    };

    ~UniformArray() { glDeleteBuffers(1, &uniformBlockBuffer); }

    UniformArray& updateRange(const std::vector<Item>& data,
                              size_t indexOffset) {
        return updateBuffer(data.data(), data.size(), indexOffset);
    }

    UniformArray& update(const Item& data, size_t indexOffset) {
        return updateBuffer(&data, 1, indexOffset);
    }

    void bind(GLenum target, GLuint index) const {
        glBindBufferBase(target, index, uniformBlockBuffer);
    }

    GLuint getBuffer() const noexcept { return uniformBlockBuffer; }

    size_t getNumElements() const noexcept { return numElements; }

   private:
    friend class UniformArrayBuilder<Item>;

    UniformArray(const std::vector<Item>& data) : numElements(data.size()) {
        glCreateBuffers(1, &uniformBlockBuffer);
        glNamedBufferStorage(uniformBlockBuffer, data.size() * sizeof(Item),
                             data.data(), GL_DYNAMIC_STORAGE_BIT);
    }

    UniformArray& updateBuffer(const Item* data, size_t dataSize,
                               size_t indexOffset) {
        if (indexOffset + dataSize <= numElements) {
            glNamedBufferSubData(uniformBlockBuffer, indexOffset * sizeof(Item),
                                 dataSize * sizeof(Item), data);
        } else {
            std::println(std::cerr, "UniformArray::updateRange: Out of bounds");
            std::abort();
        }
        return *this;
    }

    GLuint uniformBlockBuffer;
    size_t numElements;
};

template <typename Item>
class UniformArrayBuilder {
   public:
    UniformArrayBuilder() = default;

    UniformArrayBuilder(const UniformArrayBuilder&) = delete;
    UniformArrayBuilder& operator=(const UniformArrayBuilder&) = delete;

    UniformArrayBuilder(UniformArrayBuilder&&) = default;
    UniformArrayBuilder& operator=(UniformArrayBuilder&&) = default;

    template <typename... Items>
    UniformArrayBuilder& emplace(Items&&... items) {
        buffer.emplace_back(std::forward<Items>(items)...);
        return *this;
    }

    UniformArrayBuilder& push(const Item& block) {
        buffer.push_back(block);
        return *this;
    }

    UniformArrayBuilder& pushMulti(const std::vector<Item>& items) {
        buffer.insert(buffer.end(), items.begin(), items.end());
        return *this;
    }

    const std::vector<Item>& getBuffer() const { return buffer; }

    UniformArray<Item> build() const { return UniformArray<Item>(buffer); }

   private:
    std::vector<Item> buffer;
};

}  // namespace std140
