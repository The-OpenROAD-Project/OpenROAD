///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#pragma once

#include "dbCore.h"
#include "dbHashTable.hpp"
#include "dbMatrix.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"

namespace odb {

template <class T>
class dbTable;

class _dbProperty;
class dbPropertyItr;
class _dbNameCache;
class _dbTechLayer;
class _dbTechLayerRule;
class _dbTechVia;
class _dbTechNonDefaultRule;
class _dbTechSameNetRule;
class _dbTechLayerAntennaRule;
class _dbTechViaRule;
class _dbTechViaLayerRule;
class _dbTechViaGenerateRule;
class _dbBox;
class _dbDatabase;
class dbTechLayerItr;
class dbBoxItr;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbTechFlags
{
  dbOnOffType::Value _namecase : 1;
  dbOnOffType::Value _haswireext : 1;
  dbOnOffType::Value _nowireext : 1;
  dbOnOffType::Value _hasclmeas : 1;
  dbClMeasureType::Value _clmeas : 2;
  dbOnOffType::Value _hasminspobs : 1;
  dbOnOffType::Value _minspobs : 1;
  dbOnOffType::Value _hasminsppin : 1;
  dbOnOffType::Value _minsppin : 1;
  uint _spare_bits : 22;
};

class _dbTech : public _dbObject
{
 private:
  double _version;
  char _version_buf[35];  // Tmp for outputting string
 public:
  // PERSISTANT-MEMBERS
  int _via_cnt;
  int _layer_cnt;
  int _rlayer_cnt;
  int _lef_units;
  int _dbu_per_micron;
  int _mfgrid;
  _dbTechFlags _flags;
  dbId<_dbTechLayer> _bottom;
  dbId<_dbTechLayer> _top;
  dbId<_dbTechNonDefaultRule> _non_default_rules;
  dbVector<dbId<_dbTechSameNetRule>> _samenet_rules;
  dbMatrix<dbId<_dbTechSameNetRule>> _samenet_matrix;
  dbHashTable<_dbTechVia> _via_hash;

  // NON-PERSISTANT-STREAMED-MEMBERS
  dbTable<_dbTechLayer>* _layer_tbl;
  dbTable<_dbTechVia>* _via_tbl;
  dbTable<_dbTechNonDefaultRule>* _non_default_rule_tbl;
  dbTable<_dbTechLayerRule>* _layer_rule_tbl;
  dbTable<_dbBox>* _box_tbl;
  dbTable<_dbTechSameNetRule>* _samenet_rule_tbl;
  dbTable<_dbTechLayerAntennaRule>* _antenna_rule_tbl;
  dbTable<_dbTechViaRule>* _via_rule_tbl;
  dbTable<_dbTechViaLayerRule>* _via_layer_rule_tbl;
  dbTable<_dbTechViaGenerateRule>* _via_generate_rule_tbl;
  dbTable<_dbProperty>* _prop_tbl;
  _dbNameCache* _name_cache;

  // NON-PERSISTANT-NON-STREAMED-MEMBERS
  dbTechLayerItr* _layer_itr;
  dbBoxItr* _box_itr;
  dbPropertyItr* _prop_itr;

  double _getLefVersion() const;
  const char* _getLefVersionStr() const;
  void _setLefVersion(double inver);

  _dbTech(_dbDatabase* db);
  _dbTech(_dbDatabase* db, const _dbTech& t);
  ~_dbTech();

  bool operator==(const _dbTech& rhs) const;
  bool operator!=(const _dbTech& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbTech& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  dbObjectTable* getObjectTable(dbObjectType type);
};

dbOStream& operator<<(dbOStream& stream, const _dbTech& tech);
dbIStream& operator>>(dbIStream& stream, _dbTech& tech);

}  // namespace odb
