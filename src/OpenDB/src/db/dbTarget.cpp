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

#include "dbTarget.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"

namespace odb {

template class dbTable<_dbTarget>;

bool _dbTarget::operator==(const _dbTarget& rhs) const
{
  if (_point != rhs._point)
    return false;

  if (_mterm != rhs._mterm)
    return false;

  if (_layer != rhs._layer)
    return false;

  if (_next != rhs._next)
    return false;

  return true;
}

void _dbTarget::differences(dbDiff&          diff,
                            const char*      field,
                            const _dbTarget& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_point);
  DIFF_FIELD(_mterm);
  DIFF_FIELD(_layer);
  DIFF_FIELD(_next);
  DIFF_END
}

void _dbTarget::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_point);
  DIFF_OUT_FIELD(_mterm);
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_next);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbTarget - Methods
//
////////////////////////////////////////////////////////////////////

dbMaster* dbTarget::getMaster()
{
  return (dbMaster*) getImpl()->getOwner();
}

dbMTerm* dbTarget::getMTerm()
{
  _dbTarget* target = (_dbTarget*) this;
  _dbMaster* master = (_dbMaster*) target->getOwner();
  return (dbMTerm*) master->_mterm_tbl->getPtr(target->_mterm);
}

dbTechLayer* dbTarget::getTechLayer()
{
  _dbTarget*  target = (_dbTarget*) this;
  dbDatabase* db     = (dbDatabase*) target->getDatabase();
  _dbTech*    tech   = (_dbTech*) db->getTech();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(target->_layer);
}

Point dbTarget::getPoint()
{
  _dbTarget* target = (_dbTarget*) this;
  return target->_point;
}

dbTarget* dbTarget::create(dbMTerm* mterm_, dbTechLayer* layer_, Point point)
{
  _dbMTerm*     mterm  = (_dbMTerm*) mterm_;
  _dbMaster*    master = (_dbMaster*) mterm->getOwner();
  _dbTechLayer* layer  = (_dbTechLayer*) layer_;

  _dbTarget* target = master->_target_tbl->create();
  target->_point    = point;
  target->_mterm    = mterm->getOID();
  target->_layer    = layer->getOID();
  target->_next     = mterm->_targets;
  mterm->_targets   = target->getOID();
  return (dbTarget*) target;
}

void dbTarget::destroy(dbTarget* target_)
{
  _dbTarget* target = (_dbTarget*) target_;
  _dbMaster* master = (_dbMaster*) target->getOwner();

  // unlink target from the mterm
  _dbMTerm*  mterm = (_dbMTerm*) master->_mterm_tbl->getPtr(target->_mterm);
  uint       tid   = target->getOID();
  uint       id    = mterm->_targets;
  _dbTarget* p     = 0;

  while (id) {
    _dbTarget* t = master->_target_tbl->getPtr(id);

    if (id == tid) {
      if (p == NULL)
        mterm->_targets = t->_next;
      else
        p->_next = t->_next;

      break;
    }

    id = t->_next;
    p  = t;
  }

  dbProperty::destroyProperties(target);
  master->_target_tbl->destroy(target);
}

dbSet<dbTarget>::iterator dbTarget::destroy(dbSet<dbTarget>::iterator& itr)
{
  dbTarget*                 t    = *itr;
  dbSet<dbTarget>::iterator next = ++itr;
  destroy(t);
  return next;
}

dbTarget* dbTarget::getTarget(dbMaster* master_, uint dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbTarget*) master->_target_tbl->getPtr(dbid_);
}

}  // namespace odb
