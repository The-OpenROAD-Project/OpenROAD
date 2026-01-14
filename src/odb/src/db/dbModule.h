// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include <map>
#include <string>
#include <unordered_map>

#include "dbHashTable.h"
#include "dbModulePortItr.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbInst;
class _dbModInst;
class _dbModNet;
class _dbModBTerm;
// User Code Begin Classes
class dbITerm;
class dbInst;
class dbModBTerm;
class dbModInst;
class dbModule;
// User Code End Classes

class _dbModule : public _dbObject
{
 public:
  _dbModule(_dbDatabase*);

  ~_dbModule();

  bool operator==(const _dbModule& rhs) const;
  bool operator!=(const _dbModule& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbModule& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods

  // This is only used when destroying an inst
  void removeInst(dbInst* inst);

  // Copy and uniquify a given module based on current instance
  using modBTMap = std::map<dbModBTerm*, dbModBTerm*>;
  using ITMap = std::map<dbITerm*, dbITerm*>;

  static void copy(dbModule* old_module,
                   dbModule* new_module,
                   dbModInst* new_mod_inst,
                   modBTMap& mod_bt_map);

  static void copyModulePorts(dbModule* old_module,
                              dbModule* new_module,
                              modBTMap& mod_bt_map);
  static void copyModuleInsts(dbModule* old_module,
                              dbModule* new_module,
                              dbModInst* new_mod_inst,
                              ITMap& it_map);
  static void copyModuleModNets(dbModule* old_module,
                                dbModule* new_module,
                                modBTMap& mod_bt_map,
                                ITMap& it_map);
  static void copyModuleBoundaryIO(dbModule* old_module,
                                   dbModule* new_module,
                                   dbModInst* new_mod_inst);

  // Copy module to child block for future use
  static bool copyToChildBlock(dbModule* module);
  // User Code End Methods

  char* name_;
  dbId<_dbModule> next_entry_;
  dbId<_dbInst> insts_;
  dbId<_dbModInst> mod_inst_;
  dbId<_dbModInst> modinsts_;
  dbId<_dbModNet> modnets_;
  dbId<_dbModBTerm> modbterms_;

  // User Code Begin Fields
  // custom iterator for traversing ports
  // fast access
  std::unordered_map<std::string, dbId<_dbInst>> dbinst_hash_;
  std::unordered_map<std::string, dbId<_dbModInst>> modinst_hash_;
  std::unordered_map<std::string, dbId<_dbModBTerm>> modbterm_hash_;
  std::unordered_map<std::string, dbId<_dbModNet>> modnet_hash_;
  dbModulePortItr* _port_iter = nullptr;
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbModule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbModule& obj);
}  // namespace odb
   // Generator Code End Header
