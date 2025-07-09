#pragma once
#include <string_view>
#include <cstddef>
#include <span>
#include <format>
#include <algorithm>
#include <cctype>

#include <rsl/util/to_string.hpp>

#include <rsl/json5/serializer.hpp>

namespace rsl::_cli_impl {
struct ArgParser {
  std::span<std::string_view> args;
  std::size_t cursor = 0;
  std::string error;

  std::string_view current() const { return args[cursor]; }

  template <typename... Args>
  void set_error(std::format_string<Args...> fmt, Args&&... args) {
    error = std::format(fmt, std::forward<Args>(args)...);
  }
  [[nodiscard]] bool has_error() const { return !error.empty(); }
  [[nodiscard]] bool valid() const { return cursor < args.size() && !has_error(); }
};

template <typename T>
T parse_value(std::string_view value) {
  if constexpr (std::same_as<T, std::string>) {
    return std::string(value);
  } else if constexpr (std::same_as<T, bool>) {
    // allow a few more ways to spell true/false
    // TODO move out of header
    std::string lower_value;
    std::ranges::transform(value, lower_value.begin(), [](char c) {return std::tolower(c); });
    if (lower_value == "1" || lower_value == "y" || lower_value == "yes" || lower_value == "on" ||
        lower_value == "true") {
      return true;
    } else if (lower_value == "0" || lower_value == "n" || lower_value == "no" ||
               lower_value == "off" || lower_value == "false") {
      return false;
    } else {
      throw std::runtime_error("could not parse as bool");
    }
  } else {
    return json5::Serializer<T>::deserialize(json5::Value{std::string(value)});
  }
}
}  // namespace rsl::_cli_impl