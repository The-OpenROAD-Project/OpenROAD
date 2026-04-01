// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

namespace utl {
class Logger;
}
namespace odb {
class dbChip;

class VerilogWriter
{
 public:
  VerilogWriter(utl::Logger* logger);
  void writeChiplet(const std::string& filename, odb::dbChip* chip);

 private:
  utl::Logger* logger_ = nullptr;
};

}  // namespace odb
