#pragma once
#include <vector>
#include <string_view>
#include <print>

#include "../_impl/default_construct.hpp"

#include "spec.hpp"
#include "argument.hpp"
#include "option.hpp"

namespace rsl {
class config;
}

namespace rsl::_cli_impl {
struct ParsedArgs {
  Spec spec;
  std::string_view program_name;
  std::vector<Argument::Unevaluated> arguments;
  std::vector<Option::Unevaluated> options;

  constexpr ParsedArgs(Spec const& spec, std::span<std::string_view> args_in) : spec(spec) {
    auto parser                 = ArgParser{args_in};
    auto& [args, cursor, error] = parser;

    auto program_name = args_in[0];
    cursor            = 1;

    std::string argument_error;
    if (!parse_arguments(parser) && parser.has_error()) {
      // reset parser error state, get its error
      std::swap(argument_error, error);
    }

    // even if parsing arguments fail, try options - some might be commands that we can execute
    // anyway
    if (!parse_options(parser) && parser.has_error()) {
      std::println("{}", error);
      std::exit(1);
    }

    // finally, report argument parsing errors
    if (!argument_error.empty()) {
      std::println("{}", argument_error);
      std::exit(1);
    }
  }

  bool parse_arguments(ArgParser& parser) {
    for (auto arg : spec.arguments) {
      auto argument = arg.parse(parser);
      if (!argument) {
        if (!arg.is_optional) {
          parser.set_error("Missing required argument {}", arg.name);
          return false;
        }
        // failed to parse argument - bail out
        return true;
      }
      arguments.push_back(*argument);
    }
    return true;
  }

  bool parse_options(ArgParser& parser) {
    while (parser.valid()) {
      auto current = parser.current();

      if (!current.starts_with('-')) {
        parser.set_error("Expected an option.");
        return false;
      } else {
        current.remove_prefix(1);
      }

      if (current.starts_with('-')) {
        // long option
        current.remove_prefix(1);

      } else {
        // short option
      }

      auto try_options = [&](auto spec_opts) {
        for (auto opt : spec_opts) {
          if (auto unevaluated = opt.parse(parser); unevaluated) {
            if (opt.run_early) {
              (*unevaluated)(nullptr);
            } else {
              options.push_back(*unevaluated);
            }
            return true;
          }
        }
        return false;
      };

      bool found = try_options(spec.options);
      for (auto extra : spec.extensions()) {
        if (found) {
          break;
        }
        found |= try_options(extra.options);
      }

      if (parser.has_error()) {
        return false;
      }

      if (!found) {
        parser.set_error("Could not find option `{}`", parser.current());
        return false;
      }
    }
    return true;
  }
};

template <typename T>
ParsedArgs parse(std::span<std::string_view> args_in) {
  static constexpr _cli_impl::Spec spec{^^T, {^^T}};
  return {spec, args_in};
}

template <typename T>
_impl::ArgumentTuple<T> tuple_from_args(std::span<Argument::Unevaluated> args) {
  // TODO only get positional
  _impl::ArgumentTuple<T> args_tuple;
  if constexpr (std::derived_from<T, config>) {
    // only load config if T is configurable
    auto config = json5::load(T::get_config_path());
    config.template update_argtuple<T>(args_tuple);
  }

  for (auto argument : args) {
    argument(&args_tuple);
  }
  return args_tuple;
}

template <typename T>
T construct_from_args(std::span<Argument::Unevaluated> args) {
  auto args_tuple = tuple_from_args<T>(args);
  return _impl::default_construct<T>(args_tuple);
}

template <typename T>
void apply_args(T&& object, std::span<Argument::Unevaluated> args) {
  using raw_t     = std::remove_cvref_t<T>;
  auto args_tuple = tuple_from_args<raw_t>(args);

  constexpr auto ctx = std::meta::access_context::current();
  constexpr auto members =
      std::define_static_array(nonstatic_data_members_of(dealias(^^raw_t), ctx));
  constexpr auto base_count = bases_of(dealias(^^raw_t), ctx).size();

  template for (constexpr auto Idx : std::views::iota(0ZU, members.size())) {
    if (auto value = get<base_count + Idx>(args_tuple); value) {
      object.[:members[Idx]:] = *value;
    }
  }
}

template <typename T>
void apply_options(T&& object, std::span<Option::Unevaluated> options) {
  for (auto&& option : options) {
    option(&object);
  }
}
}  // namespace rsl::_cli_impl