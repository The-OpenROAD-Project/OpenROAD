// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

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
class dbChipRegionInst;
class dbChipInst;

class ThreeDBlox
{
 public:
  ThreeDBlox(utl::Logger* logger, odb::dbDatabase* db);
  ~ThreeDBlox() = default;
  void readDbv(const std::string& dbv_file);
  void readDbx(const std::string& dbx_file);

 private:
  void createChiplet(const ChipletDef& chiplet);
  void createRegion(const ChipletRegion& region, dbChip* chip);
  dbChip* createDesignTopChiplet(const DesignDef& design);
  void createChipInst(const ChipletInst& chip_inst);
  void createConnection(const Connection& connection);
  dbChipRegionInst* resolvePath(const std::string& path,
                                std::vector<dbChipInst*>& path_insts);
  void readHeaderIncludes(const std::vector<std::string>& includes);
  void calculateSize(dbChip* chip);

  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
};
}  // namespace odb
