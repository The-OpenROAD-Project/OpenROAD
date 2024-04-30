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

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;

struct dbTechLayerKeepOutZoneRuleFlags
{
  bool same_mask_ : 1;
  bool same_metal_ : 1;
  bool diff_metal_ : 1;
  bool except_aligned_side_ : 1;
  bool except_aligned_end_ : 1;
  uint spare_bits_ : 27;
};

class _dbTechLayerKeepOutZoneRule : public _dbObject
{
 public:
  _dbTechLayerKeepOutZoneRule(_dbDatabase*,
                              const _dbTechLayerKeepOutZoneRule& r);
  _dbTechLayerKeepOutZoneRule(_dbDatabase*);

  ~_dbTechLayerKeepOutZoneRule() = default;

  bool operator==(const _dbTechLayerKeepOutZoneRule& rhs) const;
  bool operator!=(const _dbTechLayerKeepOutZoneRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerKeepOutZoneRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerKeepOutZoneRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  dbTechLayerKeepOutZoneRuleFlags flags_;
  std::string first_cut_class_;
  std::string second_cut_class_;
  int aligned_spacing_;
  int side_extension_;
  int forward_extension_;
  int end_side_extension_;
  int end_forward_extension_;
  int side_side_extension_;
  int side_forward_extension_;
  int spiral_extension_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerKeepOutZoneRule& obj);
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerKeepOutZoneRule& obj);
}  // namespace odb
   // Generator Code End Header