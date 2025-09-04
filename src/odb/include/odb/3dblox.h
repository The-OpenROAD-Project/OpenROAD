// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once
#include <string>

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class ChipletDef;
class ChipletRegion;
class dbChip;
class ChipletInst;
class Connection;
class DesignDef;
class ThreeDBlox
{
 public:
  ThreeDBlox(utl::Logger* logger, odb::dbDatabase* db);
  ~ThreeDBlox() = default;
  void readDbv(const std::string& dbv_file);
  void readDbx(const std::string& dbx_file);

 private:
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;

  void createChiplet(const ChipletDef& chiplet);
  void createRegion(const ChipletRegion& region, dbChip* chip);
  void createDesignTopChiplet(const DesignDef& design);
  void createChipInst(const ChipletInst& chip_inst);
  void createConnection(const Connection& connection);

  std::string resolveIncludePath(const std::string& include_path,
                                 const std::string& current_file_path);
};
}  // namespace odb