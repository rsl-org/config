#include <string>

#include <rsl/config>

struct Arguments : rsl::config {
  // static inline std::string config_path = "foo";
  [[=positional]] std::string text;
  bool from_config;
  [[=positional]] int times = 5;

  [[=option]]
  void test(int x) {
    std::println("text: {}, times: {}, x: {}", text, times, x);
  }

  // [[=option]]
  // static void config(std::string cfg_path) {
  //   std::println("config() invoked");
  //   Arguments::config_path = cfg_path;
  // }
};

int main(int argc, char** argv) {
  static constexpr rsl::_cli_impl::Spec spec{^^Arguments};
  std::println("bases: {}, arguments: {}, commands: {}, options: {}",
    spec.bases.size(), spec.arguments.size(), spec.commands.size(), spec.options.size());

  auto args = rsl::load_config<Arguments>(argc, argv);
  // std::println("{}", Arguments::config_path);
  // std::println("text: {}, times: {}", args.text, args.times);
}