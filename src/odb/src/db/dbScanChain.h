// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbScanPin.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <variant>

#include "odb/db.h"
#include "odb/dbObject.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbScanPartition;
class dbScanPin;

class _dbScanChain : public _dbObject
{
 public:
  _dbScanChain(_dbDatabase*);

  ~_dbScanChain();

  bool operator==(const _dbScanChain& rhs) const;
  bool operator!=(const _dbScanChain& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbScanChain& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  std::variant<dbBTerm*, dbITerm*> getPin(const dbId<dbScanPin>& scan_pin_id);
  void setPin(dbId<dbScanPin> _dbScanChain::*field, dbBTerm* pin);
  void setPin(dbId<dbScanPin> _dbScanChain::*field, dbITerm* pin);
  // User Code End Methods

  std::string name_;
  dbId<dbScanPin> scan_in_;
  dbId<dbScanPin> scan_out_;
  dbId<dbScanPin> scan_enable_;
  dbId<dbScanPin> test_mode_;
  std::string test_mode_name_;
  dbTable<_dbScanPartition>* scan_partitions_;
};
dbIStream& operator>>(dbIStream& stream, _dbScanChain& obj);
dbOStream& operator<<(dbOStream& stream, const _dbScanChain& obj);
}  // namespace odb
   // Generator Code End Header
