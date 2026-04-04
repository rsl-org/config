#pragma once
#include <string_view>
#include <cstddef>
#include <span>
#include <format>
#include <algorithm>
#include <cctype>

#include <rsl/serialize>

#include <rsl/json5/serializer.hpp>

namespace rsl::_cli_impl {
struct ArgParser {
  std::span<std::string_view> args;
  std::size_t cursor = 0;
  std::string error;

  std::string_view current() const { return cursor < args.size() ? args[cursor] : ""; }

  template <typename... Args>
  void set_error(std::format_string<Args...> fmt, Args&&... args) {
    error = std::format(fmt, std::forward<Args>(args)...);
  }
  [[nodiscard]] bool has_error() const { return !error.empty(); }
  [[nodiscard]] bool valid() const { return cursor < args.size() && !has_error(); }
};

bool parse_bool(std::string_view value);

template <typename T>
T parse_value(std::string_view value) {
  if constexpr (std::same_as<T, std::string>) {
    return std::string(value);
  } else {
    // TODO use rsl::repr instead?
    return json5::Serializer<T>::deserialize(json5::Value{std::string(value)});
  }
}
}  // namespace rsl::_cli_impl