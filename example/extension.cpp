#include <rsl/config>
#include <print>

struct Arguments : rsl::cli {
  [[= positional]] std::string text;
  [[ = option, = flag ]] bool some_flag = false;
};

struct Foo : rsl::cli_extension<Foo> {
  [[=option]] int bar = 3;
};

int main(int argc, char** argv) {
  auto cfg = rsl::load_config<Arguments>(argc, argv);
  std::println("{}", Foo::value.bar);
}