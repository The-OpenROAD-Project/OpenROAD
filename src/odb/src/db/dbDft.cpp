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
// User Code Begin Includes
#include <vector>

#include "dbBlock.h"
#include "dbProperty.h"
#include "dbScanInst.h"
#include "odb/dbObject.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbDft>;

bool _dbDft::operator==(const _dbDft& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
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
  // NOLINTEND(readability-simplify-boolean-expr)
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

// User Code Begin dbDftPublicMethods
void dbDft::reset()
{
  _dbDft* obj = (_dbDft*) this;
  _dbBlock* block = (_dbBlock*) obj->getOwner();

  // Scan insts belong to the block, not to their scan list, so destroy them
  // before the chains that list them.
  std::vector<dbScanInst*> scan_insts;
  for (dbScanChain* chain : getScanChains()) {
    for (dbScanPartition* partition : chain->getScanPartitions()) {
      for (dbScanList* list : partition->getScanLists()) {
        for (dbScanInst* scan_inst : list->getScanInsts()) {
          scan_insts.push_back(scan_inst);
        }
      }
    }
  }
  for (dbScanInst* scan_inst : scan_insts) {
    _dbScanInst* obj_scan_inst = (_dbScanInst*) scan_inst;
    // Drop the instance's lookup entry so dbInst::getScanInst() no longer
    // finds the destroyed scan inst.
    auto itr = block->inst_scan_inst_map_.find(obj_scan_inst->inst_);
    if (itr != block->inst_scan_inst_map_.end()
        && itr->second == obj_scan_inst->getId()) {
      block->inst_scan_inst_map_.erase(itr);
    }
    dbProperty::destroyProperties(scan_inst);
    block->scan_inst_tbl_->destroy(obj_scan_inst);
  }

  obj->scan_inserted_ = false;
  obj->scan_pins_->clear();
  obj->scan_chains_->clear();
}
// User Code End dbDftPublicMethods
}  // namespace odb
// Generator Code End Cpp
