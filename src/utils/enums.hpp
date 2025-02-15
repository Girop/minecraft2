#pragma once
#include <type_traits>

namespace utils {

template <typename T>
concept Enum = std::is_enum_v<std::decay_t<T>>;

template <Enum EnumT>
constexpr auto to_underlying(EnumT value) {
    return std::underlying_type_t<EnumT>(value);
}

} 
