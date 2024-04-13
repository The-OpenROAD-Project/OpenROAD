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

struct dbTechLayerMinCutRuleFlags
{
  bool per_cut_class_ : 1;
  bool within_cut_dist_valid : 1;
  bool from_above_ : 1;
  bool from_below_ : 1;
  bool length_valid_ : 1;
  bool area_valid_ : 1;
  bool area_within_dist_valid_ : 1;
  bool same_metal_overlap : 1;
  bool fully_enclosed_ : 1;
  uint spare_bits_ : 23;
};

class _dbTechLayerMinCutRule : public _dbObject
{
 public:
  _dbTechLayerMinCutRule(_dbDatabase*, const _dbTechLayerMinCutRule& r);
  _dbTechLayerMinCutRule(_dbDatabase*);

  ~_dbTechLayerMinCutRule() = default;

  bool operator==(const _dbTechLayerMinCutRule& rhs) const;
  bool operator!=(const _dbTechLayerMinCutRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerMinCutRule& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbTechLayerMinCutRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;

  dbTechLayerMinCutRuleFlags flags_;
  int num_cuts_;
  std::map<std::string, int> cut_class_cuts_map_;
  int width_;
  int within_cut_dist;
  int length_;
  int length_within_dist_;
  int area_;
  int area_within_dist_;
};
dbIStream& operator>>(dbIStream& stream, _dbTechLayerMinCutRule& obj);
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMinCutRule& obj);
}  // namespace odb
   // Generator Code End Header