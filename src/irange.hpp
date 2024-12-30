#pragma once
#include <ranges>

// TODO Not working
template<std::semiregular Bound>
auto irange(Bound b)
{
     return std::ranges::iota_view(0, b);
}
