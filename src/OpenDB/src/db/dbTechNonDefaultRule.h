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
#include "dbId.h"
#include "dbMatrix.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"

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
class dbDiff;

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
  _dbTechNonDefaultRuleFlags              _flags;
  char*                                   _name;
  dbVector<dbId<_dbTechLayerRule> >       _layer_rules;
  dbVector<dbId<_dbTechVia> >             _vias;
  dbVector<dbId<_dbTechSameNetRule> >     _samenet_rules;
  dbMatrix<dbId<_dbTechSameNetRule> >     _samenet_matrix;
  dbVector<dbId<_dbTechVia> >             _use_vias;
  dbVector<dbId<_dbTechViaGenerateRule> > _use_rules;
  dbVector<dbId<_dbTechLayer> >           _cut_layers;
  dbVector<int>                           _min_cuts;

  _dbTechNonDefaultRule(_dbDatabase*);
  _dbTechNonDefaultRule(_dbDatabase*, const _dbTechNonDefaultRule& r);
  ~_dbTechNonDefaultRule();

  _dbTech*  getTech();
  _dbBlock* getBlock();

  bool operator==(const _dbTechNonDefaultRule& rhs) const;
  bool operator!=(const _dbTechNonDefaultRule& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbTechNonDefaultRule& rhs) const;
  void differences(dbDiff&                      diff,
                   const char*                  field,
                   const _dbTechNonDefaultRule& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

dbOStream& operator<<(dbOStream& stream, const _dbTechNonDefaultRule& rule);
dbIStream& operator>>(dbIStream& stream, _dbTechNonDefaultRule& rule);

}  // namespace odb
