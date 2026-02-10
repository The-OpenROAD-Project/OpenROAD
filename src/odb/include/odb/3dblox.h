// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace utl {
class Logger;
}
namespace sta {
class Sta;
}
namespace odb {
class dbDatabase;
class dbChip;
class dbChipRegionInst;
class dbChipInst;
class dbChipRegion;
class dbBlock;
class dbBTerm;
class dbInst;
class dbTech;
class dbLib;

struct ChipletDef;
struct ChipletRegion;
struct ChipletInst;
struct Connection;
struct DesignDef;
struct BumpMapEntry;

class ThreeDBlox
{
 public:
  ThreeDBlox(utl::Logger* logger, odb::dbDatabase* db, sta::Sta* sta = nullptr);
  ~ThreeDBlox() = default;
  void readDbv(const std::string& dbv_file);
  void readDbx(const std::string& dbx_file);
  void readBMap(const std::string& bmap_file);
  void check();
  void writeDbv(const std::string& dbv_file, odb::dbChip* chip);
  void writeDbx(const std::string& dbx_file, odb::dbChip* chip);
  void writeBMap(const std::string& bmap_file, odb::dbChipRegion* region);

 private:
  void createChiplet(const ChipletDef& chiplet);
  void createRegion(const ChipletRegion& region, dbChip* chip);
  dbChip* createDesignTopChiplet(const DesignDef& design);
  void createChipInst(const ChipletInst& chip_inst);
  void createConnection(const Connection& connection);
  void createBump(const BumpMapEntry& entry, dbChipRegion* chip_region);
  std::pair<dbInst*, dbBTerm*> createBump(const BumpMapEntry& entry,
                                          dbBlock* block);
  dbChipRegionInst* resolvePath(const std::string& path,
                                std::vector<dbChipInst*>& path_insts);
  void readHeaderIncludes(const std::vector<std::string>& includes);
  void calculateSize(dbChip* chip);

  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  sta::Sta* sta_ = nullptr;
  std::unordered_set<odb::dbTech*> written_techs_;
  std::unordered_set<odb::dbLib*> written_libs_;
  std::unordered_set<std::string> read_files_;
};
}  // namespace odb
