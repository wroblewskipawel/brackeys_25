#pragma once

#include <ranges>
#include <type_traits>

template <typename Range, typename Type>
concept RefConstRange =
    std::is_convertible_v<std::ranges::range_value_t<Range>, const Type&>;
