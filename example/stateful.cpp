#include <string>

#include <rsl/config>

struct Arguments : rsl::cli {
  unsigned times;
  std::vector<std::string> part;
  explicit Arguments(int) {}
  [[=positional]] std::string filter = "";
  [[=option]] bool durations = false;

  [[=option]]
  void c(std::string part) { }

  [[=option]]
  void reporter(std::string opt) { }

};

int main(int argc, char** argv) {
  auto args = Arguments(123);
  args.parse_args(argc, argv);

  std::println("{}", args.durations);
}