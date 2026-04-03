#include <string>
#include <rsl/config>

struct Arguments : rsl::cli {
  [[= positional]] std::string text;
  [[= positional]] int times = 5;
  // [[=option, =flag]] bool baz = false;
  // [[= option]] void test(char x) { std::println("text: {}, times: {}, x: {}", text, times, x); }
  // [[= option]] static void cmd() { std::exit(1); }
};

int main(int argc, char** argv) {
  // auto args = rsl::load_config<Arguments>(argc, argv);
  auto args = Arguments{.text="foo"};
  args.parse_args(argc, argv);
  std::println("text: {}, times: {}", args.text, args.times);
}