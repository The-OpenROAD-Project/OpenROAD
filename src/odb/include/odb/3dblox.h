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
class ThreeDBlox
{
 public:
  ThreeDBlox(utl::Logger* logger, odb::dbDatabase* db);
  ~ThreeDBlox() = default;
  void readDbv(const std::string& dbv_file);

 private:
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;

  void createChiplet(const ChipletDef& chiplet);
  void createRegion(const ChipletRegion& region, dbChip* chip);
};
}  // namespace odb