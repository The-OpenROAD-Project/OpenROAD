// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanPartition.h"

#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbDft.h"
#include "dbScanChain.h"
#include "dbScanList.h"
#include "dbScanPin.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include "odb/dbObject.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbScanPartition>;

bool _dbScanPartition::operator==(const _dbScanPartition& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (*scan_lists_ != *rhs.scan_lists_) {
    return false;
  }

  return true;
}

bool _dbScanPartition::operator<(const _dbScanPartition& rhs) const
{
  return true;
}

_dbScanPartition::_dbScanPartition(_dbDatabase* db)
{
  scan_lists_ = new dbTable<_dbScanList>(
      db, this, (GetObjTbl_t) &_dbScanPartition::getObjectTable, dbScanListObj);
}

dbIStream& operator>>(dbIStream& stream, _dbScanPartition& obj)
{
  stream >> obj.name_;
  stream >> *obj.scan_lists_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanPartition& obj)
{
  stream << obj.name_;
  stream << *obj.scan_lists_;
  return stream;
}

dbObjectTable* _dbScanPartition::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbScanListObj:
      return scan_lists_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
void _dbScanPartition::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  scan_lists_->collectMemInfo(info.children["scan_lists_"]);
}

_dbScanPartition::~_dbScanPartition()
{
  delete scan_lists_;
}

////////////////////////////////////////////////////////////////////
//
// dbScanPartition - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbScanList> dbScanPartition::getScanLists() const
{
  _dbScanPartition* obj = (_dbScanPartition*) this;
  return dbSet<dbScanList>(obj, obj->scan_lists_);
}

// User Code Begin dbScanPartitionPublicMethods

const std::string& dbScanPartition::getName() const
{
  _dbScanPartition* scan_partition = (_dbScanPartition*) this;
  return scan_partition->name_;
}

void dbScanPartition::setName(const std::string& name)
{
  _dbScanPartition* scan_partition = (_dbScanPartition*) this;
  scan_partition->name_ = name;
}

dbScanPartition* dbScanPartition::create(dbScanChain* chain)
{
  _dbScanChain* scan_chain = (_dbScanChain*) chain;
  return (dbScanPartition*) (scan_chain->scan_partitions_->create());
}

// User Code End dbScanPartitionPublicMethods
}  // namespace odb
   // Generator Code End Cpp
