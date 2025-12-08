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
  uint hard_spacing : 1;
  uint block_rule : 1;
  uint spare_bits : 30;
};

class _dbTechNonDefaultRule : public _dbObject
{
 public:
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

  // PERSISTANT-MEMBERS
  _dbTechNonDefaultRuleFlags flags_;
  char* name_;
  dbVector<dbId<_dbTechLayerRule>> layer_rules_;
  dbVector<dbId<_dbTechVia>> vias_;
  dbVector<dbId<_dbTechSameNetRule>> samenet_rules_;
  dbMatrix<dbId<_dbTechSameNetRule>> samenet_matrix_;
  dbVector<dbId<_dbTechVia>> use_vias_;
  dbVector<dbId<_dbTechViaGenerateRule>> use_rules_;
  dbVector<dbId<_dbTechLayer>> cut_layers_;
  dbVector<int> min_cuts_;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechNonDefaultRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechNonDefaultRule& rule);

}  // namespace odb
