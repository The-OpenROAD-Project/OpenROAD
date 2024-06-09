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
#include "dbScanList.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbScanChain.h"
#include "dbScanInst.h"
#include "dbScanPartition.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
namespace odb {
template class dbTable<_dbScanList>;

bool _dbScanList::operator==(const _dbScanList& rhs) const
{
  if (*scan_insts_ != *rhs.scan_insts_) {
    return false;
  }

  return true;
}

bool _dbScanList::operator<(const _dbScanList& rhs) const
{
  return true;
}

void _dbScanList::differences(dbDiff& diff,
                              const char* field,
                              const _dbScanList& rhs) const
{
  DIFF_BEGIN
  DIFF_TABLE(scan_insts_);
  DIFF_END
}

void _dbScanList::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_TABLE(scan_insts_);

  DIFF_END
}

_dbScanList::_dbScanList(_dbDatabase* db)
{
  scan_insts_ = new dbTable<_dbScanInst>(
      db, this, (GetObjTbl_t) &_dbScanList::getObjectTable, dbScanInstObj);
}

_dbScanList::_dbScanList(_dbDatabase* db, const _dbScanList& r)
{
  scan_insts_ = new dbTable<_dbScanInst>(db, this, *r.scan_insts_);
}

dbIStream& operator>>(dbIStream& stream, _dbScanList& obj)
{
  stream >> *obj.scan_insts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanList& obj)
{
  stream << *obj.scan_insts_;
  return stream;
}

dbObjectTable* _dbScanList::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbScanInstObj:
      return scan_insts_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbScanList::~_dbScanList()
{
  delete scan_insts_;
}

////////////////////////////////////////////////////////////////////
//
// dbScanList - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbScanInst> dbScanList::getScanInsts() const
{
  _dbScanList* obj = (_dbScanList*) this;
  return dbSet<dbScanInst>(obj, obj->scan_insts_);
}

// User Code Begin dbScanListPublicMethods
dbScanInst* dbScanList::add(dbInst* inst)
{
  return dbScanInst::create(this, inst);
}

dbScanList* dbScanList::create(dbScanPartition* scan_partition)
{
  _dbScanPartition* obj = (_dbScanPartition*) scan_partition;
  return (dbScanList*) obj->scan_lists_->create();
}
// User Code End dbScanListPublicMethods
}  // namespace odb
// Generator Code End Cpp
