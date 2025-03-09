#pragma once
#include <source_location>
#include <fmt/format.h>
#include <string_view>


enum class LogLevel : uint8_t {
    debug,
    info,
    warn,
    error,
    VIP
};

template <>
struct fmt::formatter<LogLevel>: formatter<std::string_view> {
    auto format(LogLevel c, format_context& ctx) const -> format_context::iterator {
      std::string_view name = "unknown";
      switch (c) {
        case LogLevel::debug: name = "DEBUG"; break;
        case LogLevel::info: name = "INFO"; break;
        case LogLevel::warn: name = "WARN"; break;
        case LogLevel::error: name = "ERROR"; break;
        case LogLevel::VIP: name = "VIP"; break;
      }
      return formatter<string_view>::format(name, ctx);
  }
};

#define LOG(msg, lvl, ...) do { \
    constexpr auto location {std::source_location::current()}; \
    /* fmt::print("[{} {}:{}:{}]", lvl, location.file_name(), location.function_name(), location.line()); */ \
    fmt::print("[{} {}:{}] ", lvl, location.file_name(), location.line()); \
    fmt::println(msg, ##__VA_ARGS__); \
} while (false);

#define debug(msg, ...) LOG(msg, LogLevel::debug, ##__VA_ARGS__)

#define info(msg, ...) LOG(msg, LogLevel::info, ##__VA_ARGS__)

#define warn(msg, ...) LOG(msg, LogLevel::warn, ##__VA_ARGS__)

#define error(msg, ...) LOG(msg, LogLevel::error, ##__VA_ARGS__)

#define VIP(msg, ...) LOG(msg, LogLevel::VIP, ##__VA_ARGS__)

