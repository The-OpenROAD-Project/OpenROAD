// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <utility>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbPowerDomain;

class _dbLevelShifter : public _dbObject
{
 public:
  _dbLevelShifter(_dbDatabase*);

  ~_dbLevelShifter();

  bool operator==(const _dbLevelShifter& rhs) const;
  bool operator!=(const _dbLevelShifter& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbLevelShifter& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* name_;
  dbId<_dbLevelShifter> next_entry_;
  dbId<_dbPowerDomain> domain_;
  dbVector<std::string> elements_;
  dbVector<std::string> exclude_elements_;
  std::string source_;
  std::string sink_;
  bool use_functional_equivalence_;
  std::string applies_to_;
  std::string applies_to_boundary_;
  std::string rule_;
  float threshold_;
  bool no_shift_;
  bool force_shift_;
  std::string location_;
  std::string input_supply_;
  std::string output_supply_;
  std::string internal_supply_;
  std::string name_prefix_;
  std::string name_suffix_;
  dbVector<std::pair<std::string, std::string>> instances_;
  std::string cell_name_;
  std::string cell_input_;
  std::string cell_output_;
};
dbIStream& operator>>(dbIStream& stream, _dbLevelShifter& obj);
dbOStream& operator<<(dbOStream& stream, const _dbLevelShifter& obj);
}  // namespace odb
   // Generator Code End Header
