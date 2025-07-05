#pragma once
#include <string_view>
#include <cstddef>
#include <span>
#include <format>
#include <rsl/util/to_string.hpp>

namespace rsl::_cli_impl {
struct ArgParser {
  std::span<std::string_view> args;
  std::size_t cursor = 0;

  std::string_view current() const { return args[cursor]; }

  template <typename... Args>
  [[noreturn]] void fail(std::format_string<Args...> fmt, Args&&... args) const {
    std::println(fmt, std::forward<Args>(args)...);
    std::exit(1);
  }

  [[nodiscard]] bool valid() const noexcept { return cursor < args.size(); }
};

template <typename T>
T parse_value(std::string_view value) {
  // TODO parse more types
  if constexpr (std::is_integral_v<T>) {
    // TODO parse signed, bool etc
    return util::stou(value);
  } else {
    return T{value};
  }
}
}  // namespace rsl::_cli_impl