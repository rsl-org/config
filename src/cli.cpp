#include <print>
#include <filesystem>
#include <rsl/config>

namespace rsl {
std::string& cli::get_config_path() {
  static std::string config_path{ std::filesystem::current_path() / "settings.json5" };
  return config_path;
}

void config::print_help(_cli_impl::Spec const& spec,
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
    std::println("    {} -> {}", argument.name, argument.type);
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
    std::size_t offset = 8;
    // if (!opt.description.empty()) {
    //   std::println("{:<{}}{}\n", "", offset, opt.description);
    // }

    if (opt.arguments.size() != 0) {
      for (auto argument : opt.arguments) {
        char const* optional = "";
        if (argument.is_optional) {
          optional = " (optional)";
        }
        std::println("{:<{}}{} -> {}{}",
                     "",
                     offset,
                     argument.name,
                     argument.type,
                     optional);
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