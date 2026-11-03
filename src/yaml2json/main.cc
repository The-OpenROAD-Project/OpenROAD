// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// yaml2json: convert one YAML document to JSON.
//
//   yaml2json [FILE]
//
// Reads FILE, or standard input when FILE is omitted or "-", and writes JSON
// to standard output. Lets a script read YAML through its language's JSON
// library, with no YAML library installed on the host.

#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "yaml2json.h"

int main(int argc, char* argv[])
{
  if (argc > 2) {
    std::cerr << "usage: yaml2json [FILE]\n";
    return 2;
  }
  const std::string path = argc == 2 ? argv[1] : "-";
  std::stringstream text;
  if (path == "-") {
    text << std::cin.rdbuf();
  } else {
    std::ifstream file(path);
    if (!file) {
      std::cerr << "yaml2json: cannot open " << path << "\n";
      return 1;
    }
    text << file.rdbuf();
  }
  try {
    std::cout << yaml2json::yamlToJson(text.str());
  } catch (const std::exception& e) {
    std::cerr << "yaml2json: " << path << ": " << e.what() << "\n";
    return 1;
  }
  std::cout.flush();
  return std::cout ? 0 : 1;
}
