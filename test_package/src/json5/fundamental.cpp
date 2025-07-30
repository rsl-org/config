#include <rsl/test>
#include <tuple>
#include <string_view>
#include <rsl/json5/json5.hpp>
#include <print>

using namespace std::string_view_literals;


namespace rsl::_test {

[[=test, =params({{"test"}, {"\"test\""}, {"'test'"}})]]
void member_name(std::string_view str) {
  auto parser = json5::Parser(str);
  std::string name;
  ASSERT(parser.try_member_name(name));
  ASSERT(name == "test");
  ASSERT(std::distance(parser.cursor, str.end()) == 0);
}

[[=test, =params({{"\"test\"", "test"}, 
                  {"'test'", "test"},
                  {"'foo bar 123'", "foo bar 123"}})]]
void string(std::string_view str, std::string_view expected) {
  auto parser = json5::Parser(str);
  std::string name;
  ASSERT(parser.try_string(name));
  ASSERT(name == expected);
  ASSERT(std::distance(parser.cursor, str.end()) == 0);
}


std::vector<std::tuple<std::string_view, json5::Object>> test_objects() {
  return {
    {"{}", {}},
    {"{'foo': 123}", {{"foo", {"123"}}}},
    {"{'foo': 123,}", {{"foo", {"123"}}}},
    {"{'foo': 123, \n}", {{"foo", {"123"}}}},
    {"{'foo': 123, 'bar': 123}", {{"foo", {"123"}}, {"bar", {"123"}}}},
    {R"(
{
    foo: 123,
    'bar': 123,
    "baz": 123
}
      )", {{"foo", {"123"}}, {"bar", {"123"}}, {"baz", {"123"}}}},
    {"{'foo': {'bar': 123}}", {{"foo", {"{'bar': 123}"}}}}
  };
}
// TODO
// [[=test, =params(test_objects)]]
// void object(std::string_view str, json5::Object expected) {
//   auto parser = json5::Parser(str);
//   auto obj = parser.object();
//   ASSERT(obj.size() == expected.size());
//   ASSERT(obj == expected);
// }

struct Test {
  bool a;
  std::string b;
  unsigned c = 123;
};

[[=test]]
void aggregate() {
  auto value = json5::Value("{a: false, b: \"foo\"}");
  auto obj = value.as<Test>();

  ASSERT(obj.a == false);
  ASSERT(obj.b == "foo");
  ASSERT(obj.c == 123);
}

}  // namespace rsl::_test