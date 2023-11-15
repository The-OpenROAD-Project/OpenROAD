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
  if (name != rhs.name)
    return false;
  if (length != rhs.length)
    return false;
  if (scanIn != rhs.scanIn)
    return false;
  if (scanOut != rhs.scanOut)
    return false;
  if (scanClock != rhs.scanClock)
    return false;
  if (scanEnable != rhs.scanEnable)
    return false;
  if (testMode != rhs.testMode)
    return false;

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
  DIFF_FIELD(name);
  DIFF_FIELD(length);
  DIFF_FIELD(scanIn);
  DIFF_FIELD(scanOut);
  DIFF_FIELD(scanClock);
  DIFF_FIELD(scanEnable);
  DIFF_FIELD(testMode);
  DIFF_END
}

void _dbScanChain::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(name);
  DIFF_OUT_FIELD(length);
  DIFF_OUT_FIELD(scanIn);
  DIFF_OUT_FIELD(scanOut);
  DIFF_OUT_FIELD(scanClock);
  DIFF_OUT_FIELD(scanEnable);
  DIFF_OUT_FIELD(testMode);

  DIFF_END
}

_dbScanChain::_dbScanChain(_dbDatabase* db)
{
}

_dbScanChain::_dbScanChain(_dbDatabase* db, const _dbScanChain& r)
{
  name = r.name;
  length = r.length;
  scanIn = r.scanIn;
  scanOut = r.scanOut;
  scanClock = r.scanClock;
  scanEnable = r.scanEnable;
  testMode = r.testMode;
}

dbIStream& operator>>(dbIStream& stream, _dbScanChain& obj)
{
  stream >> obj.name;
  stream >> obj.length;
  stream >> obj.cells;
  stream >> obj.scanIn;
  stream >> obj.scanOut;
  stream >> obj.scanClock;
  stream >> obj.scanEnable;
  stream >> obj.testMode;
  stream >> obj.partitions;
  stream >> obj.scanInsts;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanChain& obj)
{
  stream << obj.name;
  stream << obj.length;
  stream << obj.cells;
  stream << obj.scanIn;
  stream << obj.scanOut;
  stream << obj.scanClock;
  stream << obj.scanEnable;
  stream << obj.testMode;
  stream << obj.partitions;
  stream << obj.scanInsts;
  return stream;
}

_dbScanChain::~_dbScanChain()
{
}

////////////////////////////////////////////////////////////////////
//
// dbScanChain - Methods
//
////////////////////////////////////////////////////////////////////

void dbScanChain::setLength(uint length)
{
  _dbScanChain* obj = (_dbScanChain*) this;

  obj->length = length;
}

uint dbScanChain::getLength() const
{
  _dbScanChain* obj = (_dbScanChain*) this;
  return obj->length;
}

}  // namespace odb
   // Generator Code End Cpp