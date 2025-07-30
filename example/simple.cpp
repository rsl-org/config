#include <string>

#include <rsl/config>

struct Test : rsl::config {
  bool from_config;

  [[= rsl::cli::option]] static void zoinks() { std::exit(1); }
};

struct Arguments
    : rsl::cli
    // , rsl::config
     {
  [[= positional]] std::string text;
  // Test foo;
  [[= positional]] char times = 5;
  [[=option, =flag]] bool baz = false;

  [[= option]] void test(char x) { std::println("text: {}, times: {}, x: {}", text, times, x); }

  [[= option]] static void cmd() {
    // std::exit(1);
  }
};

int main(int argc, char** argv) {
  static constexpr rsl::_cli_impl::Spec spec{^^Arguments, {^^Arguments}};
  std::println("bases: {}, arguments: {}, options: {}",
               spec.bases.size(),
               spec.arguments.size(),
               spec.options.size());
  for (auto const& arg : spec.arguments) {
    std::println("arg: {}", arg.name);
  }

  for (auto const& opt : spec.options) {
    std::println("opt: {}", opt.name);
  }

  auto args = rsl::load_config<Arguments>(argc, argv);
  std::println("text: {}, times: {}", args.text, args.times);
}