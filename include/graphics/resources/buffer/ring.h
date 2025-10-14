#pragma once

#include <array>
#include <iostream>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

struct GenerationIndices {
    size_t generation{0};
    size_t bufferIndex{0};
};

template <typename Type>
struct BufferAllocation {
    size_t physicalBufferIndex;
    size_t numInstances;
    size_t bufferOffset;
    size_t generation;

    void join(const BufferAllocation& other) noexcept {
        if (!canJoin(other)) {
            std::println(std::cerr,
                         "BufferAllocation::join: Invalid BufferAllocation "
                         "join operation");
            std::abort();
        }
        numInstances += other.numInstances;
    }

    bool tryJoin(const BufferAllocation& other) noexcept {
        if (canJoin(other)) {
            numInstances += other.numInstances;
            return true;
        }
        return false;
    }

    bool canJoin(const BufferAllocation& other) const noexcept {
        if (generation == other.generation &&
            physicalBufferIndex == other.physicalBufferIndex &&
            (bufferOffset + numInstances) == other.bufferOffset) {
            return true;
        }
        return false;
    }

    BufferAllocation takeFirst(size_t numTake) noexcept {
        auto chunk = BufferAllocation{
            .physicalBufferIndex = physicalBufferIndex,
            .numInstances = numTake,
            .bufferOffset = bufferOffset,
            .generation = generation,
        };
        numInstances -= numTake;
        bufferOffset += numTake;
        return chunk;
    }

    auto getBufferIndices() const noexcept {
        return GenerationIndices{
            .generation = generation,
            .bufferIndex = physicalBufferIndex,
        };
    }
};

template <typename Type>
class PageVector {
   public:
    PageVector(size_t pageSize) noexcept : pageSize(pageSize), numPages(0) {}

    PageVector(const PageVector&) = default;
    PageVector& operator=(const PageVector&) = default;

    PageVector(PageVector&&) = default;
    PageVector& operator=(PageVector&&) = default;

    ~PageVector() = default;

    void reserve(size_t numReserve) noexcept {
        dataStorage.reserve(numReserve * pageSize);
    }

    void resize(size_t newSize) noexcept {
        numPages = newSize;
        dataStorage.resize(numPages * pageSize);
    }

    auto size() const noexcept { return numPages; }

    auto operator[](size_t pageIndex) noexcept {
        auto [begin, end] = rangeIndices(*this, pageIndex);
        return std::ranges::subrange(begin, end);
    }

    auto operator[](size_t pageIndex) const noexcept {
        auto [begin, end] = rangeIndices(*this, pageIndex);
        return std::ranges::subrange(begin, end);
    }

    const size_t pageSize;

   private:
    template<typename Vector>
    friend auto rangeIndices(Vector&&, size_t) noexcept;

    std::vector<Type> dataStorage;
    size_t numPages;
};

template<typename Vector>
auto rangeIndices(Vector&& vector, size_t pageIndex) noexcept {
    if (pageIndex >= vector.numPages) {
            std::println(std::cerr,
                         "PageVector::getPage: pageIndex out of range");
            std::abort();
        }
    auto storageOffset = pageIndex * vector.pageSize;
    return std::pair{vector.dataStorage.begin() + storageOffset,
                     vector.dataStorage.begin() + storageOffset + vector.pageSize};
}

template <typename Type>
class DynamicRing {
   public:
    DynamicRing(size_t pageSize, size_t reserveNumPages = 0,
                size_t initialRingSize = 3) noexcept
        : dataStorage(pageSize), isWraparound(false) {
        dataStorage.reserve(initialRingSize + reserveNumPages);
        dataCount.reserve(initialRingSize + reserveNumPages);
        dataGeneration.reserve(initialRingSize + reserveNumPages);
        allocateBuffers(initialRingSize);
    }

    DynamicRing(const DynamicRing&) = delete;
    DynamicRing& operator=(const DynamicRing&) = delete;

    DynamicRing(DynamicRing&&) noexcept = default;
    DynamicRing& operator=(DynamicRing&&) noexcept = default;

    ~DynamicRing() noexcept = default;

    auto pageSize() const noexcept { return dataStorage.pageSize; }

    auto nextGeneration() noexcept {
        previousGeneration = currentGeneration;
        currentGeneration.generation += 1;
        isWraparound = false;
        nextRingBuffer();
        return currentGeneration;
    }

    auto pushData(const Type& item) noexcept {
        return pushDataContiguous(std::views::single(item));
    }

    template <typename Range>
        requires std::is_convertible_v<std::ranges::range_value_t<Range>, Type>
    auto pushData(Range&& range) noexcept {
        auto numToCopy = static_cast<size_t>(std::ranges::distance(range));
        if (numToCopy == 0) {
            std::println(
                std::cerr,
                "DynamicRing::pushData: Attempted to allocate empty range");
            std::abort();
        }
        auto rangeBegin = range.begin();
        auto allocations = createAllocationBuffer(numToCopy);
        for (auto& allocation : allocations) {
            auto writeSize = std::min(numToCopy, getCurrentFree());
            allocation = writeDataUnchecked(
                std::ranges::subrange(rangeBegin, rangeBegin + writeSize));
            updateCurrentCount(writeSize);
            rangeBegin += writeSize;
            numToCopy -= writeSize;
        }
        return allocations;
    }

    template <typename Range>
        requires std::is_convertible_v<std::ranges::range_value_t<Range>, Type>
    auto pushDataContiguous(Range&& range) noexcept {
        auto numToCopy = static_cast<size_t>(std::ranges::distance(range));
        if (numToCopy == 0) {
            std::println(std::cerr,
                         "DynamicRing::pushDataContiguous: Attempted to "
                         "allocate empty range");
            std::abort();
        }
        if (numToCopy > dataStorage.pageSize) {
            std::println(std::cerr,
                         "DynamicRing::pushDataContiguous: page size not "
                         "sufficient for contiguous allocation request");
            std::abort();
        }
        if (std::min(numToCopy, getCurrentFree()) != numToCopy) {
            nextBuffer();
        }
        auto allocation = writeDataUnchecked(std::forward<Range>(range));
        updateCurrentCount(numToCopy);
        return allocation;
    }

    const auto getBufferRange(GenerationIndices bufferIndex) const noexcept {
        if (bufferIndex.bufferIndex >= dataStorage.size()) {
            std::println(
                std::cerr,
                "DynamicRing::getBufferRange: Out of range buffer access");
            std::abort();
        }

        if (bufferIndex.generation != dataGeneration[bufferIndex.bufferIndex]) {
            std::println(std::cerr,
                         "DynamicRing::getBufferRange: Buffer access with "
                         "invalid generation bufferIndex");
            std::abort();
        }

        auto rangeBegin = dataStorage[bufferIndex.bufferIndex].begin();
        return std::ranges::subrange(
            rangeBegin, rangeBegin + dataCount[bufferIndex.bufferIndex]);
    }

   private:
    template <typename Range>
        requires std::is_convertible_v<std::ranges::range_value_t<Range>, Type>
    auto writeDataUnchecked(Range&& range) noexcept {
        auto numInstances = static_cast<size_t>(std::ranges::distance(range));
        auto bufferOffset = getCurrentCount();
        auto writeBegin = getWriteHead();
        std::ranges::copy(range, writeBegin);
        return BufferAllocation<Type>{
            .physicalBufferIndex = currentGeneration.bufferIndex,
            .numInstances = numInstances,
            .bufferOffset = bufferOffset,
            .generation = currentGeneration.generation};
    }

    auto getCurrentCount() const noexcept {
        return dataCount[currentGeneration.bufferIndex];
    }

    auto getCurrentFree() const noexcept {
        return dataStorage.pageSize - getCurrentCount();
    }

    auto createAllocationBuffer(size_t numData) const noexcept {
        size_t additionalBuffers =
            (numData > getCurrentFree())
                ? (numData - getCurrentFree() + (dataStorage.pageSize - 1)) /
                      dataStorage.pageSize
                : 0;
        return std::vector<BufferAllocation<Type>>(1 + additionalBuffers);
    }

    auto getWriteHead() noexcept {
        return dataStorage[currentGeneration.bufferIndex].begin() +
               dataCount[currentGeneration.bufferIndex];
    }

    auto checkWraparound() noexcept {
        isWraparound = isWraparound || currentGeneration.bufferIndex ==
                                           previousGeneration.bufferIndex;
        return isWraparound;
    }

    auto appendBuffer() noexcept {
        allocateBuffers(1);
        currentGeneration.bufferIndex = dataStorage.size() - 1;
    }

    void nextRingBuffer() noexcept {
        currentGeneration.bufferIndex =
            (currentGeneration.bufferIndex + 1) % dataCount.size();
        dataCount[currentGeneration.bufferIndex] = 0;
        dataGeneration[currentGeneration.bufferIndex] =
            currentGeneration.generation;
    }

    void nextBuffer() noexcept {
        if (checkWraparound()) {
            appendBuffer();
        } else {
            nextRingBuffer();
        }
    }

    // TODO: This being used after the buffer write, may cause empty buffer
    // being handled as if it was used by previous generation in following
    // scenario pushData writes enough data to fully fill current buffer
    // updatecurrentCount proceeds to next buffer, its count is set as 0
    // nextGeneration is called, proceeding to next buffer, despite no data was
    // written in previous generation to its last would-be-used buffer
    // This shouldn't cause visibe issues, apart from using more resources that
    // would be needed, could also resoult in empty ranges being returned by
    // getBufferRange
    void updateCurrentCount(size_t numData) noexcept {
        dataCount[currentGeneration.bufferIndex] += numData;
        if (dataCount[currentGeneration.bufferIndex] == dataStorage.pageSize) {
            nextBuffer();
        }
    }

    void allocateBuffers(size_t numBuffers) noexcept {
        auto currentSize = dataStorage.size();
        dataStorage.resize(currentSize + numBuffers);
        dataCount.resize(currentSize + numBuffers, 0);
        dataGeneration.resize(currentSize + numBuffers,
                              currentGeneration.generation);
    }

    PageVector<Type> dataStorage;
    std::vector<size_t> dataCount;
    std::vector<size_t> dataGeneration;
    GenerationIndices currentGeneration;
    GenerationIndices previousGeneration;
    bool isWraparound;
};
