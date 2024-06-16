///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbDft.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbScanChain.h"
#include "dbScanPin.h"
#include "dbTable.h"
#include "dbTable.hpp"
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

void _dbDft::differences(dbDiff& diff,
                         const char* field,
                         const _dbDft& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(scan_inserted_);
  DIFF_TABLE(scan_pins_);
  DIFF_TABLE(scan_chains_);
  DIFF_END
}

void _dbDft::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(scan_inserted_);
  DIFF_OUT_TABLE(scan_pins_);
  DIFF_OUT_TABLE(scan_chains_);

  DIFF_END
}

_dbDft::_dbDft(_dbDatabase* db)
{
  scan_inserted_ = false;
  scan_pins_ = new dbTable<_dbScanPin>(
      db, this, (GetObjTbl_t) &_dbDft::getObjectTable, dbScanPinObj);
  scan_chains_ = new dbTable<_dbScanChain>(
      db, this, (GetObjTbl_t) &_dbDft::getObjectTable, dbScanChainObj);
}

_dbDft::_dbDft(_dbDatabase* db, const _dbDft& r)
{
  scan_inserted_ = r.scan_inserted_;
  scan_pins_ = new dbTable<_dbScanPin>(db, this, *r.scan_pins_);
  scan_chains_ = new dbTable<_dbScanChain>(db, this, *r.scan_chains_);
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
