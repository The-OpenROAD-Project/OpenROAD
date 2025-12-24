// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <regex>
#include <string>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbMTerm.h"
#include "dbNet.h"
#include "dbRegion.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <map>
#include <set>
#include <utility>
#include <vector>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbRegion;
class _dbNet;
// User Code Begin Classes
class dbInst;
class dbMaster;
class dbMTerm;
class dbITerm;
// User Code End Classes

class _dbGlobalConnect : public _dbObject
{
 public:
  _dbGlobalConnect(_dbDatabase*);

  bool operator==(const _dbGlobalConnect& rhs) const;
  bool operator!=(const _dbGlobalConnect& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbGlobalConnect& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  void setupRegex();
  static void testRegex(utl::Logger* logger,
                        const std::string& pattern,
                        const std::string& type);
  std::map<dbMaster*, std::set<dbMTerm*>> getMTermMapping();
  std::set<dbMTerm*> getMTermMapping(dbMaster* master,
                                     const std::regex& pin_regex) const;
  std::pair<std::set<dbITerm*>, std::set<dbITerm*>> connect(
      const std::vector<dbInst*>& insts,
      bool force);
  bool appliesTo(dbInst* inst) const;
  bool needsModification(dbInst* inst) const;
  // User Code End Methods

  dbId<_dbRegion> region_;
  dbId<_dbNet> net_;
  std::string inst_pattern_;
  std::string pin_pattern_;
  std::regex inst_regex_;
};
dbIStream& operator>>(dbIStream& stream, _dbGlobalConnect& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGlobalConnect& obj);
}  // namespace odb
   // Generator Code End Header
