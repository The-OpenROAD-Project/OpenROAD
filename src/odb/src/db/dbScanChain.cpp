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
#include "dbScanChain.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbScanPin.h"
#include "dbTable.h"
#include "dbTable.hpp"
namespace odb {
template class dbTable<_dbScanChain>;

bool _dbScanChain::operator==(const _dbScanChain& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (scan_in_ != rhs.scan_in_) {
    return false;
  }
  if (scan_out_ != rhs.scan_out_) {
    return false;
  }
  if (scan_clock_ != rhs.scan_clock_) {
    return false;
  }
  if (scan_enable_ != rhs.scan_enable_) {
    return false;
  }
  if (test_mode_ != rhs.test_mode_) {
    return false;
  }

  return true;
}

bool _dbScanChain::operator<(const _dbScanChain& rhs) const
{
  return true;
}

void _dbScanChain::differences(dbDiff& diff,
                               const char* field,
                               const _dbScanChain& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(name_);
  DIFF_FIELD(length_);
  DIFF_FIELD(scan_in_);
  DIFF_FIELD(scan_out_);
  DIFF_FIELD(scan_clock_);
  DIFF_FIELD(scan_enable_);
  DIFF_FIELD(test_mode_);
  DIFF_END
}

void _dbScanChain::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(name_);
  DIFF_OUT_FIELD(length_);
  DIFF_OUT_FIELD(scan_in_);
  DIFF_OUT_FIELD(scan_out_);
  DIFF_OUT_FIELD(scan_clock_);
  DIFF_OUT_FIELD(scan_enable_);
  DIFF_OUT_FIELD(test_mode_);

  DIFF_END
}

_dbScanChain::_dbScanChain(_dbDatabase* db)
{
}

_dbScanChain::_dbScanChain(_dbDatabase* db, const _dbScanChain& r)
{
  name_ = r.name_;
  length_ = r.length_;
  scan_in_ = r.scan_in_;
  scan_out_ = r.scan_out_;
  scan_clock_ = r.scan_clock_;
  scan_enable_ = r.scan_enable_;
  test_mode_ = r.test_mode_;
}

dbIStream& operator>>(dbIStream& stream, _dbScanChain& obj)
{
  stream >> obj.name_;
  stream >> obj.length_;
  stream >> obj.cells_;
  stream >> obj.scan_in_;
  stream >> obj.scan_out_;
  stream >> obj.scan_clock_;
  stream >> obj.scan_enable_;
  stream >> obj.test_mode_;
  stream >> obj.partitions_;
  stream >> obj.scan_insts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanChain& obj)
{
  stream << obj.name_;
  stream << obj.length_;
  stream << obj.cells_;
  stream << obj.scan_in_;
  stream << obj.scan_out_;
  stream << obj.scan_clock_;
  stream << obj.scan_enable_;
  stream << obj.test_mode_;
  stream << obj.partitions_;
  stream << obj.scan_insts_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbScanChain - Methods
//
////////////////////////////////////////////////////////////////////

}  // namespace odb
   // Generator Code End Cpp