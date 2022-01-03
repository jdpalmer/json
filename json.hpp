// Copyright (C) 2021 by James Palmer
//
// Zero Clause BSD License
//
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
// OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Website: https://github.com/jdpalmer/json

#pragma once
#ifndef JSON_HPP
#define JSON_HPP

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <cmath>

namespace json {

struct ParseError : public std::runtime_error {
  const std::size_t line;

  ParseError(const std::string &what, const std::size_t line = 0) throw () :
    std::runtime_error(what), line(line) {}
};

static void print(std::ostream &out, const std::string &value) {
  out << '\"';
  for (const auto &c : value) {
    switch (c) {
    case '\b': out << "\\b"; continue;
    case '\f': out << "\\f"; continue;
    case '\n': out << "\\n"; continue;
    case '\t': out << "\\t"; continue;
    case '\r': out << "\\r"; continue;
    case '\"': out << "\\\""; continue;
    }
    out << c;
  }
  out << '\"';
}

struct Reader {
  std::istream &input;
  int line = 0;
  int capture_offset = -1;
  char buffer[64];

  Reader(std::istream &input) : input(input) {}

  int get() {
    int c = input.get();
    if (c == EOF) {
      throw ParseError("Unexpected end of file", line);
    }
    if (capture_offset >= 0) {
      if (capture_offset >= 64) {
        throw ParseError("Overlong value", line);
      }
      buffer[capture_offset] = c;
      capture_offset += 1;
    }
    return c;
  }

  void unget() {
    input.unget();
    if (capture_offset >= 0) {
      capture_offset -= 1;
    }
  }

  void begin_capture() {
    capture_offset = 0;
  }

  void end_capture() {
    buffer[capture_offset] = 0;
    capture_offset = -1;
  }

  void skip_ws() {
    int c = input.get();
    while (true) {
      switch (c) {
      case '\n': line += 1;
      case ' ':
      case '\r':
      case '\t': break;
      case EOF:  return;
      default:
        input.unget();
        return;
      };
      c = input.get();
    }
  }

  void consume(const std::string str, const std::string err) {
    for (const auto &c : str) {
      if (c != input.get()) {
        throw ParseError(err, line);
      }
    }
  }

  const std::string utf16_to_utf8(const uint16_t high, const uint16_t low) const {
    uint32_t codepoint;
    if ((high <= 0xD7FF) || (high >= 0xE000)) {
      if (low) {
        throw ParseError("Invalid UTF16 surrogate pair", line);
      }
      codepoint = high;
    } else {
      if (high > 0xDBFF) {
        throw ParseError("Invalid UTF16 high surrogate", line);
      }
      if (low < 0xDC00 || low > 0xDFFF) {
        throw ParseError("Invalid UTF16 low surrogate", line);
      }
      codepoint = ((high - 0xD800) << 10) + (low - 0xDC00) + 0x010000;
    }
    if (codepoint <= 0x00007F) {
      return std::string({(char)codepoint});
    } else if (codepoint <= 0x0007FF) {
      return std::string({
          (char)(0xC0 | ((codepoint & (0x1F << 6)) >> 6)),
          (char)(0x80 | ((codepoint & (0x3F))))});
    } else if (codepoint <= 0x00FFFF) {
      return std::string({
          (char)(0xE0 | ((codepoint & (0x0F << 12)) >> 12)),
          (char)(0x80 | ((codepoint & (0x3F << 6)) >> 6)),
          (char)(0x80 | ((codepoint & (0x3F))))});
    } else if (codepoint <= 0x1FFFFF) {
      return std::string({
          (char)(0xF0 | ((codepoint & (0x07 << 18)) >> 18)),
          (char)(0x80 | ((codepoint & (0x3F << 12)) >> 12)),
          (char)(0x80 | ((codepoint & (0x3F << 6)) >> 6)),
          (char)(0x80 | ((codepoint & (0x3F))))});
    }
    throw ParseError("Invalid unicode codepoint", line);
  }

  char16_t consume_surrogate() {
    begin_capture();
    for (int repeat = 0; repeat < 4; repeat += 1) {
      if (!isxdigit(get())) {
        throw ParseError("Invalid hex escape code", line);
      }
    }
    end_capture();
    return strtol(buffer, 0, 16);
  }
};

struct Value {
  virtual ~Value() {}
  virtual void print(std::ostream &out) const {}
  bool is_array() const;
  bool is_bool() const;
  bool is_null() const;
  bool is_number() const;
  bool is_object() const;
  bool is_string() const;
  const std::vector<std::unique_ptr<Value>> &as_array() const;
  bool as_bool() const;
  double as_number() const;
  const std::map<std::string, std::unique_ptr<Value>> &as_object() const;
  const std::string &as_string() const;
};

struct Array : Value {
  std::vector<std::unique_ptr<Value>> value;

  void print(std::ostream &out) const {
    out << '[';
    for (const auto &element : value) {
      if (*std::begin(value) != element) {
        out << ", ";
      }
      element->print(out);
    }
    out << ']';
  }
};

struct Bool : Value {
  bool value;

  Bool(const bool value) : value(value) {}

  void print(std::ostream &out) const {
    if (value) {
      out << "true";
    } else {
      out << "false";
    }
  }
};

struct Null : Value {
  void print(std::ostream &out) const {
    out << "null";
  }
};

struct Number : Value {
  double value;

  Number(const double value) : value(value) {}

  void print(std::ostream &out) const {
    if (value == INFINITY) {
      out << "1.0e5000";
    } else if (value == -INFINITY) {
      out << "-1.0e5000";
    } else if (std::isnan(value)) {
      out << "null";
    } else {
      out << value;
    }
  }
};

struct Object : Value {
  std::map<std::string, std::unique_ptr<Value>> value;

  void print(std::ostream &out) const {
    out << '{';
    for (const auto &element : value) {
      if (*std::begin(value) != element) {
        out << ", ";
      }
      json::print(out, element.first);
      out << ": ";
      element.second->print(out);
    }
    out << '}';
  }
};

struct String : Value {
  std::string value;

  String(const std::string value) : value(value) {}

  void print(std::ostream &out) const {
    json::print(out, value);
  }
};

inline bool Value::is_array() const {
  auto json_array = dynamic_cast<const Array *>(this);
  return json_array != nullptr;
}

inline const std::vector<std::unique_ptr<Value>> &Value::as_array() const {
  auto json_array = dynamic_cast<const Array *>(this);
  return json_array->value;
}

inline bool Value::is_bool() const {
  auto json_bool = dynamic_cast<const Bool *>(this);
  return json_bool != nullptr;
}

inline bool Value::as_bool() const {
  auto json_bool = dynamic_cast<const Bool *>(this);
  return json_bool->value;
}

inline bool Value::is_null() const {
  auto json_null = dynamic_cast<const Null *>(this);
  return json_null != nullptr;
}

inline bool Value::is_number() const {
  auto json_number = dynamic_cast<const Number *>(this);
  return json_number != nullptr;
}

inline double Value::as_number() const {
  auto json_number = dynamic_cast<const Number *>(this);
  return json_number->value;
}

inline bool Value::is_object() const {
  auto json_object = dynamic_cast<const Object *>(this);
  return json_object != nullptr;
}

inline const std::map<std::string,std::unique_ptr<Value>> &Value::as_object() const {
  auto json_object = dynamic_cast<const Object *>(this);
  return json_object->value;
}

inline bool Value::is_string() const {
  auto json_string = dynamic_cast<const String *>(this);
  return json_string != nullptr;
}

inline const std::string &Value::as_string() const {
  auto json_string = dynamic_cast<const String *>(this);
  return json_string->value;
}

std::ostream &operator<<(std::ostream &out, const Value &value) {
  value.print(out);
  return out;
}

std::ostream &operator<<(std::ostream &out, const Value *value) {
  value->print(out);
  return out;
}

static std::unique_ptr<Value> parse(Reader &reader) {
  reader.skip_ws();
  reader.begin_capture();
  int c = reader.get();
  if (!(isdigit(c) || c == '-')) {
    reader.end_capture();
  }
  switch (c) {
  case '{': {
    auto result = std::make_unique<Object>();
    reader.skip_ws();
    c = reader.get();
    if (c != '}') {
      reader.unget();
      while (true) {
        const auto left = parse(reader);
        if (!left->is_string()) {
          throw ParseError("Expected string key", reader.line);
        }
        reader.skip_ws();
        c = reader.get();
        if (c != ':') {
          throw ParseError("Expected ':' after key", reader.line);
        }
        result->value.insert(std::pair<std::string, std::unique_ptr<Value>>
                             (left->as_string(), parse(reader)));
        reader.skip_ws();
        c = reader.get();
        if (c == '}') break;
        if (c == ',') continue;
        throw ParseError("Expected ',' or '}' after value", reader.line);
      }
    }
    return result;
  }
  case '[': {
    auto result = std::make_unique<Array>();
    reader.skip_ws();
    c = reader.get();
    if (c != ']') {
      reader.unget();
      while (true) {
        result->value.push_back(parse(reader));
        reader.skip_ws();
        c = reader.get();
        if (c == ']') break;
        if (c == ',') continue;
        throw ParseError("Expected ',' or ']' after value", reader.line);
      }
    }
    return result;
  }
  case '"': {
    std::string value;
    c = reader.get();
    while (true) {
      if (c == '"') break;
      if (c == '\n') {
        throw ParseError("Missing string termination before EOL", reader.line);
      }
      if (c == '\\') {
        c = reader.get();
        switch (c) {
        case '"':
        case '/':
        case '\\': break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'u': {
          char16_t high = reader.consume_surrogate();
          char16_t low = 0;
          if ((high >= 0xD800) && (high <= 0xDBFF)) {
            reader.consume("\\u", "Expected unicode surrogate pair");
            low = reader.consume_surrogate();
          }
          value.append(reader.utf16_to_utf8(high, low));
          c = reader.get();
          continue;
        }
        default:
          throw ParseError("Invalid escape code", reader.line);
        }
      }
      value.push_back(c);
      c = reader.get();
    }
    return std::make_unique<String>(value);
  }
  case '-':
    c = reader.get();
  case '0'...'9': {
    if (c == '0') {
      c = reader.get();
    } else {
      if (!isdigit(c)) {
        throw ParseError("Invalid number format", reader.line);
      }
      while (true) {
        c = reader.get();
        if (!isdigit(c)) break;
      }
    }
    if (c == '.') {
      while (true) {
        c = reader.get();
        if (!isdigit(c)) break;
      }
    }
    if ((c == 'e') || (c == 'E')) {
      c = reader.get();
      if ((c == '-') || (c == '+')) {
        c = reader.get();
      }
      if (!isdigit(c)) {
        throw ParseError("Invalid number format", reader.line);
      }
      while (true) {
        c = reader.get();
        if (!isdigit(c)) break;
      }
    }
    reader.unget();
    reader.end_capture();
    return std::make_unique<Number>(atof(reader.buffer));
  }
  case 't': {
    reader.consume("rue", "Invalid literal");
    return std::make_unique<Bool>(true);
  }
  case 'f': {
    reader.consume("alse", "Invalid literal");
    return std::make_unique<Bool>(false);
  }
  case 'n': {
    reader.consume("ull", "Invalid literal");
    return std::make_unique<Null>();
  }
  };
  throw ParseError("Invalid structure or literal", reader.line);
}

static std::unique_ptr<Value> parse(std::istream &in) {
  auto reader = Reader(in);
  auto value = parse(reader);
  reader.skip_ws();
  if (in.eof()) {
    return value;
  }
  throw ParseError("Invalid structure or literal", reader.line);
}

}
#endif
