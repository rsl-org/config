#include <rsl/config>
#include <string>

struct Args : rsl::cli {
  int times;
  [[= positional]] std::string text = "foo";
};

int main(int argc, char** argv) {
  auto args = Args{.times = 3};
  args.parse_args(argc, argv);
  for (int i = 0; i < args.times; ++i) {
    std::println("{}", args.text);
  }
}
