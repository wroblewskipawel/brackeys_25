#pragma once

#include <ranges>
#include <type_traits>

template <typename Range, typename Type>
concept RefConstRange =
    std::is_convertible_v<std::ranges::range_value_t<Range>, const Type&>;

template <typename Range, typename Type>
concept ContiguousRefConstRange =
    RefConstRange<Range, Type> && std::ranges::contiguous_range<Range>;

template <typename Range, typename Type>
concept ContiguousRefConstRangeRange =
    RefConstRange<std::ranges::range_value_t<Range>, Type> &&
    std::ranges::contiguous_range<std::ranges::range_value_t<Range>>;
