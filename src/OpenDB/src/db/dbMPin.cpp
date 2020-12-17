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

#include "dbMPin.h"

#include "db.h"
#include "dbBoxItr.h"
#include "dbMPinItr.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

template class dbTable<_dbMTerm>;

_dbMPin::_dbMPin(_dbDatabase*)
{
}

_dbMPin::_dbMPin(_dbDatabase*, const _dbMPin& p)
    : _mterm(p._mterm), _geoms(p._geoms), _next_mpin(p._next_mpin)
{
}

_dbMPin::~_dbMPin()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbMPin& mpin)
{
  stream << mpin._mterm;
  stream << mpin._geoms;
  stream << mpin._next_mpin;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbMPin& mpin)
{
  stream >> mpin._mterm;
  stream >> mpin._geoms;
  stream >> mpin._next_mpin;
  return stream;
}

bool _dbMPin::operator==(const _dbMPin& rhs) const
{
  if (_mterm != rhs._mterm)
    return false;

  if (_geoms != rhs._geoms)
    return false;

  if (_next_mpin != rhs._next_mpin)
    return false;

  return true;
}

void _dbMPin::differences(dbDiff&        diff,
                          const char*    field,
                          const _dbMPin& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_mterm);
  DIFF_FIELD(_geoms);
  DIFF_FIELD(_next_mpin);
  DIFF_END
}

void _dbMPin::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_mterm);
  DIFF_OUT_FIELD(_geoms);
  DIFF_OUT_FIELD(_next_mpin);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbMPin - Methods
//
////////////////////////////////////////////////////////////////////

dbMTerm* dbMPin::getMTerm()
{
  _dbMPin*   pin    = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return (dbMTerm*) master->_mterm_tbl->getPtr(pin->_mterm);
}

dbMaster* dbMPin::getMaster()
{
  return (dbMaster*) getImpl()->getOwner();
}

dbSet<dbBox> dbMPin::getGeometry()
{
  _dbMPin*   pin    = (_dbMPin*) this;
  _dbMaster* master = (_dbMaster*) pin->getOwner();
  return dbSet<dbBox>(pin, master->_box_itr);
}

dbMPin* dbMPin::create(dbMTerm* mterm_)
{
  _dbMTerm*  mterm  = (_dbMTerm*) mterm_;
  _dbMaster* master = (_dbMaster*) mterm->getOwner();
  _dbMPin*   mpin   = master->_mpin_tbl->create();
  mpin->_mterm      = mterm->getOID();
  mpin->_next_mpin  = mterm->_pins;
  mterm->_pins      = mpin->getOID();
  return (dbMPin*) mpin;
}

dbMPin* dbMPin::getMPin(dbMaster* master_, uint dbid_)
{
  _dbMaster* master = (_dbMaster*) master_;
  return (dbMPin*) master->_mpin_tbl->getPtr(dbid_);
}

}  // namespace odb
