#pragma once
#include <meta>
#include <vector>
#include <string_view>
#include <optional>

#include <rsl/string_view>
#include <rsl/string_constant>
#include <rsl/span>

#include "parser.hpp"
#include "annotations.hpp"
#include <rsl/_impl/default_construct.hpp>

namespace rsl::_cli_impl {

struct Option {
  struct Unevaluated {
    using handler_type = void (Unevaluated::*)(void*) const;
    handler_type handle;
    std::vector<Argument::Unevaluated> arguments;
    void operator()(void* object) const { (this->*handle)(object); }

    template <std::meta::info R>
    void do_handle(void* obj) const {
      _impl::ArgumentTuple<typename[:type_of(R):]> arg_tuple;
      for (auto arg : arguments) {
        arg(&arg_tuple);
      }
      if constexpr (meta::nonstatic_member_function<R>) {
        _impl::default_invoke<R>(*static_cast<[:parent_of(R):]*>(obj), arg_tuple);
      } else if constexpr (is_function(R)) {
        _impl::default_invoke<R>(arg_tuple);
      } else if constexpr (is_object_type(type_of(R))) {
        static_cast<[:parent_of(R):]*>(obj) -> [:R:] = *get<0>(arg_tuple);
      } else {
        static_assert(false, "Unsupported handler type.");
      }
    }
  };

  Unevaluated::handler_type _impl_handler = nullptr;
  rsl::string_view name;
  rsl::string_view description;
  rsl::span<Argument const> arguments;

  std::optional<Unevaluated> parse(ArgParser& parser) {
    auto opt_name = parser.current();
    if (!opt_name.starts_with("--")) {
      return {};
    }

    opt_name.remove_prefix(2);
    if (opt_name != name) {
      return {};
    }
    ++parser.cursor;  // parser will not unwind after this point

    Unevaluated option{.handle = _impl_handler};
    for (auto arg : arguments) {
      auto argument = arg.parse(parser);
      if (argument) {
        option.arguments.push_back(*argument);
      } else {
        if (!arg.is_optional) {
          parser.fail("Missing argument {} of option {}", arg.name, name);
          break;
        }
      }
    };

    return option;
  }

  consteval Option(std::string_view name, std::meta::info reflection)
      : name(std::define_static_string(name)) {
    if (auto desc = annotation_of_type<annotations::Description>(reflection); desc) {
      description = std::define_static_string(desc->data);
    }

    std::vector<Argument> args;
    std::size_t index = 0;
    if (is_object_type(type_of(reflection))) {
      args.emplace_back(index, reflection);
    } else {
      for (auto param : parameters_of(reflection)) {
        args.emplace_back(index++, param);
      }
    }

    arguments = std::define_static_array(args);
    _impl_handler   = extract<Unevaluated::handler_type>(
        substitute(^^Unevaluated::do_handle, {reflect_constant(reflection)}));
  }
};
}  // namespace rsl::_cli_impl