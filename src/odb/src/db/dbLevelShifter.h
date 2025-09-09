// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"

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

  char* _name;
  dbId<_dbLevelShifter> _next_entry;
  dbId<_dbPowerDomain> _domain;
  dbVector<std::string> _elements;
  dbVector<std::string> _exclude_elements;
  std::string _source;
  std::string _sink;
  bool _use_functional_equivalence;
  std::string _applies_to;
  std::string _applies_to_boundary;
  std::string _rule;
  float _threshold;
  bool _no_shift;
  bool _force_shift;
  std::string _location;
  std::string _input_supply;
  std::string _output_supply;
  std::string _internal_supply;
  std::string _name_prefix;
  std::string _name_suffix;
  dbVector<std::pair<std::string, std::string>> _instances;
  std::string _cell_name;
  std::string _cell_input;
  std::string _cell_output;
};
dbIStream& operator>>(dbIStream& stream, _dbLevelShifter& obj);
dbOStream& operator<<(dbOStream& stream, const _dbLevelShifter& obj);
}  // namespace odb
   // Generator Code End Header
