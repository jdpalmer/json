#include <string>
#include <vector>
#include <stdio.h>
#include <fstream>
#include "../json.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: jsontest <JSONFILE>" << std::endl;
    return 1;
  }
  auto input = std::ifstream(argv[1]);
  try {
    auto value = json::parse(input);
    std::cout << value << std::endl;
    return 0;
  }
  catch (json::ParseError e) {
    std::cout << "[" << "\"" << e.what() << "\", " << e.line << "]" << std::endl;
    return 1;
  }
}
