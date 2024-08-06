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

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb/odb.h"

// User Code Begin Includes
#include "dbModuleBusPortModBTermItr.h"
#include "dbVector.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbModBTerm;
class _dbModule;

class _dbBusPort : public _dbObject
{
 public:
  _dbBusPort(_dbDatabase*, const _dbBusPort& r);
  _dbBusPort(_dbDatabase*);

  ~_dbBusPort();

  bool operator==(const _dbBusPort& rhs) const;
  bool operator!=(const _dbBusPort& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBusPort& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbBusPort& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  uint _flags;
  int _from;
  int _to;
  dbId<_dbModBTerm> _port;
  dbId<_dbModBTerm> _members;
  dbId<_dbModBTerm> _last;
  dbId<_dbModule> _parent;

  // User Code Begin Fields
  dbModuleBusPortModBTermItr* _members_iter = nullptr;
  int size() { return abs(_from - _to) + 1; }
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbBusPort& obj);
dbOStream& operator<<(dbOStream& stream, const _dbBusPort& obj);
}  // namespace odb
   // Generator Code End Header
