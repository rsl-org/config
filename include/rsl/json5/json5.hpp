#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <charconv>

#include <rsl/constexpr_assert>
#include <rsl/_impl/default_construct.hpp>

namespace rsl::json5 {
struct Parser;
struct Value;

using Object = std::unordered_map<std::string, Value>;
using Array  = std::vector<Value>;

struct Value {
  std::string raw;

  bool operator==(Value const& other) const { return raw == other.raw; }

  template <typename T>
  T as() const {
    if constexpr (std::same_as<T, bool>) {
      return as_bool();
    } else if constexpr (std::integral<T> || std::floating_point<T>) {
      return as_number<T>();
    } else if constexpr (std::same_as<T, std::string>) {
      return as_string();
    } else if constexpr (meta::specializes<T, ^^std::vector>) {
      throw std::runtime_error("unimplemented");
    } else if constexpr (std::is_aggregate_v<T>) {
      return as_aggregate<T>();
    }
    throw std::runtime_error("unimplemented");
  }

  template <typename T>
  void update_argtuple(_impl::ArgumentTuple<T>& args) const {
    Object obj = as_object();
    constexpr_assert(obj.size() != 0);
    constexpr auto ctx     = std::meta::access_context::current();
    constexpr auto members = std::define_static_array(nonstatic_data_members_of(dealias(^^T), ctx));
    constexpr auto base_count = bases_of(dealias(^^T), ctx).size();
    template for (constexpr auto Idx : std::views::iota(0zu, members.size())) {
      constexpr auto name = std::define_static_string(identifier_of(members[Idx]));
      auto it             = obj.find(name);
      if (it != obj.end()) {
        using target_type = [:type_of(members[Idx]):];
        get<base_count + Idx>(args) = it->second.template as<target_type>();
      }
    }
  }

  template <typename T>
  T as_aggregate() const {
    auto args = _impl::ArgumentTuple<T>{};
    update_argtuple<T>(args);
    return _impl::default_construct<T>(args);
  }

  template <typename T>
  T as_number() const
    requires std::integral<T> || std::floating_point<T>
  {
    T out;
    std::from_chars(raw.data(), raw.data() + raw.size(), out);
    return out;
  }

  Object as_object() const;
  Array as_array() const;

  bool as_bool() const;
  std::string as_string() const;
};

struct Parser {
  std::string_view file;
  std::string_view::iterator cursor = std::begin(file);

  void skip_whitespace();
  bool skip_comment();
  void expect_consume(char c);
  void maybe_consume(char c);
  void parse_until(std::vector<char> delims, std::string& out);
  void parse_delimited(char lhs, std::string& out, char rhs);

  bool try_string(std::string& out);
  bool try_identifier(std::string& out);
  bool try_member_name(std::string& out);

  Value value();
  Object object();

  void expect_more() const;
};

Value load(std::string_view path);

}  // namespace rsl::json5