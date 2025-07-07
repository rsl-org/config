#include <string>

#include <rsl/config>

struct Test : rsl::config {
  bool from_config;
  
  [[=option]]
  static void zoinks(){}
};

struct Arguments : rsl::cli {
  [[=positional]] std::string text;
  Test foo;
  [[=positional]] int times = 5;

  [[=option]]
  void test(int x) {
    std::println("text: {}, times: {}, x: {}", text, times, x);
  }
};

int main(int argc, char** argv) {
  static constexpr rsl::_cli_impl::Spec spec{^^Arguments};
  std::println("bases: {}, arguments: {}, commands: {}, options: {}",
    spec.bases.size(), spec.arguments.size(), spec.commands.size(), spec.options.size());

  auto args = rsl::load_config<Arguments>(argc, argv);
  // std::println("{}", Arguments::config_path);
  // std::println("text: {}, times: {}", args.text, args.times);
}