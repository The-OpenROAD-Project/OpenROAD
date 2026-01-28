// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanList.h"

#include <cstdint>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbDft.h"
#include "dbScanChain.h"
#include "dbScanListScanInstItr.h"
#include "dbScanPartition.h"
#include "dbTable.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbScanList>;

bool _dbScanList::operator==(const _dbScanList& rhs) const
{
  if (unused_ != rhs.unused_) {
    return false;
  }
  if (first_scan_inst_ != rhs.first_scan_inst_) {
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
  unused_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbScanList& obj)
{
  if (obj.getDatabase()->isSchema(kSchemaBlockOwnsScanInsts)) {
    stream >> obj.unused_;
  }
  if (obj.getDatabase()->isSchema(kSchemaBlockOwnsScanInsts)) {
    stream >> obj.first_scan_inst_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanList& obj)
{
  stream << obj.unused_;
  stream << obj.first_scan_inst_;
  return stream;
}

void _dbScanList::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbScanList - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbScanListPublicMethods
dbSet<dbScanInst> dbScanList::getScanInsts() const
{
  _dbScanList* scan_list = (_dbScanList*) this;
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  _dbDft* dft = (_dbDft*) scan_chain->getOwner();
  _dbBlock* block = (_dbBlock*) dft->getOwner();
  return dbSet<dbScanInst>(scan_list, block->scan_list_scan_inst_itr_);
}

dbScanInst* dbScanList::add(dbInst* inst)
{
  dbScanInst* scan_inst = dbScanInst::create(this, inst);
  scan_inst->insertAtFront(this);
  return scan_inst;
}

dbScanList* dbScanList::create(dbScanPartition* scan_partition)
{
  _dbScanPartition* obj = (_dbScanPartition*) scan_partition;
  return (dbScanList*) obj->scan_lists_->create();
}
// User Code End dbScanListPublicMethods
}  // namespace odb
// Generator Code End Cpp
