// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbMatrix.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbTech;
class _dbBlock;
class _dbTechLayerRule;
class _dbTechVia;
class _dbTechLayer;
class _dbTechViaGenerateRule;
class _dbTechSameNetRule;
class _dbDatabase;
class dbIStream;
class dbOStream;

struct _dbTechNonDefaultRuleFlags
{
  uint _hard_spacing : 1;
  uint _block_rule : 1;
  uint _spare_bits : 30;
};

class _dbTechNonDefaultRule : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbTechNonDefaultRuleFlags _flags;
  char* _name;
  dbVector<dbId<_dbTechLayerRule>> _layer_rules;
  dbVector<dbId<_dbTechVia>> _vias;
  dbVector<dbId<_dbTechSameNetRule>> _samenet_rules;
  dbMatrix<dbId<_dbTechSameNetRule>> _samenet_matrix;
  dbVector<dbId<_dbTechVia>> _use_vias;
  dbVector<dbId<_dbTechViaGenerateRule>> _use_rules;
  dbVector<dbId<_dbTechLayer>> _cut_layers;
  dbVector<int> _min_cuts;

  _dbTechNonDefaultRule(_dbDatabase*);
  _dbTechNonDefaultRule(_dbDatabase*, const _dbTechNonDefaultRule& r);
  ~_dbTechNonDefaultRule();

  _dbTech* getTech();
  _dbBlock* getBlock();

  bool operator==(const _dbTechNonDefaultRule& rhs) const;
  bool operator!=(const _dbTechNonDefaultRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechNonDefaultRule& rhs) const;
  void collectMemInfo(MemInfo& info);
};

dbOStream& operator<<(dbOStream& stream, const _dbTechNonDefaultRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechNonDefaultRule& rule);

}  // namespace odb
