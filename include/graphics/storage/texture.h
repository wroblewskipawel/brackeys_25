#pragma once

#include "collections/slot_map.h"

class TextureData;
class TextureDataHandle;

TextureDataHandle registerTextureData(TextureData&&) noexcept;

class TextureDataStorage {
   private:
    friend TextureDataHandle registerTextureData(TextureData&&) noexcept;
    friend class TextureData;
    friend class TextureDataHandle;

    inline static SlotMap<TextureData, Shared> textureStorage;
};

class TextureDataHandle {
   public:
    TextureDataHandle& operator=(const TextureDataHandle& other) noexcept {
        TextureDataStorage::textureStorage.pop(std::move(handle));
        handle = other.handle.copyHandle(TextureDataStorage::textureStorage);
        return *this;
    };

    TextureDataHandle(TextureDataHandle&&) = default;
    TextureDataHandle& operator=(TextureDataHandle&&) = default;

    ~TextureDataHandle() {
        TextureDataStorage::textureStorage.pop(std::move(handle));
    }

    bool operator==(const TextureDataHandle& other) const noexcept {
        return handle == other.handle;
    }

    const Ref<TextureData, Shared> get() const noexcept {
        return TextureDataStorage::textureStorage.get(handle);
    }

    Ref<TextureData, Shared> get() noexcept {
        return TextureDataStorage::textureStorage.get(handle);
    }

    static TextureDataHandle getInvalid() noexcept {
        return TextureDataHandle(Handle<TextureData, Shared>::getInvalid());
    }

    TextureDataHandle copy() const noexcept { return TextureDataHandle(*this); }

    bool isInvalid() const noexcept { return handle.isInvalid(); }

   private:
    friend TextureDataHandle registerTextureData(TextureData&&) noexcept;

    TextureDataHandle(Handle<TextureData, Shared>&& handle)
        : handle(std::move(handle)) {}
    TextureDataHandle(const TextureDataHandle& handle) noexcept
        : handle(handle.handle.copyHandle(TextureDataStorage::textureStorage)) {
    }
    Handle<TextureData, Shared> handle;
};

inline TextureDataHandle registerTextureData(TextureData&& texture) noexcept {
    auto handle =
        TextureDataStorage::textureStorage.emplace(std::move(texture));
    return TextureDataHandle(std::move(handle));
}
