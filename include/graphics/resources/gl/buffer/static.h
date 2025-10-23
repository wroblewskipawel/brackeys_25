#pragma once

#include <glad/glad.h>

#include "concepts/range.h"
#include "graphics/resources/gl/buffer/binding.h"

template <typename Item>
class StaticBufferVec;

template <typename Item>
struct StaticBufferInfo {
    GLuint buffer;
    uint32_t numItems;

    template <auto Binding>
    void bindBuffer(GLuint bindingIndex) const noexcept {
        static_assert(std::is_same_v<decltype(Binding), BufferBindings>);
        BindingState::bindBuffer<Binding>(buffer, bindingIndex);
    };
};

template <typename Item>
class StaticBuffer {
   public:
    template <typename Range>
        requires ContiguousRefConstRange<Range, Item>
    StaticBuffer(Range&& range) noexcept {
        numItems = std::ranges::distance(range);
        glCreateBuffers(1, &buffer);
        glNamedBufferStorage(buffer, sizeof(Item) * numItems, &*range.begin(),
                             GL_NONE);
    }

    StaticBuffer(const StaticBuffer&) = delete;
    StaticBuffer& operator=(const StaticBuffer&) = delete;

    StaticBuffer(StaticBuffer&& other) noexcept
        : buffer(other.buffer), numItems(other.numItems) {
        other.buffer = GL_ZERO;
        other.numItems = 0;
    };

    StaticBuffer& operator=(StaticBuffer&& other) noexcept {
        if (this != &other) {
            buffer = other.buffer;
            numItems = other.numItems;
            other.buffer = GL_ZERO;
            other.numItems = 0;
        }
        return *this;
    };

    ~StaticBuffer() noexcept { glDeleteBuffers(1, &buffer); }

    auto getBufferInfo() const noexcept {
        return StaticBufferInfo<Item>{
            .buffer = buffer,
            .numItems = numItems,
        };
    }

   private:
    friend class StaticBufferVec<Item>;

    StaticBuffer(GLuint buffer, uint32_t numItems) noexcept
        : buffer(buffer), numItems(numItems) {}

    GLuint buffer;
    uint32_t numItems;
};

template <typename Item>
class StaticBufferVec {
   public:
    template <typename Range>
        requires ContiguousRefConstRangeRange<Range, Item>
    StaticBufferVec(Range&& range) noexcept {
        auto numBuffers = std::ranges::distance(range);
        buffers.resize(numBuffers);
        numItems.resize(numBuffers);
        glCreateBuffers(buffers.size(), buffers.data());
        for (const auto& [i, dataRange] : std::views::enumerate(range)) {
            numItems[i] = std::ranges::distance(dataRange);
            glNamedBufferStorage(buffers[i], sizeof(Item) * numItems[i],
                                 &*dataRange.begin(), GL_NONE);
        }
    }

    auto split() noexcept {
        auto staticBuffers =
            std::views::transform(std::views::zip(buffers, numItems),
                                  [](const auto& zip) {
                                      return StaticBuffer<Item>{
                                          std::get<0>(zip),
                                          std::get<1>(zip),
                                      };
                                  }) |
            std::ranges::to<std::vector>();
        numItems.clear();
        buffers.clear();
        return staticBuffers;
    }

    StaticBufferVec(const StaticBufferVec&) = delete;
    StaticBufferVec& operator=(const StaticBufferVec&) = delete;

    StaticBufferVec(StaticBufferVec&& other) = default;
    StaticBufferVec& operator=(StaticBufferVec&& other) = default;

    ~StaticBufferVec() noexcept {
        glDeleteBuffers(buffers.size(), buffers.data());
    }

    auto size() const noexcept { return buffers.size(); }

    auto getBufferInfo(size_t bufferIndex) const noexcept {
        checkIndex(bufferIndex);
        return StaticBufferInfo<Item>{.buffer = buffers[bufferIndex],
                                      .numItems = numItems[bufferIndex]};
    }

    auto getBufferInfos() const noexcept {
        return std::views::transform(std::views::zip(buffers, numItems),
                                     [](const auto& zip) {
                                         return StaticBufferInfo<Item>{
                                             .buffer = std::get<0>(zip),
                                             .numItems = std::get<1>(zip),
                                         };
                                     });
    }

   private:
    void checkIndex(size_t index) const noexcept {
        if (index >= buffers.size()) {
            std::println(std::cerr,
                         "StaticBufferVec::getBuffer: invalid buffer index");
            std::abort();
        }
    }

    std::vector<GLuint> buffers;
    std::vector<uint32_t> numItems;
};