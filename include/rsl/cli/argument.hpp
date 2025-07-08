#pragma once

#include <string_view>
#include <vector>
#include <meta>

#include <rsl/span>
#include <rsl/string_view>
#include <rsl/string_constant>
#include <rsl/expect>

#include "parser.hpp"
#include "annotations.hpp"
#include <rsl/_impl/default_construct.hpp>

namespace rsl::_cli_impl {

struct Argument {
  struct Unevaluated {
    using handler_type = void (Unevaluated::*)(void*) const;

    handler_type handle = nullptr;
    std::string_view argument;

    void operator()(void* args) const { (this->*handle)(args); }

    template <std::meta::info R, typename T>
    void validate(T value) const {
      auto parent = display_string_of(parent_of(R));
      if constexpr (rsl::meta::has_annotation(R, ^^rsl::_expect_impl::Expect)) {
        constexpr static auto constraint = [:constant_of(meta::get_annotation(
                                                 R,
                                                 ^^rsl::_expect_impl::Expect)):];

        std::vector<std::string> failed_terms;
        if (constraint.eval_verbose(std::tuple{value}, failed_terms)) {
          return;
        }

        std::println("Validation failed for argument `{}` of `{}`", identifier_of(R), parent);
        // for (auto&& term : failed_terms | std::views::reverse) {
        //   std::println("    => {}{}{} evaluated to {}false{}",
        //                fg::Blue,
        //                term,
        //                style::Reset,
        //                fg::Red,
        //                style::Reset);
        // }
        std::exit(1);
      }
    }

    template <std::size_t Idx, std::meta::info R>
    void do_handle(void* arguments) const {
      using parent = [:is_function_parameter(R) ? type_of(parent_of(R)) : parent_of(R):];

      using type = [:remove_cvref(type_of(R)):];
      auto value = parse_value<type>(argument);
      //   validate<R>(value);

      using arg_tuple                               = _impl::ArgumentTuple<parent>;
      get<Idx>(*static_cast<arg_tuple*>(arguments)) = value;
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

  template <std::meta::info R, auto Constraint>
  constexpr char const* stringify_constraint() {
    std::string _constraint = Constraint.to_string(identifier_of(R));
    // strip outer parenthesis and make static
    return std::define_static_string(_constraint.substr(1, _constraint.size() - 2));
  }

  consteval Argument(std::size_t idx, std::meta::info reflection)
      : index(idx)
      , name(std::define_static_string(identifier_of(reflection)))
      , type(std::define_static_string(
            display_string_of(type_of(reflection))))  // TODO clean at this point?
      , is_optional(is_function_parameter(reflection)
                        ? has_default_argument(reflection)
                        : has_default_member_initializer(reflection)) {
    if (auto desc = annotation_of_type<annotations::Description>(reflection); desc) {
      description = desc->data;
    }

    if (rsl::meta::has_annotation(reflection, ^^rsl::_expect_impl::Expect)) {
      auto annotation = rsl::meta::get_annotation(reflection, ^^rsl::_expect_impl::Expect);
      constraint      = (this->*extract<char const* (Argument::*)()>(substitute(
                               ^^stringify_constraint,
                               {reflect_constant(reflection), constant_of(annotation)})))();
    }

    _impl_handler = extract<Unevaluated::handler_type>(
        substitute(^^Unevaluated::do_handle,
                   {std::meta::reflect_constant(idx), reflect_constant(reflection)}));
  }
};
}  // namespace rsl::_cli_impl