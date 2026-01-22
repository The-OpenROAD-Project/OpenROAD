// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

#include "odb/db.h"

namespace utl {
class Logger;
}
namespace odb {

class BmapWriter
{
 public:
  BmapWriter(utl::Logger* logger);

  void writeFile(const std::string& filename, odb::dbChipRegion* region);

 private:
  void logError(const std::string& message);

  utl::Logger* logger_;
};

}  // namespace odb
