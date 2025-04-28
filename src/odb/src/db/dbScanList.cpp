// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanList.h"

#include "dbDatabase.h"
#include "dbScanChain.h"
#include "dbScanInst.h"
#include "dbScanPartition.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
namespace odb {
template class dbTable<_dbScanList>;

bool _dbScanList::operator==(const _dbScanList& rhs) const
{
  if (*scan_insts_ != *rhs.scan_insts_) {
    return false;
  }

  return true;
}

bool _dbScanList::operator<(const _dbScanList& rhs) const
{
  return true;
}

_dbScanList::_dbScanList(_dbDatabase* db)
{
  scan_insts_ = new dbTable<_dbScanInst>(
      db, this, (GetObjTbl_t) &_dbScanList::getObjectTable, dbScanInstObj);
}

dbIStream& operator>>(dbIStream& stream, _dbScanList& obj)
{
  stream >> *obj.scan_insts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanList& obj)
{
  stream << *obj.scan_insts_;
  return stream;
}

dbObjectTable* _dbScanList::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbScanInstObj:
      return scan_insts_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbScanList::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  scan_insts_->collectMemInfo(info.children_["scan_insts_"]);
}

_dbScanList::~_dbScanList()
{
  delete scan_insts_;
}

////////////////////////////////////////////////////////////////////
//
// dbScanList - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbScanInst> dbScanList::getScanInsts() const
{
  _dbScanList* obj = (_dbScanList*) this;
  return dbSet<dbScanInst>(obj, obj->scan_insts_);
}

// User Code Begin dbScanListPublicMethods
dbScanInst* dbScanList::add(dbInst* inst)
{
  return dbScanInst::create(this, inst);
}

dbScanList* dbScanList::create(dbScanPartition* scan_partition)
{
  _dbScanPartition* obj = (_dbScanPartition*) scan_partition;
  return (dbScanList*) obj->scan_lists_->create();
}
// User Code End dbScanListPublicMethods
}  // namespace odb
// Generator Code End Cpp
