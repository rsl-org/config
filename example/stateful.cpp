#include <string>

#include <rsl/config>

class Arguments : public rsl::cli {
  std::vector<std::string> part;
public:
  unsigned times;
  explicit Arguments(int) {}

  [[=positional]] std::string filter = "";
  [[=option]] std::string test = "false";

  [[=option]]
  void c(std::string part) { }

  [[=option]]
  void reporter(std::string opt) { }

};

int main(int argc, char** argv) {
    static constexpr rsl::_cli_impl::Spec spec{^^Arguments};
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


  auto args = Arguments(123);
  args.parse_args(argc, argv);

  std::println("{}", args.test);
}