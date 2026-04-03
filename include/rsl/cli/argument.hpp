#pragma once

#include <string_view>
#include <vector>
#include <meta>
#include <print>

#include <rsl/span>
#include <rsl/string_view>
#include <rsl/string_constant>
#include <rsl/expect>

#include "accessor.hpp"
#include "serialize.hpp"
#include "annotations.hpp"
#include <rsl/_impl/default_construct.hpp>
#include <rsl/_impl/prettify.hpp>

namespace rsl::_cli_impl {

struct Argument {
  struct Unevaluated {
    using handler_type = void (Unevaluated::*)(void*) const;

    handler_type handle = nullptr;
    std::string_view argument;

    void operator()(void* args) const { (this->*handle)(args); }

    // template <std::meta::info R, typename T>
    // void validate(T value) const {
    //   auto parent = display_string_of(parent_of(R));
    //   if constexpr (rsl::meta::has_annotation(R, ^^rsl::_expect_impl::Expect)) {
    //     constexpr static auto constraint = [:constant_of(meta::get_annotation(
    //                                              R,
    //                                              ^^rsl::_expect_impl::Expect)):];

    //     std::vector<std::string> failed_terms;
    //     if (constraint.eval_verbose(std::tuple{value}, failed_terms)) {
    //       return;
    //     }

    //     std::println("Validation failed for argument `{}` of `{}`", identifier_of(R), parent);
    //     // for (auto&& term : failed_terms | std::views::reverse) {
    //     //   std::println("    => {}{}{} evaluated to {}false{}",
    //     //                fg::Blue,
    //     //                term,
    //     //                style::Reset,
    //     //                fg::Red,
    //     //                style::Reset);
    //     // }
    //     std::exit(1);
    //   }
    // }

    template <std::size_t Idx, typename ValueT, typename ArgT>
    void do_handle(void* arguments) const {
      auto value = parse_value<ValueT>(argument);
      //   validate<ValueT>(value);
      get<Idx>(*static_cast<ArgT*>(arguments)) = value;
    }
  };

  Unevaluated::handler_type _impl_handler = nullptr;

  std::size_t index;
  rsl::string_view name;
  rsl::string_view type;
  rsl::string_view description;
  rsl::string_view constraint;
  bool is_optional = false;
  bool is_variadic = false;  // currently never set

  std::optional<Unevaluated> parse(ArgParser& parser) {
    if (!parser.valid()) {
      return {};
    }

    auto current = parser.current();
    if (current[0] == '-' && !(current.size() > 1 && current[1] >= '0' && current[1] <= '9')) {
      // this is probably an option, bail out
      return {};
    }
    ++parser.cursor;

    return Unevaluated{.handle = _impl_handler, .argument = current};
  }

  // template <std::meta::info R, auto Constraint>
  // constexpr char const* stringify_constraint() {
  //   std::string _constraint = Constraint.to_string(identifier_of(R));
  //   // strip outer parenthesis and make static
  //   return std::define_static_string(_constraint.substr(1, _constraint.size() - 2));
  // }

  consteval Argument(std::size_t idx, std::meta::info r, Accessor const& access)
      : index(idx)
      , name(std::define_static_string(identifier_of(r)))
      , type(std::define_static_string(pretty_type(r)))
      , is_optional(has_default_member_initializer(r)) {
    if (auto desc = rsl::meta::get_annotation<annotations::Description>(r); desc) {
      description = desc->data;
    }

    // if (rsl::meta::has_annotation(r, ^^rsl::_expect_impl::Expect)) {
    //   auto annotation = rsl::meta::get_annotation<rsl::_expect_impl::Expect>(r);
    //   constraint      = (this->*extract<char const* (Argument::*)()>(substitute(
    //                            ^^stringify_constraint,
    //                            {reflect_constant(r), constant_of(annotation)})))();
    // }
    auto arg_type = substitute(^^_cli_impl::ArgumentTuple, {parent_of(r)});
    _impl_handler = extract<Unevaluated::handler_type>(
        substitute(^^Unevaluated::do_handle,
                   {std::meta::reflect_constant(idx), 
                    remove_cvref(type_of(r)),
                    arg_type}));
  }
};
}  // namespace rsl::_cli_impl