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
#include "dbGDSStructure.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;

class _dbGDSText : public _dbObject
{
 public:
  _dbGDSText(_dbDatabase*, const _dbGDSText& r);
  _dbGDSText(_dbDatabase*);

  ~_dbGDSText() = default;

  bool operator==(const _dbGDSText& rhs) const;
  bool operator!=(const _dbGDSText& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSText& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbGDSText& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  int16_t _layer;
  int16_t _datatype;
  std::vector<Point> _xy;
  std::vector<std::pair<std::int16_t, std::string>> _propattr;
  dbGDSTextPres _presentation;
  int _width;
  dbGDSSTrans _transform;
  std::string _text;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj);
}  // namespace odb
   // Generator Code End Header
