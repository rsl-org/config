#pragma once
#include <meta>
#include <rsl/meta_traits>
#include <vector>
#include <string_view>
#include <optional>

#include <rsl/string_view>
#include <rsl/string_constant>
#include <rsl/span>

#include "parser.hpp"
#include "annotations.hpp"
#include <rsl/_impl/default_construct.hpp>
#include <rsl/_impl/prettify.hpp>

namespace rsl::_cli_impl {

struct Parameter {
  struct Unevaluated {
    using handler_type = void (Unevaluated::*)(void*) const;

    handler_type handle = nullptr;
    std::string_view argument;

    void operator()(void* args) const { (this->*handle)(args); }

    template <std::size_t Idx, std::meta::info R, std::meta::info Parent>
    void do_handle(void* arguments) const {
      constexpr static auto type = remove_cvref(type_of(R));
      using parent               = [:Parent == std::meta::info{} ? type : type_of(Parent):];
      using arg_tuple            = _impl::ArgumentTuple<parent>;

      auto value                                    = parse_value<typename[:type:]>(argument);
      get<Idx>(*static_cast<arg_tuple*>(arguments)) = value;
    }
  };

  Unevaluated::handler_type _impl_handler = nullptr;

  std::size_t index;
  rsl::string_view name;
  rsl::string_view type;
  bool is_optional = false;
  bool is_variadic = false;  // currently never set

  std::optional<Unevaluated> parse(ArgParser& parser) {
    if (!parser.valid()) {
      return {};
    }

    auto current = parser.current();
    if (current[0] == '-' && (current.size() <= 1 || current[1] < '0' || current[1] > '9')) {
      // this is probably an option, bail out
      return {};
    }
    ++parser.cursor;

    return Unevaluated{.handle = _impl_handler, .argument = current};
  }

  consteval Parameter(std::size_t idx, std::meta::info reflection, std::meta::info parent)
      : index(idx)
      , name(std::define_static_string(identifier_of(reflection)))
      , type(std::define_static_string(pretty_type(reflection)))
      , is_optional(is_function_parameter(reflection)
                        ? has_default_argument(reflection)
                        : has_default_member_initializer(reflection)) {
    _impl_handler = extract<Unevaluated::handler_type>(substitute(^^Unevaluated::do_handle,
                                                                  {std::meta::reflect_constant(idx),
                                                                   reflect_constant(reflection),
                                                                   reflect_constant(parent)}));
  }
};

struct Option {
  struct Unevaluated {
    using handler_type = void (Unevaluated::*)(void*) const;
    handler_type handle;
    std::vector<Parameter::Unevaluated> parameters;
    void operator()(void* object) const { (this->*handle)(object); }

    template <std::meta::info R>
    void do_handle(void* obj) const {
      _impl::ArgumentTuple<typename[:type_of(R):]> arg_tuple;

      for (auto arg : parameters) {
        arg(&arg_tuple);
      }
      if constexpr (meta::nonstatic_member_function<R>) {
        _impl::default_invoke<R>(*static_cast<[:parent_of(R):]*>(obj), arg_tuple);
      } else if constexpr (is_function(R)) {
        _impl::default_invoke<R>(arg_tuple);
      } else if constexpr (is_object_type(type_of(R))) {
        static_cast<[:parent_of(R):]*>(
            obj) -> [:R:] = _impl::default_construct<typename[:type_of(R):]>(arg_tuple);
      } else {
        static_assert(false, "Unsupported handler type.");
      }
    }
  };

  Unevaluated::handler_type _impl_handler = nullptr;
  rsl::string_view name;
  rsl::string_view description;
  rsl::span<Parameter const> parameters;
  bool run_early = false;
  bool is_flag   = false;

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

    for (auto arg : parameters) {
      auto parameter = arg.parse(parser);
      if (parameter.has_value()) {
        option.parameters.push_back(*parameter);
      } else {
        if (is_flag && parameters.size() == 1) {
          // insert a fake "true" for flags
          option.parameters.emplace_back(arg._impl_handler, "true");
          break;
        }
        if (!arg.is_optional) {
          parser.set_error("Missing parameter {} of option {}", arg.name, name);
          break;
        }
      }
    };

    return option;
  }

  consteval Option(std::string_view name_in, std::meta::info reflection)
      : run_early(is_static_member(reflection)) {
    //? make optional? do outside of constant evaluation?
    std::string raw_name = std::string(name_in);
    std::ranges::transform(raw_name, raw_name.begin(), [](char c) { return c == '_' ? '-' : c; });
    name = std::define_static_string(raw_name);

    if (auto desc = annotation_of_type<annotations::Description>(reflection); desc) {
      description = std::define_static_string(desc->data);
    }

    if (meta::has_annotation<annotations::Flag>(reflection)) {
      is_flag = true;
    }

    std::vector<Parameter> args;
    std::size_t index = 0;
    if (is_object_type(type_of(reflection))) {
      auto parameter = Parameter(index, reflection, std::meta::info{});
      // this is an object parameter, we need a value unless it's a flag
      parameter.is_optional = false;
      args.push_back(parameter);
    } else {
      for (auto param : parameters_of(reflection)) {
        args.emplace_back(index++, param, reflection);
      }
    }

    parameters    = std::define_static_array(args);
    _impl_handler = extract<Unevaluated::handler_type>(
        substitute(^^Unevaluated::do_handle, {reflect_constant(reflection)}));
  }
};
}  // namespace rsl::_cli_impl