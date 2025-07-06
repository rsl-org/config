#pragma once
#include <vector>
#include <meta>
#include <optional>

#include <rsl/string_view>
#include <rsl/span>
#include <rsl/meta_traits>
#include <rsl/constexpr_assert>

#include "argument.hpp"
#include "option.hpp"
#include "annotations.hpp"

namespace rsl {
struct CLI;
}

namespace rsl::_cli_impl {

struct Spec {
  class Parser {
    // options and commands from bases are directly folded into this spec
    std::vector<std::vector<Argument>> bases;

    // arguments are positional options required to construct the settings container
    std::vector<Argument> arguments;
    // - or -- prefixed options, can be non-static member functions
    std::vector<Option> options;
    // options that are executed before constructing an object of the wrapped type
    std::vector<Option> commands;

    // subcommands change parser context, they do not need `--` prefix
    // but cannot be used in combination with variadic or optional arguments
    std::vector<Spec> subcommands;

    consteval void parse(std::meta::info r) {
      // for (auto base : bases_of(r)) {
      //   bases.emplace_back(type_of(base));
      // }

      for (auto member : members_of(r, std::meta::access_context::current())) {
        if (!is_public(member) || !has_identifier(member)) {
          continue;
        }
        (void)(parse_argument(member) || parse_command(member) || parse_option(member));
      }
    }

    consteval void parse_base(std::meta::info r) {
      // if (meta::is_derived_from(dealias(r), ^^CLI)) {
      //   for (auto fnc_template :
      //        members_of(^^CLI) | std::views::filter(std::meta::is_function_template)) {
      //     if (!can_substitute(fnc_template, {r})) {
      //       continue;
      //     }

      //     auto fnc = substitute(fnc_template, {r});
      //     if (meta::has_annotation<annotations::Option>(fnc)) {
      //       commands.emplace_back(identifier_of(fnc_template), fnc);
      //     }
      //   }
      // }
    }

    consteval bool parse_argument(std::meta::info r) {
      if (!is_nonstatic_data_member(r)) {
        return false;
      }

      if (meta::has_annotation(r, ^^annotations::Option) ||
          meta::has_annotation(r, ^^annotations::Descend)) {
        return false;
      }

      auto base_offset = bases_of(parent_of(r), std::meta::access_context::current()).size();
      arguments.emplace_back(base_offset + arguments.size(), r);
      return true;
    }

    consteval bool parse_command(std::meta::info r) {
      if (is_function(r) && is_static_member(r)) {
        if (meta::has_annotation(r, ^^annotations::Option)) {
          commands.emplace_back(identifier_of(r), r);
          return true;
        }
      }

      return false;
    }

    consteval bool parse_option(std::meta::info r) {
      if (!meta::has_annotation(r, ^^annotations::Option)) {
        return false;
      }

      if (is_nonstatic_data_member(r)) {
        if (!has_default_member_initializer(r)) {
          rsl::compile_error(std::string("Option ") + identifier_of(r) +
                             " lacks a default member initializer.");
        }
      } else if (!is_function(r) || is_static_member(r)) {
        return false;
      }

      options.emplace_back(identifier_of(r), r);
      return true;
    }

    consteval void validate() {
      bool has_variadic = false;
      bool has_optional = false;

      for (auto&& argument : arguments) {
        if (has_variadic) {
          // we've already seen a variadic argument, no arguments can follow it
          // error
          rsl::compile_error("Non-variadic argument after a variadic argument is not allowed.");
        }

        if (argument.is_variadic) {
          has_variadic = true;
          continue;
        }

        if (argument.is_optional) {
          has_optional = true;
        } else if (has_optional) {
          // we've seen an optional argument already but this one isn't
          rsl::compile_error("Non-optional argument after an optional argument is not allowed.");
        }
      }

      if (!subcommands.empty()) {
        if (has_variadic) {
          rsl::compile_error("Variadic arguments and subcommands cannot be combined.");
        }

        if (has_optional) {
          rsl::compile_error("Optional arguments and subcommands cannot be combined.");
        }
      }
    }

    friend struct Spec;
  };

  rsl::string_view name;
  rsl::span<rsl::span<Argument const>> bases;
  rsl::span<Argument const> arguments;
  rsl::span<Option const> commands;
  rsl::span<Option const> options;

  consteval explicit Spec(std::meta::info r) : name(identifier_of(r)) {
    auto type   = is_type(r) ? r : type_of(r);
    auto parser = Parser();
    parser.parse(type);
    parser.parse_base(type);
    parser.validate();
    // bases     = define_static_array(parser.bases);
    arguments = define_static_array(parser.arguments);
    commands  = define_static_array(parser.commands);
    options   = define_static_array(parser.options);
  }

  std::vector<Argument::Unevaluated> parse_arguments(ArgParser& parser) const {
    std::vector<Argument::Unevaluated> parsed_args;
    for (auto Arg : arguments) {
      Argument::Unevaluated argument{};
      if (!(argument.*Arg.parse)(parser)) {
        // failed to parse argument - bail out
        return parsed_args;
      }
      parsed_args.push_back(argument);
    }
    return parsed_args;
  }

  bool parse_as_command(ArgParser& parser) const {
    Option::Unevaluated option;
    for (auto cmd : commands) {
      if ((option.*cmd.parse)(parser)) {
        // run commands right away
        option(nullptr);
        return true;
      }
    }
    return false;
  }

  std::optional<Option::Unevaluated> parse_as_option(ArgParser& parser) const {
    Option::Unevaluated option;
    for (auto opt : options) {
      if ((option.*opt.parse)(parser)) {
        return option;
      }
    }

    return {};
  }

  std::vector<Option::Unevaluated> parse_options(ArgParser& parser) const {
    std::vector<Option::Unevaluated> parsed_opts{};
    while (parser.valid()) {
      if (parse_as_command(parser)) {
        continue;
      }

      auto unevaluated = parse_as_option(parser);
      if (!unevaluated) {
        parser.fail("Could not find option `{}`", parser.current());
      }
      parsed_opts.push_back(*unevaluated);
    }
    return parsed_opts;
  }
};
}  // namespace rsl::_cli_impl