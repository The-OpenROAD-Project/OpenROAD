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
#include "dbScanPartition.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbScanPin.h"
#include "dbTable.h"
#include "dbTable.hpp"
namespace odb {
template class dbTable<_dbScanPartition>;

bool _dbScanPartition::operator==(const _dbScanPartition& rhs) const
{
  if (start_ != rhs.start_) {
    return false;
  }
  if (stop_ != rhs.stop_) {
    return false;
  }
  if (name_ != rhs.name_) {
    return false;
  }

  return true;
}

bool _dbScanPartition::operator<(const _dbScanPartition& rhs) const
{
  return true;
}

void _dbScanPartition::differences(dbDiff& diff,
                                   const char* field,
                                   const _dbScanPartition& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(start_);
  DIFF_FIELD(stop_);
  DIFF_FIELD(name_);
  DIFF_END
}

void _dbScanPartition::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(start_);
  DIFF_OUT_FIELD(stop_);
  DIFF_OUT_FIELD(name_);

  DIFF_END
}

_dbScanPartition::_dbScanPartition(_dbDatabase* db)
{
}

_dbScanPartition::_dbScanPartition(_dbDatabase* db, const _dbScanPartition& r)
{
  start_ = r.start_;
  stop_ = r.stop_;
  name_ = r.name_;
}

dbIStream& operator>>(dbIStream& stream, _dbScanPartition& obj)
{
  stream >> obj.start_;
  stream >> obj.stop_;
  stream >> obj.name_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanPartition& obj)
{
  stream << obj.start_;
  stream << obj.stop_;
  stream << obj.name_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbScanPartition - Methods
//
////////////////////////////////////////////////////////////////////

}  // namespace odb
   // Generator Code End Cpp