// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbTable.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbGDSBoundary;
template <class T>
class dbTable;
class _dbGDSBox;
class _dbGDSPath;
class _dbGDSSRef;
class _dbGDSARef;
class _dbGDSText;

class _dbGDSStructure : public _dbObject
{
 public:
  _dbGDSStructure(_dbDatabase*);

  ~_dbGDSStructure();

  bool operator==(const _dbGDSStructure& rhs) const;
  bool operator!=(const _dbGDSStructure& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSStructure& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbGDSStructure> _next_entry;
  dbTable<_dbGDSBoundary>* boundaries_;
  dbTable<_dbGDSBox>* boxes_;
  dbTable<_dbGDSPath>* paths_;
  dbTable<_dbGDSSRef>* srefs_;
  dbTable<_dbGDSARef>* arefs_;
  dbTable<_dbGDSText>* texts_;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSStructure& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSStructure& obj);
}  // namespace odb
   // Generator Code End Header
