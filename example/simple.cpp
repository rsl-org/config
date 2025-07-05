#include <string>

#include <rsl/cli>

struct Arguments : rsl::CLI {
  std::string text;
  int times = 5;

  [[=option]]
  void test(int x) {
    std::println("text: {}, times: {}, x: {}", text, times, x);
  }

  [[=option]]
  static void version() {
    std::println("version() invoked");
    std::exit(0);
  }
};

int main(int argc, char** argv) {
  auto args = rsl::parse_args<Arguments>({argv, argv+argc});
  std::println("text: {}, times: {}", args.text, args.times);
}