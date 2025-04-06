// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbHier.h"

#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbHier>;

bool _dbHier::operator==(const _dbHier& rhs) const
{
  if (_inst != rhs._inst) {
    return false;
  }

  if (_child_block != rhs._child_block) {
    return false;
  }

  if (_child_bterms != rhs._child_bterms) {
    return false;
  }

  return true;
}

_dbHier::_dbHier(_dbDatabase*)
{
}

_dbHier::_dbHier(_dbDatabase*, const _dbHier& i)
    : _inst(i._inst),
      _child_block(i._child_block),
      _child_bterms(i._child_bterms)
{
}

_dbHier::~_dbHier()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbHier& hier)
{
  stream << hier._inst;
  stream << hier._child_block;
  stream << hier._child_bterms;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbHier& hier)
{
  stream >> hier._inst;
  stream >> hier._child_block;
  stream >> hier._child_bterms;
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
  _dbHier* hier = parent->_hier_tbl->create();
  hier->_inst = inst->getOID();
  hier->_child_block = child->getOID();
  hier->_child_bterms.resize(child->_bterm_tbl->size());

  // create "down-hier" mapping to bterms
  for (dbBTerm* bterm : bterms) {
    _dbMTerm* mterm = master->_mterm_hash.find(bterm->getConstName());

    // bterms do not map 1-to-1 to mterms.
    if (mterm == nullptr) {
      parent->_hier_tbl->destroy(hier);
      return nullptr;
    }

    hier->_child_bterms[mterm->_order_id] = bterm->getId();
  }

  // create "up-hier" mapping to iterms
  for (dbBTerm* bterm : bterms) {
    _dbBTerm* bterm_impl = (_dbBTerm*) bterm;
    _dbMTerm* mterm = master->_mterm_hash.find(bterm_impl->_name);
    bterm_impl->_parent_block = parent->getOID();
    bterm_impl->_parent_iterm = inst->_iterms[mterm->_order_id];
  }

  // bind hier to inst
  inst->_hierarchy = hier->getOID();

  // point child-block to parent inst
  child->_parent_block = parent->getOID();
  child->_parent_inst = inst->getOID();

  return hier;
}

void _dbHier::destroy(_dbHier* hier)
{
  _dbBlock* parent = (_dbBlock*) hier->getOwner();
  _dbChip* chip = (_dbChip*) parent->getOwner();
  _dbBlock* child = chip->_block_tbl->getPtr(hier->_child_block);
  _dbInst* inst = parent->_inst_tbl->getPtr(hier->_inst);

  // unbind inst from hier
  inst->_hierarchy = 0;

  // unbind child bterms from hier
  for (dbBTerm* bterm : ((dbBlock*) child)->getBTerms()) {
    _dbBTerm* bterm_impl = (_dbBTerm*) bterm;
    bterm_impl->_parent_block = 0;
    bterm_impl->_parent_iterm = 0;
  }

  // unbind child block to inst
  child->_parent_block = 0;
  child->_parent_inst = 0;

  // destroy the hier object...
  parent->_hier_tbl->destroy(hier);
}

void _dbHier::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["child_bterms"].add(_child_bterms);
}

}  // namespace odb
