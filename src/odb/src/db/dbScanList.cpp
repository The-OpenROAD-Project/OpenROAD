// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanList.h"

#include "dbDatabase.h"
#include "dbDft.h"
#include "dbScanChain.h"
#include "dbScanInst.h"
#include "dbScanInstItr.h"
#include "dbScanPartition.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {
template class dbTable<_dbScanList>;

bool _dbScanList::operator==(const _dbScanList& rhs) const
{
  return _scan_insts == rhs._scan_insts;
}

bool _dbScanList::operator<(const _dbScanList& rhs) const
{
  return true;
}

_dbScanList::_dbScanList(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbScanList& obj)
{
  stream >> obj._scan_insts;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanList& obj)
{
  stream << obj._scan_insts;
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

dbSet<dbScanInst> dbScanList::getScanInsts() const
{
  _dbScanList* scan_list = (_dbScanList*) this;
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  _dbDft* dft = (_dbDft*) scan_chain->getOwner();
  _dbBlock* block = (_dbBlock*) dft->getOwner();
  return dbSet<dbScanInst>(scan_list, block->_scan_list_scan_inst_itr);
}

// User Code Begin dbScanListPublicMethods
dbScanInst* dbScanList::add(dbInst* inst)
{
  dbScanInst* scan_inst = dbScanInst::create(this, inst);
  scan_inst->insertInScanList(this);
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
