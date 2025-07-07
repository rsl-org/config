#include "json5.hpp"

namespace rsl::json5 {
template <typename T>
struct Serializer {
  static T deserialize(Value const& json) 
  requires requires { {from_json(Tag<T>{}, json)} -> std::convertible_to<T>;}
  {
    return from_json(Tag<T>{}, json);
  }

  static Value serialize(T const& obj) 
  requires requires { {to_json(Tag<T>{}, obj)} -> std::convertible_to<Value>;}
  {
    return to_json(Tag<T>{}, obj);
  }
};

template <>
struct Serializer<bool> {
  static bool deserialize(Value const& json);
  static Value serialize(bool obj);
};

template <>
struct Serializer<std::string> {
  static std::string deserialize(Value const& json);
  static Value serialize(std::string obj);
};

template <typename T>
  requires std::integral<T> || std::floating_point<T>
struct Serializer<T> {
  static T deserialize(Value const& json) {
    return json.template as_number<T>();
  }

  static Value serialize(T obj) {
    throw "unimplemented";
  }
};
}
