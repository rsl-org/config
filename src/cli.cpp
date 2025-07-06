#include <print>
#include <rsl/cli>

namespace rsl {
void CLI::print_help(_cli_impl::Spec const& spec,
                     std::string_view program_name,
                     std::string_view description) {
  std::string arguments{};
  for (auto argument : spec.arguments) {
    if (argument.is_optional) {
      arguments += std::format("[{}] ", argument.name);
    } else {
      arguments += std::format("{} ", argument.name);
    }
  }
  std::println("usage: {} {}", program_name, arguments);

  if (!description.empty()) {
    std::println("\n{}", description);
  }

  std::println("\nArguments:");
  for (auto argument : spec.arguments) {
    std::string constraint = argument.constraint.empty()
                                 ? ""
                                 : std::format("\n{:<8}requires: {}", "", argument.constraint);
    std::println("    {} -> {}{}", argument.name, argument.type, constraint);
  }

  std::println("\nOptions:");

  auto name_length = [](_cli_impl::Option const& opt) { return opt.name.size(); };
  auto safe_max    = [&](std::span<_cli_impl::Option const> const& opts) {
    return opts.empty() ? 0 : std::ranges::max(opts | std::views::transform(name_length));
  };
  std::size_t max_name_length = std::max(safe_max(spec.commands), safe_max(spec.options));

  auto print_option = [&](_cli_impl::Option opt) {
    std::string params;
    bool has_constraints = false;
    for (auto argument : opt.arguments) {
      if (argument.is_optional) {
        params += std::format("[{}] ", argument.name);
      } else {
        params += std::format("{} ", argument.name);
      }
      has_constraints = !argument.constraint.empty();
    }

    std::println("    --{} {}", opt.name, params);
    std::size_t offset = max_name_length + 4 + 4;
    // if (!opt.description.empty()) {
    //   std::println("{:<{}}{}\n", "", offset, opt.description);
    // }

    if (opt.arguments.size() != 0) {
      std::println("{:<{}}Arguments:", "", offset);
      for (auto argument : opt.arguments) {
        std::string constraint =
            argument.constraint.empty()
                ? ""
                : std::format("\n{:<{}}requires: {}", "", offset + 8, argument.constraint);
        char const* optional = "";
        if (argument.is_optional) {
          optional = " (optional)";
        }
        std::println("{:<{}}{} -> {}{}{}",
                     "",
                     offset + 4,
                     argument.name,
                     argument.type,
                     optional,
                     constraint);
      }
    }
  };

  for (auto command : spec.commands) {
    print_option(command);
  }

  for (auto opt : spec.options) {
    print_option(opt);
  }
}
}  // namespace rsl