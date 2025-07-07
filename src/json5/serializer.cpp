#include <rsl/json5/serializer.hpp>

namespace rsl::json5 {
bool Serializer<bool>::deserialize(Value const& json) {
  return json.as_bool();
}

Value Serializer<bool>::serialize(bool obj) {
  throw "unimplemented";
}

std::string Serializer<std::string>::deserialize(Value const& json) {
  std::string content;
  if (!Parser(json.raw).try_string(content)) {
    throw std::runtime_error("Could not extract as string.");
  }
  return content;
}
Value Serializer<std::string>::serialize(std::string obj) {
  throw "unimplemented";
}
}