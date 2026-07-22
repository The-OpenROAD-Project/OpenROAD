// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbHier.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbInst.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "odb/db.h"
#include "odb/dbSet.h"

namespace odb {

template class dbTable<_dbHier>;

bool _dbHier::operator==(const _dbHier& rhs) const
{
  if (inst_ != rhs.inst_) {
    return false;
  }

  if (child_block_ != rhs.child_block_) {
    return false;
  }

  if (child_bterms_ != rhs.child_bterms_) {
    return false;
  }

  return true;
}

_dbHier::_dbHier(_dbDatabase*)
{
}

_dbHier::_dbHier(_dbDatabase*, const _dbHier& i)
    : inst_(i.inst_),
      child_block_(i.child_block_),
      child_bterms_(i.child_bterms_)
{
}

dbOStream& operator<<(dbOStream& stream, const _dbHier& hier)
{
  stream << hier.inst_;
  stream << hier.child_block_;
  stream << hier.child_bterms_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbHier& hier)
{
  stream >> hier.inst_;
  stream >> hier.child_block_;
  stream >> hier.child_bterms_;
  return stream;
}

_dbHier* _dbHier::create(dbInst* inst_, dbBlock* child_)
{
  _dbInst* inst = (_dbInst*) inst_;
  _dbBlock* child = (_dbBlock*) child_;
  _dbBlock* parent = (_dbBlock*) inst->getOwner();

  _dbMaster* master = (_dbMaster*) inst_->getMaster();
  dbSet<dbBTerm> bterms = ((dbBlock*) child)->getBTerms();
  dbSet<dbMTerm> mterms = ((dbMaster*) master)->getMTerms();

  // bterms do not map 1-to-1 to mterms.
  if (mterms.size() != bterms.size()) {
    return nullptr;
  }

  // initialize the hier structure
  _dbHier* hier = parent->hier_tbl_->create();
  hier->inst_ = inst->getOID();
  hier->child_block_ = child->getOID();
  hier->child_bterms_.resize(child->bterm_tbl_->size());

  // create "down-hier" mapping to bterms
  for (dbBTerm* bterm : bterms) {
    _dbMTerm* mterm = master->mterm_hash_.find(bterm->getConstName());

    // bterms do not map 1-to-1 to mterms.
    if (mterm == nullptr) {
      parent->hier_tbl_->destroy(hier);
      return nullptr;
    }

    hier->child_bterms_[mterm->order_id_] = bterm->getId();
  }

  // create "up-hier" mapping to iterms
  for (dbBTerm* bterm : bterms) {
    _dbBTerm* bterm_impl = (_dbBTerm*) bterm;
    _dbMTerm* mterm = master->mterm_hash_.find(bterm_impl->name_);
    bterm_impl->parent_block_ = parent->getOID();
    bterm_impl->parent_iterm_ = inst->iterms_[mterm->order_id_];
  }

  // bind hier to inst
  inst->hierarchy_ = hier->getOID();

  // point child-block to parent inst
  child->parent_block_ = parent->getOID();
  child->parent_inst_ = inst->getOID();

  return hier;
}

void _dbHier::destroy(_dbHier* hier)
{
  _dbBlock* parent = (_dbBlock*) hier->getOwner();
  _dbChip* chip = (_dbChip*) parent->getOwner();
  _dbBlock* child = chip->block_tbl_->getPtr(hier->child_block_);
  _dbInst* inst = parent->inst_tbl_->getPtr(hier->inst_);

  // unbind inst from hier
  inst->hierarchy_ = 0;

  // unbind child bterms from hier
  for (dbBTerm* bterm : ((dbBlock*) child)->getBTerms()) {
    _dbBTerm* bterm_impl = (_dbBTerm*) bterm;
    bterm_impl->parent_block_ = 0;
    bterm_impl->parent_iterm_ = 0;
  }

  // unbind child block to inst
  child->parent_block_ = 0;
  child->parent_inst_ = 0;

  // destroy the hier object...
  parent->hier_tbl_->destroy(hier);
}

void _dbHier::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["child_bterms"].add(child_bterms_);
}

}  // namespace odb
