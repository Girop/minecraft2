#pragma once
#include <source_location>
#include <fmt/format.h>

[[noreturn]] inline void fail(
    std::string const& message = "Encountered critical error",
    std::source_location loc = std::source_location::current()
) {
    auto const location = fmt::format("[{} - {} - {}]", loc.file_name(), loc.function_name(), loc.line());
    fmt::println(": {}\n{}", message, location);
    std::abort();
}
