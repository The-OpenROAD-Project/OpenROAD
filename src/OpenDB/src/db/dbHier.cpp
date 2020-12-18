///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "dbHier.h"

#include "db.h"
#include "dbBTerm.h"
#include "dbBlock.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbInst.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

template class dbTable<_dbHier>;

bool _dbHier::operator==(const _dbHier& rhs) const
{
  if (_inst != rhs._inst)
    return false;

  if (_child_block != rhs._child_block)
    return false;

  if (_child_bterms != rhs._child_bterms)
    return false;

  return true;
}

void _dbHier::differences(dbDiff&        diff,
                          const char*    field,
                          const _dbHier& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_inst);
  DIFF_FIELD(_child_block);
  DIFF_VECTOR(_child_bterms);
  DIFF_END
}

void _dbHier::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_inst);
  DIFF_OUT_FIELD(_child_block);
  DIFF_OUT_VECTOR(_child_bterms);
  DIFF_END
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
  _dbInst*  inst   = (_dbInst*) inst_;
  _dbBlock* child  = (_dbBlock*) child_;
  _dbBlock* parent = (_dbBlock*) inst->getOwner();

  _dbMaster*     master = (_dbMaster*) inst_->getMaster();
  dbSet<dbBTerm> bterms = ((dbBlock*) child)->getBTerms();
  dbSet<dbMTerm> mterms = ((dbMaster*) master)->getMTerms();

  // bterms do not map 1-to-1 to mterms.
  if (mterms.size() != bterms.size())
    return NULL;

  // initialize the hier structure
  _dbHier* hier      = parent->_hier_tbl->create();
  hier->_inst        = inst->getOID();
  hier->_child_block = child->getOID();
  hier->_child_bterms.resize(child->_bterm_tbl->size());

  // create "down-hier" mapping to bterms
  dbSet<dbBTerm>::iterator itr;

  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    _dbBTerm* bterm = (_dbBTerm*) *itr;
    _dbMTerm* mterm = master->_mterm_hash.find(bterm->_name);

    // bterms do not map 1-to-1 to mterms.
    if (mterm == NULL) {
      parent->_hier_tbl->destroy(hier);
      return NULL;
    }

    hier->_child_bterms[mterm->_order_id] = bterm->getOID();
  }

  // create "up-hier" mapping to iterms
  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    _dbBTerm* bterm      = (_dbBTerm*) *itr;
    _dbMTerm* mterm      = master->_mterm_hash.find(bterm->_name);
    bterm->_parent_block = parent->getOID();
    bterm->_parent_iterm = inst->_iterms[mterm->_order_id];
  }

  // bind hier to inst
  inst->_hierarchy = hier->getOID();

  // point child-block to parent inst
  child->_parent_block = parent->getOID();
  child->_parent_inst  = inst->getOID();

  return hier;
}

void _dbHier::destroy(_dbHier* hier)
{
  _dbBlock* parent = (_dbBlock*) hier->getOwner();
  _dbChip*  chip   = (_dbChip*) parent->getOwner();
  _dbBlock* child  = chip->_block_tbl->getPtr(hier->_child_block);
  _dbInst*  inst   = parent->_inst_tbl->getPtr(hier->_inst);

  // unbind inst from hier
  inst->_hierarchy = 0;

  // unbind child bterms from hier
  dbSet<dbBTerm>           bterms = ((dbBlock*) child)->getBTerms();
  dbSet<dbBTerm>::iterator itr;

  for (itr = bterms.begin(); itr != bterms.end(); ++itr) {
    _dbBTerm* bterm      = (_dbBTerm*) *itr;
    bterm->_parent_block = 0;
    bterm->_parent_iterm = 0;
  }

  // unbind child block to inst
  child->_parent_block = 0;
  child->_parent_inst  = 0;

  // destroy the hier object...
  parent->_hier_tbl->destroy(hier);
}

}  // namespace odb
