#pragma once

#include <meta>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <tuple> // use rsl::tuple?
#include <utility>
#include <stdexcept>

#include <rsl/meta_traits>

namespace rsl::_impl {
template <std::meta::info R>
consteval std::meta::info make_arg_tuple() {
  std::vector<std::meta::info> args;
  auto make_optional = [](auto r) {
    return substitute(^^std::optional, {
                                           r});
  };

  if constexpr (is_function(R)) {
    for (auto& arg : parameters_of(R)) {
      args.push_back(make_optional(type_of(arg)));
    }
  } else if constexpr (is_function_type(R)) {
    for (auto&& arg : parameters_of(dealias(R))) {
      // turns out parameters_of of a reflection of a function type
      // returns a reflection of a type, not a reflection of a parameter
      args.push_back(make_optional(arg));
    }
  } else if constexpr (is_class_type(R)) {
    constexpr auto ctx = std::meta::access_context::current();
    for (auto&& base : bases_of(dealias(R), ctx)) {
      args.push_back(extract<std::meta::info (*)()>(
          substitute(^^make_arg_tuple, {
                                           reflect_constant(type_of(base))}))());
    }
    for (auto&& arg : nonstatic_data_members_of(R, ctx)) {
      args.push_back(make_optional(type_of(arg)));
    }
  } else {
    throw "unsupported reflection type";
  }

  return substitute(^^std::tuple, args);
}

template <typename T>
using ArgumentTuple = [:make_arg_tuple<^^T>():];

namespace _default_impl {
consteval std::size_t required_args_count(std::meta::info reflection) {
  if (is_function(reflection)) {
    auto members = parameters_of(reflection);
    return std::count_if(members.begin(), members.end(),
                         [](auto x) { return !has_default_argument(x); });
  } else if (is_type(reflection)) {
    auto members = nonstatic_data_members_of(reflection, std::meta::access_context::current());
    return std::count_if(members.begin(), members.end(),
                         [](auto x) { return !has_default_member_initializer(x); });
  } else {
    return {};
  }
}

template <std::size_t Offset, std::size_t Max, typename F, typename... Args>
decltype(auto) do_visit(F visitor, std::size_t index, Args&&... extra_args) {
  template for (constexpr auto Idx : std::views::iota(0zu, Max)) {
    if (Idx == index) {
      return visitor(std::make_index_sequence<Offset + Idx>(), std::forward<Args>(extra_args)...);
    }
  }
  throw std::runtime_error("invalid");
}

template <std::size_t Bases, std::size_t Required, typename T, typename F, typename... Args>
decltype(auto) visit(F visitor, ArgumentTuple<T> const& args, Args&&... extra_args) {
  constexpr static int size = std::tuple_size_v<ArgumentTuple<T>>;
  constexpr static int optional_min = Bases + Required;
  constexpr static int branches = size - optional_min;

  std::size_t index = 0;
  template for(constexpr auto Idx : std::views::iota(0zu, size - Bases)) {
    if (get<Bases + Idx>(args).has_value()) {
      ++index;
    } else {
      break;
    }
  }

  if (index < Required) {
    // fail more gracefully
    throw std::runtime_error("not all required arguments are given");
  }

  if constexpr (branches == 0) {
    // no optional arguments
    return visitor(std::make_index_sequence<size - Bases>(), std::forward<Args>(extra_args)...);
  } else {
    return do_visit<Required, branches>(
        visitor, index - Required, std::forward<Args>(extra_args)...);
  }
}

}  // namespace _default_impl

template <std::meta::info R>
constexpr inline std::size_t required_arg_count = _default_impl::required_args_count(R);

template <typename T>
constexpr inline std::size_t base_count = bases_of(^^T, std::meta::access_context::current()).size();

template <typename T>
  requires(std::is_aggregate_v<T> && !std::is_array_v<T>)
T default_construct(ArgumentTuple<T> const& args) {
  constexpr static auto num_bases = base_count<T>;
  constexpr static auto ctx = std::meta::access_context::current();

  return _default_impl::visit<num_bases, required_arg_count<dealias(^^T)>, T>(
      [&]<std::size_t... Idx, std::size_t... BIdx>(std::index_sequence<Idx...>,
                                                   std::index_sequence<BIdx...>) {
        return T{default_construct<typename [:type_of(bases_of(^^T, ctx)[BIdx]):]>(get<BIdx>(args))...,
                 *get<num_bases + Idx>(args)...};
      },
      args, std::make_index_sequence<num_bases>());
}

template <std::meta::info R>
  requires(meta::function<R> || meta::static_member_function<R>)
decltype(auto) default_invoke(ArgumentTuple<typename [:type_of(R):]> const& args) {
  return _default_impl::visit<0, required_arg_count<R>, typename [:type_of(R):]>(
      [&]<std::size_t... Idx>(std::index_sequence<Idx...>) { return [:R:](*get<Idx>(args)...); },
      args);
}

template <std::meta::info R>
  requires(meta::nonstatic_member_function<R>)
decltype(auto) default_invoke() =
    delete ("calling a member function requires an object of its parent type");

template <std::meta::info R, typename T>
  requires(meta::nonstatic_member_function<R>)
decltype(auto) default_invoke(T&& self, ArgumentTuple<typename [:type_of(R):]> const& args) {
  static_assert(std::convertible_to<std::remove_cvref_t<T>, typename [:parent_of(R):]>,
                "wrong type for self argument");

  return _default_impl::visit<0, required_arg_count<R>, typename [:type_of(R):]>(
      [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
        std::forward<T>(self).[:R:](*get<Idx>(args)...);
      },
      args);
}

}  // namespace rsl::_impl