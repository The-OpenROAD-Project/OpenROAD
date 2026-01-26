// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbDft.h"

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbScanChain.h"
#include "dbScanPin.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
namespace odb {
template class dbTable<_dbDft>;

bool _dbDft::operator==(const _dbDft& rhs) const
{
  if (scan_inserted_ != rhs.scan_inserted_) {
    return false;
  }
  if (*scan_pins_ != *rhs.scan_pins_) {
    return false;
  }
  if (*scan_chains_ != *rhs.scan_chains_) {
    return false;
  }

  return true;
}

bool _dbDft::operator<(const _dbDft& rhs) const
{
  return true;
}

_dbDft::_dbDft(_dbDatabase* db)
{
  scan_inserted_ = false;
  scan_pins_ = new dbTable<_dbScanPin>(
      db, this, (GetObjTbl_t) &_dbDft::getObjectTable, dbScanPinObj);
  scan_chains_ = new dbTable<_dbScanChain>(
      db, this, (GetObjTbl_t) &_dbDft::getObjectTable, dbScanChainObj);
}

dbIStream& operator>>(dbIStream& stream, _dbDft& obj)
{
  stream >> obj.scan_inserted_;
  stream >> *obj.scan_pins_;
  stream >> *obj.scan_chains_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbDft& obj)
{
  stream << obj.scan_inserted_;
  stream << *obj.scan_pins_;
  stream << *obj.scan_chains_;
  return stream;
}

dbObjectTable* _dbDft::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbScanPinObj:
      return scan_pins_;
    case dbScanChainObj:
      return scan_chains_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbDft::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  scan_pins_->collectMemInfo(info.children["scan_pins_"]);

  scan_chains_->collectMemInfo(info.children["scan_chains_"]);
}

_dbDft::~_dbDft()
{
  delete scan_pins_;
  delete scan_chains_;
}

// User Code Begin PrivateMethods
void _dbDft::initialize()
{
  scan_inserted_ = false;
}
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbDft - Methods
//
////////////////////////////////////////////////////////////////////

void dbDft::setScanInserted(bool scan_inserted)
{
  _dbDft* obj = (_dbDft*) this;

  obj->scan_inserted_ = scan_inserted;
}

bool dbDft::isScanInserted() const
{
  _dbDft* obj = (_dbDft*) this;
  return obj->scan_inserted_;
}

dbSet<dbScanChain> dbDft::getScanChains() const
{
  _dbDft* obj = (_dbDft*) this;
  return dbSet<dbScanChain>(obj, obj->scan_chains_);
}

}  // namespace odb
// Generator Code End Cpp
