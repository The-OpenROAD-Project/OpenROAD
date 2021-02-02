///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

// Generator Code Begin 1
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb.h"
// User Code Begin includes
#include <map>
#include <string>
// User Code End includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
// User Code Begin Classes
// User Code End Classes

struct dbTechLayerCutSpacingTableDefSubRuleFlags
{
  bool _default_valid : 1;
  bool _same_mask : 1;
  bool _same_net : 1;
  bool _same_metal : 1;
  bool _same_via : 1;
  bool _layer_valid : 1;
  bool _no_stack : 1;
  bool _non_zero_enclosure : 1;
  bool _prl_for_aligned_cut : 1;
  bool _center_to_center_valid : 1;
  bool _center_and_edge_valid : 1;
  bool _no_prl : 1;
  bool _prl_valid : 1;
  bool _max_x_y : 1;
  bool _end_extension_valid : 1;
  bool _side_extension_valid : 1;
  bool _exact_aligned_spacing_valid : 1;
  bool _horizontal : 1;
  bool _vertical : 1;
  bool _non_opposite_enclosure_spacing_valid : 1;
  bool _opposite_enclosure_resize_spacing_valid : 1;
  uint _spare_bits : 11;
};
// User Code Begin structs
// User Code End structs

class _dbTechLayerCutSpacingTableDefSubRule : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbTechLayerCutSpacingTableDefSubRuleFlags  _flags;
  int                                        _default;
  char*                                      _second_layer_name;
  dbVector<std::pair<char*, char*>>          _prl_for_aligned_cut_tbl;
  dbVector<std::pair<char*, char*>>          _center_to_center_tbl;
  dbVector<std::pair<char*, char*>>          _center_and_edge_tbl;
  int                                        _prl;
  dbVector<std::tuple<char*, char*, int>>    _prl_tbl;
  int                                        _extension;
  dbVector<std::pair<char*, int>>            _end_extension_tbl;
  dbVector<std::pair<char*, int>>            _side_extension_tbl;
  dbVector<std::pair<char*, int>>            _exact_aligned_spacing_tbl;
  dbVector<std::pair<char*, int>>            _non_opp_enc_spacing_tbl;
  dbVector<std::tuple<char*, int, int, int>> _opp_enc_spacing_tbl;
  dbVector<dbVector<std::pair<int, int>>>    _spacing_tbl;
  std::map<std::string, uint>                _row_map;
  std::map<std::string, uint>                _col_map;

  // User Code Begin fields
  // User Code End fields
  _dbTechLayerCutSpacingTableDefSubRule(
      _dbDatabase*,
      const _dbTechLayerCutSpacingTableDefSubRule& r);
  _dbTechLayerCutSpacingTableDefSubRule(_dbDatabase*);
  ~_dbTechLayerCutSpacingTableDefSubRule();
  bool operator==(const _dbTechLayerCutSpacingTableDefSubRule& rhs) const;
  bool operator!=(const _dbTechLayerCutSpacingTableDefSubRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechLayerCutSpacingTableDefSubRule& rhs) const;
  void differences(dbDiff&                                      diff,
                   const char*                                  field,
                   const _dbTechLayerCutSpacingTableDefSubRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin methods
  // User Code End methods
};
dbIStream& operator>>(dbIStream&                             stream,
                      _dbTechLayerCutSpacingTableDefSubRule& obj);
dbOStream& operator<<(dbOStream&                                   stream,
                      const _dbTechLayerCutSpacingTableDefSubRule& obj);
// User Code Begin general
// User Code End general
}  // namespace odb
   // Generator Code End 1