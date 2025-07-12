#pragma once
#include <optional>
#include <string_view>
#include <experimental/meta>

#include <rsl/string_view>
#include <rsl/expect>

namespace rsl::annotations {
constexpr inline struct Option {} option {};
constexpr inline struct Positional {} positional {};
constexpr inline struct Flag {} flag {};

namespace _impl {
struct StringAnnotation {
  rsl::string_view data;

  consteval StringAnnotation() = default;
  consteval explicit StringAnnotation(std::string_view text) : data(define_static_string(text)) {}
  
  consteval StringAnnotation operator()(std::string_view text) {
    data = define_static_string(text);
    return *this;
  }
  
  constexpr operator std::string_view() const {
    return {data.data(), data.size()};
  }
  
  friend consteval bool operator==(StringAnnotation const& self, std::optional<StringAnnotation> const& other) {
    if (!other.has_value()) {
      return false;
    }
    return self.data == other->data;
  }
};
}

struct Shorthand : _impl::StringAnnotation {
    using _impl::StringAnnotation::StringAnnotation;
};

struct Description : _impl::StringAnnotation {
    using _impl::StringAnnotation::StringAnnotation;
};
}  // namespace rsl::annotations