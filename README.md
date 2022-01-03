# json.hpp

## Motivation

json.hpp is a lightweight (<500 LOC) C++17 single file header library
for encoding and decoding JSON. JSON entities are deserialized into
`std::vector`, `std::map`, `std::string`, `double`, and `bool`.

While there are many JSON parsing libraries for C++ they often have
significant dependencies or unnecessary complexity.  json.hpp intends
to be extremely small, easy to integrate into existing C++ code, easy
to understand, and robust.

This library is liberally licensed under the [Zero Clause
BSD](https://en.wikipedia.org/wiki/Public-domain-equivalent_license),
a public domain equivalent license.

## Installation

Simply include `json.hpp` directly in your project! When compiling
with clang or gcc you may need to add the "-std=c++17" compiler flag.

## Decoding JSON

json.hpp uses `std::istream` and `std::ostream` for accessing files
(or other stream subclasses). If an error occurs in parsing,
a `ParseError` exception is thrown:

    #include "json.hpp"
    ..
    auto input = std::ifstream("myfile.json");
    try {
      auto value = json::parse(input);
    }
    catch (json::ParseError e) {
      // Handle the exception.
    }

Note that `value` in the above example is returned as a
`std::unique_ptr` and will be cleaned up automatically when `value`
falls out of scope.

## Accessing JSON Values

The `json::Value` type is used to represent all JSON values as
standard C++ types used internally. The `parse()` function returns a
`Value` object which can be used as described below. JSON Arrays and
JSON Objects are also collections of `Value` objects as well.

JSON numbers are represented with `double`. The `is_number()` method
is used to detect if a `Value` is a number and the `as_number()`
returns a `double`.

    if (value.is_number()) {
      double x = value.as_number();
    }

JSON strings are represented with `std::string`. The `is_string()`
method is used to detect if a `Value` is a string and the
`as_string()` returns a `std::string`.

    if (value.is_string()) {
      std::string s = values.as_string();
    }

JSON boolean values are represented with `bool`. The `is_bool()`
method is used to detect if a `Value` is a bool and the `as_bool()`
returns a C++ `bool`.

    if (value.is_bool()) {
      bool b = values.as_bool();
    }

JSON arrays are represented with `std::vector`. The `is_array()`
method is used to detect if a `Value` is an array and the `as_array()`
returns a `std::vector` which you can access in the usual way.

    if (value.is_array()) {
      for (auto &z : value.as_array()) {
        // do something with z (Value type)
      }
    }

JSON objects are represented with `std::map`. The `is_object()` method
is used to detect if a `Value` is an object and the `as_object()`
returns a `std::map` which you can access in the usual way.

    if (value.is_object()) {
      for (auto &z : value.as_object()) {
        // do something with the key, z.first (std::string)
        // or the value, z.second (Value type)
      }
    }

The JSON null literal has no mapping to a C++ type; the `is_null()`
method determines if the `Value` is null but no `as_null()` method is
provided.

    if (value.is_null()) {
      // do something
    }

## Encoding JSON

While json.hpp can encode json from `Value` objects, this is not
intended to be a significant use case. It will be typically be more
code efficient to provide your own object serialization.  That being
said, `Value` objects are recursively serialized using the standard
stream output operator:

    out << value;

There are no options for pretty printing or customizing the output.

If you do implement your own object serialization, you may find the
`json::print()` function useful, which properly escapes a JSON string
from a `std::string`:

    json::print(out, "Hello World");

And that is pretty much the entire API!

## Tests

The included test suite is written using
[pytest](https://docs.pytest.org) and compares the serialization and
deserialization of values using
[jsondiff](https://github.com/xlwings/jsondiff) on both well-formed
and malformed samples. On MacOS the test suite also includes memory
leak testing using Apple’s “leaks” tool.

Assuming you have Python 3 installed, you can install the other
dependencies with:

    pip3 install pytest
    pip3 install jsondiff

The Makefile assumes you are using GNU Make and either gcc or
clang. Simply type `make` in the tests folder to build and execute the
test suite.
