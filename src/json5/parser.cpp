#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <format>

#include <rsl/json5/json5.hpp>

namespace rsl::json5 {

namespace {
bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t';
};

int isalpha(int c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}
int isdigit(int c) {
  return c >= '0' && c <= '9';
}
}  // namespace

void Parser::expect_more() const {
  if (cursor == file.end()) {
    throw std::runtime_error("Expected more data, reached EOF");
  }
}

bool Parser::skip_comment() {
  if (cursor == file.end()){
    return false;
  }

  if (*cursor == '/') {
    ++cursor;
    expect_more();
    std::string content;
    if (*cursor == '/') {
      ++cursor;
      parse_until({'\n'}, content);
    } else if (*cursor == '*') {
      ++cursor;
      parse_until({'*', '/'}, content);
    } else {
      // roll back - this parse was invalid
      --cursor;
      return false;
    }
    // TODO we probably want to keep comments to round-trip properly
    // std::println("Comment: {}", content);
    return true;
  }
  return false;
}

void Parser::skip_whitespace() {
  while (is_whitespace(*cursor))
    cursor++;
  
  if (skip_comment()) {
    // comment found - recurse
    skip_whitespace();
  }
  
};

void Parser::expect_consume(char c) {
  skip_whitespace();
  expect_more();
  if (*(cursor++) != c)
    throw std::runtime_error(std::format("unexpected character: got {}, expected {}", *cursor, c));
};

void Parser::maybe_consume(char c) {
  skip_whitespace();
  if (cursor != file.end() && *cursor == ',')
    ++cursor;
}

void Parser::parse_until(std::vector<char> delims, std::string& out) {
  skip_whitespace();
  while (cursor != file.end() && !std::ranges::any_of(delims, [&](char c) { return c == *cursor; }))
    out += *(cursor++);
};

void Parser::parse_delimited(char lhs, std::string& out, char rhs) {
  expect_consume(lhs);
  parse_until({rhs}, out);
  expect_consume(rhs);
};

bool Parser::try_string(std::string& out) {
  if (*cursor != '"' && *cursor != '\'') {
    return false;
  }
  auto delimiter = *cursor;
  parse_delimited(delimiter, out, delimiter);
  return true;
}

bool Parser::try_identifier(std::string& out) {
  char current = *cursor;
  if (isalpha(current) || current == '$' || current == '_') {
    out += current;
  } else {
    return false;
  }

  while (++cursor != file.end()) {
    if (current = *cursor; isalnum(current) || current == '$' || current == '_') {
      out += current;
    } else {
      break;
    }
  }
  return true;
}

bool Parser::try_member_name(std::string& out) {
  skip_whitespace();
  return try_string(out) || try_identifier(out);
}

Value Parser::value() {
  skip_whitespace();
  std::string out;
  bool quoted    = false;
  unsigned depth = 0;
  while (true) {
    expect_more();

    if (is_whitespace(*cursor) && !quoted && depth == 0)
      break;

    if (depth == 0 && (*cursor == ',' || *cursor == '}'))
      break;
    out += *(cursor++);

    if (out.back() == '{')
      ++depth;
    else if (out.back() == '}')
      --depth;
    else if (out.back() == '"') {
      if (quoted && depth == 0)
        break;
      quoted = true;
    }
  }
  return {out};
}

Object Parser::object() {
  std::unordered_map<std::string, Value> result;
  
  expect_consume('{');
  skip_whitespace();
  
  while (cursor != file.end() && *cursor != '}') {
    std::string field_name;
    if (!try_member_name(field_name))
      break;

    expect_consume(':');
    
    auto val = value();
    if (val.raw.empty()){
      throw std::runtime_error("expected value");
    }

    result.insert({field_name, val});
    maybe_consume(',');
  }

  expect_consume('}');

  return result;
}

Object Value::as_object() const {
  return Parser(raw).object();
}

Array Value::as_array() const {
  throw std::runtime_error("unimplemented");
}

bool Value::as_bool() const {
  if (raw == "true" || raw == "1")
    return true;
  if (raw == "false" || raw == "0")
    return false;
  throw std::runtime_error("Could not extract as bool");
}

std::string Value::as_string() const {
  std::string content;
  if (!Parser(raw).try_string(content)) {
    throw std::runtime_error("Could not extract as string.");
  }
  return content;
}

Value load(std::string_view path) {
  auto config_path = std::filesystem::path(path);
  if (!exists(config_path)) {
    throw std::runtime_error(std::format("File {} does not exist.", path));
  }
  auto in = std::ifstream(config_path);
  std::stringstream buffer;
  buffer << in.rdbuf();

  return Value(buffer.str());
}

}  // namespace rsl::json5