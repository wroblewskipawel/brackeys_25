#pragma once

#include <cstddef>
#include <type_traits>

template <typename Type>
concept HashableType = requires(const Type& value) {
    { std::hash<Type>{}(value) } -> std::convertible_to<std::size_t>;
};

template <HashableType Type>
std::size_t hashValue(const Type& item) {
    return std::hash<Type>{}(item);
}
