// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace utl {
class Logger;
}
namespace odb {
struct BumpMapData;
struct BumpMapEntry;

class BmapParser
{
 public:
  BmapParser(utl::Logger* logger);

  BumpMapData parseFile(const std::string& filename);

 private:
  void parseBumpMapContent(BumpMapData& data, const std::string& content);
  void parseBumpMapLine(BumpMapData& data,
                        const std::string& line,
                        int line_number);
  std::vector<std::string> splitLine(const std::string& line);

  utl::Logger* logger_;
  std::string current_file_path_;
};

}  // namespace odb
