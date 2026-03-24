// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "bmapParser.h"

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "objects.h"
#include "utl/Logger.h"
namespace odb {

BmapParser::BmapParser(utl::Logger* logger) : logger_(logger)
{
}

BumpMapData BmapParser::parseFile(const std::string& filename)
{
  current_file_path_ = filename;
  std::ifstream file(filename);
  if (!file.is_open()) {
    logger_->error(
        utl::ODB, 535, "Bump Map Parser Error: Cannot open file: {}", filename);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  file.close();

  BumpMapData data;
  parseBumpMapContent(data, content);

  return data;
}

void BmapParser::parseBumpMapContent(BumpMapData& data,
                                     const std::string& content)
{
  std::istringstream stream(content);
  std::string line;
  int line_number = 0;

  while (std::getline(stream, line)) {
    line_number++;

    // Skip empty lines and comments (lines starting with #)
    if (line.empty() || line[0] == '#') {
      continue;
    }

    parseBumpMapLine(data, line, line_number);
  }
}

void BmapParser::parseBumpMapLine(BumpMapData& data,
                                  const std::string& line,
                                  int line_number)
{
  std::vector<std::string> tokens = splitLine(line);

  // Check if we have exactly 6 columns
  if (tokens.size() != 6) {
    logger_->error(
        utl::ODB,
        536,
        "Bump Map Parser Error: file {} Line {} has {} columns, expected 6. "
        "Format: bumpInstName bumpCellType x y portName netName",
        current_file_path_,
        line_number,
        tokens.size());
  }

  try {
    // Parse coordinates as doubles
    const double x = std::stod(tokens[2]);
    const double y = std::stod(tokens[3]);

    // Create bump map entry
    BumpMapEntry entry(tokens[0], tokens[1], x, y, tokens[4], tokens[5]);
    data.entries.push_back(entry);

  } catch (const std::exception& e) {
    logger_->error(utl::ODB,
                   537,
                   "Bump Map Parser Error: file {} Line {} - Invalid "
                   "coordinate format: {}",
                   current_file_path_,
                   line_number,
                   std::string(e.what()));
  }
}

std::vector<std::string> BmapParser::splitLine(const std::string& line)
{
  std::vector<std::string> tokens;
  std::istringstream stream(line);
  std::string token;

  while (stream >> token) {
    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace odb
