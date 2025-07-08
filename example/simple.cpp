#include <string>

#include <rsl/config>

struct Test : rsl::config {
  bool from_config;
  
  [[=option]]
  static void zoinks(){
    std::exit(1);
  }
};

struct Arguments : rsl::cli {
  [[=positional]] std::string text;
  Test foo;
  [[=positional]] char times = 5;

  [[=option]]
  void test(char x) {
    std::println("text: {}, times: {}, x: {}", text, times, x);
    
  }

  [[=option]]
  static void cmd(){
    std::exit(1);
  }
};

int main(int argc, char** argv) {
  static constexpr rsl::_cli_impl::Spec spec{^^Arguments};
  std::println("bases: {}, arguments: {}, commands: {}, options: {}",
    spec.bases.size(), spec.arguments.size(), spec.commands.size(), spec.options.size());

  auto args = rsl::load_config<Arguments>(argc, argv);
  std::println("text: {}, times: {}", args.text, args.times);
}