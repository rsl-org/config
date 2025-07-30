#pragma once
#include <meta>
#include <rsl/assert>
#include <vector>

namespace rsl::_cli_impl {

struct Accessor {
  std::meta::info root;
  std::vector<std::meta::info> subobjects;

  consteval Accessor(std::meta::info r) : root(r) {
    constexpr_assert(is_type(r) ||
                     (is_function(r) && (!is_class_member(r)) || is_static_member(r)));
  }

  template <std::meta::info M, std::meta::info... Ms>
  static auto& subobject_accessor(auto& obj) {
    if constexpr (sizeof...(Ms) != 0) {
      return subobject_accessor<Ms...>(obj.[:M:]);
    } else {
      return obj.[:M:];
    }
  }

  static auto& subobject_accessor(auto& obj) {
    return obj;
  }

  template <std::meta::info R, std::meta::info... Ms>
  static auto& access(void* obj) {
    if constexpr (is_type(R)) {
      return subobject_accessor<Ms...>(*static_cast<typename[:R:]*>(obj));
    } else {
      return subobject_accessor<Ms...>([:R:]());
    }
  }

  consteval explicit(false) operator std::meta::info() const {
    std::vector args = {reflect_constant(root)};
    for (auto sub : subobjects) {
      args.push_back(reflect_constant(sub));
    }
    return substitute(^^access, args);
  }
};

}  // namespace rsl::_cli_impl