#pragma once
#include <meta>
#include <vector>
#include <string_view>

#include <rsl/string_view>
#include <rsl/string_constant>
#include <rsl/span>

#include "parser.hpp"
#include "annotations.hpp"
#include "../default_construct.hpp"

namespace rsl::_cli_impl {

struct Option {
  struct Unevaluated {
    using handler_type = void (Unevaluated::*)(void*) const;
    std::vector<Argument::Unevaluated> arguments;
    handler_type handle;
    void operator()(void* object) const { (this->*handle)(object); }

    template <std::meta::info R>
    void do_handle(void* obj) const {
      _impl::ArgumentTuple<typename [:type_of(R):]> arg_tuple;
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

    template <rsl::string_constant name, std::meta::info R, Argument... args>
    bool do_parse(ArgParser& parser) {
      static constexpr auto current_handler =
          extract<handler_type>(substitute(^^do_handle, {reflect_constant(R)}));
      auto arg_name = parser.current();
      arg_name.remove_prefix(2);
      if (arg_name != name) {
        return false;
      }
      ++parser.cursor;  // parser will not unwind after this point

      template for (constexpr auto Idx : std::views::iota(0zu, sizeof...(args))) {
        Argument::Unevaluated argument;
        if ((argument.*args...[Idx].parse)(parser)) {
          arguments.push_back(argument);
        } else {
          if (!args...[Idx].is_optional) {
            parser.fail("Missing argument {} of option {}", args...[Idx].name, arg_name);
            break;
          }
        }
      };

      handle = current_handler;
      return true;
    }
  };

  using parser_type = bool (Unevaluated::*)(ArgParser&);

  parser_type parse;
  rsl::string_view name;
  rsl::string_view description = "";
  rsl::span<Argument const> arguments;

  consteval Option(std::string_view name, std::meta::info reflection)
      : name(std::define_static_string(name)) {
    if (auto desc = annotation_of_type<annotations::Description>(reflection); desc) {
      description = desc->data;
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

    std::vector parse_args = {std::meta::reflect_constant_string(name), reflect_constant(reflection)};
    for (auto arg : args) {
      parse_args.push_back(std::meta::reflect_constant(arg));
    }

    parse = extract<parser_type>(substitute(^^Unevaluated::template do_parse, parse_args));
  }
};
}