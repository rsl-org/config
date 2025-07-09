#pragma once
#include <meta>
#include <string>
#include <string_view>

namespace rsl::_cli_impl {
consteval std::string_view pretty_type(std::meta::info r) {
  if (!is_type(r)) {
    r = type_of(r);
  }

  if (r == dealias(^^std::string) || r == dealias(^^std::string_view)) {
    return "string";
  }

  return display_string_of(r);
}
}  // namespace rsl::_cli_impl