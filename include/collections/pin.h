#pragma once

#include <iostream>

template <typename Item, typename Ownership>
class SlotMap;

template <typename Key, typename Item>
struct MapVector;

template <typename Item>
class PinRef {
   public:
    static PinRef null() noexcept { return PinRef(nullptr); }

    const Item& get() const noexcept {
        if (itemRef == nullptr) {
            std::println(std::cerr, "Ref: get called on null reference");
            std::abort();
        }
        return *itemRef;
    }

    Item& get() noexcept {
        if (itemRef == nullptr) {
            std::println(std::cerr, "Ref: get called on null reference");
            std::abort();
        }
        return *itemRef;
    }

    [[nodiscard]] bool isValid() const noexcept { return itemRef != nullptr; }

    PinRef(const PinRef&) = delete;
    PinRef& operator=(const PinRef&) = delete;
 
    PinRef(PinRef&&) = delete;
    PinRef& operator=(PinRef&&) = delete;

   private:
    template <typename, typename>
    friend class SlotMap;
    template <typename, typename>
    friend class MapVector;

    PinRef(Item* itemRef) : itemRef(itemRef) {};


    Item* itemRef;
};

template <typename Item>
class PinVal {
   public:
    PinVal(const Item& item) : item(item) {};
    PinVal(Item&& item) : item(std::move(item)) {};

    PinVal(const PinVal&) = delete;
    PinVal& operator=(const PinVal&) = delete;

    PinVal(PinVal&&) = delete;
    PinVal& operator=(PinVal&&) = delete;

    const Item& get() const noexcept { return item; }

    Item& get() noexcept { return item; }

   private:
    Item item;
};
