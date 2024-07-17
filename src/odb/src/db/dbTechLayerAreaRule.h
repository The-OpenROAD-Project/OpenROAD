///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbTechLayer;

struct dbTechLayerAreaRuleFlags
{
  bool except_rectangle_ : 1;
  uint overlap_ : 2;
  uint spare_bits_ : 29;
};

class _dbTechLayerAreaRule : public _dbObject
{
 public:
  _dbTechLayerAreaRule(_dbDatabase*, const _dbTechLayerAreaRule& r);
  _dbTechLayerAreaRule(_dbDatabase*);

  ~_dbTechLayerAreaRule() = default;

  bool operator==(const _dbTechLayerAreaRule& rhs) const;
  bool operator!=(const _dbTechLayerAreaRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerAreaRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerAreaRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  dbTechLayerAreaRuleFlags flags_;
  int area_;
  int except_min_width_;
  int except_edge_length_;
  std::pair<int, int> except_edge_lengths_;
  std::pair<int, int> except_min_size_;
  std::pair<int, int> except_step_;
  dbId<_dbTechLayer> trim_layer_;
  int mask_;
  int rect_width_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerAreaRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerAreaRule& obj);
}  // namespace odb
   // Generator Code End Header
