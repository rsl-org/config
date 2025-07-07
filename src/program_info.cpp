#include <filesystem>
#include <rsl/config>

namespace rsl {
std::string& ProgramInfo::get_name() {
  static std::string name;
  return name;
}

void ProgramInfo::set_name(std::string_view arg0) {
  if (arg0.empty()) {
    return;
  }
  auto path  = std::filesystem::path(arg0);
  get_name() = path.stem().string();
}

}  // namespace rsl