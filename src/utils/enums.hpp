#pragma once
#include <type_traits>

namespace utils {

template <typename T>
concept Enum = std::is_enum_v<std::decay_t<T>>;

template <Enum EnumType>
constexpr auto to_underlying(EnumType value) {
    return std::underlying_type_t<EnumType>(value);
}

} 
