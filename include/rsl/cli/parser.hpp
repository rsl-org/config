#pragma once
#include <string_view>
#include <cstddef>
#include <span>
#include <format>
#include <rsl/util/to_string.hpp>

#include <rsl/json5/serializer.hpp>

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
  if constexpr (std::same_as<T, std::string>) {
    return std::string(value);
  } else {
    return json5::Serializer<T>::deserialize(json5::Value{std::string(value)});
  }
}
}  // namespace rsl::_cli_impl