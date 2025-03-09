#pragma once
#include <type_traits>
#include <vulkan/vk_enum_string_helper.h>
#include <exception>
#include "log.hpp"

template <>
struct fmt::formatter<std::source_location>: formatter<std::string_view> {
    auto format(std::source_location location, format_context& ctx) const -> format_context::iterator {
        auto name =fmt::format("filename: {}, function: {}, line: {}, column: {}",
            location.file_name(),
            location.function_name(),
            location.column(),
            location.line()
        );

      return formatter<string_view>::format(name, ctx);
  }
};

#define CONST_GETTER(field)\
    [[nodiscard]] auto const& field() const {\
        return field##_;\
    }

#define GETTER(field) \
    [[nodiscard]] auto& field() { \
        return field##_; \
    } \
    CONST_GETTER(field)

namespace utils 
{

class FatalError : std::exception
{
public:
    explicit FatalError(std::source_location location = std::source_location::current()):
        location{location} {}

    std::string where() const noexcept 
    {
        return fmt::format("{}", location);
    }
private: 
    std::source_location location;
};

#define fail(msg, ...) do { \
    error(msg, ##__VA_ARGS__); \
    throw ::utils::FatalError{}; \
} while(false) 

inline void check_vk(VkResult const result) 
{
    if (result == VK_SUCCESS) return;
    fail("Vulkan operation failed: {}", string_VkResult(result));
}


template <typename T>
concept Enum = std::is_enum_v<std::decay_t<T>>;

template <Enum EnumType>
constexpr auto to_underlying(EnumType value) {
    return std::underlying_type_t<EnumType>(value);
}

} // namespace utils


