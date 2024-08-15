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
#include "dbDatabase.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/odb.h"

namespace odb {

class _dbDatabase;
class _dbPolygon;
class _dbTechVia;
class _dbTechLayer;
class _dbVia;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbBoxFlagsBackwardCompatability
{
  dbBoxOwner::Value _owner_type : 4;
  uint _visited : 1;
  uint _mark : 1;
  uint _octilinear : 1;
  uint _is_tech_via : 1;
  uint _is_block_via : 1;
  uint _layer_id : 8;
  uint _via_id : 15;
};

struct _dbBoxFlagsWithoutMask
{
  dbBoxOwner::Value _owner_type : 4;
  uint _visited : 1;
  uint _octilinear : 1;
  uint _is_tech_via : 1;
  uint _is_block_via : 1;
  uint _layer_id : 9;
  uint _via_id : 15;
};

struct _dbBoxFlags
{
  dbBoxOwner::Value _owner_type : 4;
  uint _visited : 1;
  uint _octilinear : 1;
  uint _is_tech_via : 1;
  uint _is_block_via : 1;
  uint _layer_id : 9;
  uint _via_id : 13;
  uint _layer_mask : 2;
};

static_assert(sizeof(_dbBoxFlagsBackwardCompatability) == 4,
              "_dbBoxFlagsBackwardCompatability too large");
static_assert(sizeof(_dbBoxFlagsWithoutMask) == 4,
              "_dbBoxFlagsWithoutMask too large");
static_assert(sizeof(_dbBoxFlags) == 4, "_dbBoxFlags too large");

class _dbBox : public _dbObject
{
 public:
  enum Type
  {
    BLOCK_VIA,
    TECH_VIA,
    BOX
  };
  union dbBoxShape
  {
    Rect _rect;
    Oct _oct;
  };

  // PERSISTANT-MEMBERS
  _dbBoxFlags _flags;
  // Rect         _rect;
  dbBoxShape _shape = {Rect()};
  uint _owner;
  dbId<_dbBox> _next_box;
  int design_rule_width_;

  _dbBox(_dbDatabase*);
  _dbBox(_dbDatabase*, const _dbBox& b);

  bool operator==(const _dbBox& rhs) const;
  bool operator!=(const _dbBox& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbBox& rhs) const;
  int equal(const _dbBox& rhs) const;
  void differences(dbDiff& diff, const char* field, const _dbBox& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  bool isOct() const;

  _dbTechLayer* getTechLayer() const;
  _dbTechVia* getTechVia() const;
  _dbVia* getBlockVia() const;

  void getViaXY(int& x, int& y) const;

  Type getType() const
  {
    if (_flags._is_tech_via) {
      return TECH_VIA;
    }

    if (_flags._is_block_via) {
      return BLOCK_VIA;
    }

    return BOX;
  }

  void checkMask(uint mask);
};

inline _dbBox::_dbBox(_dbDatabase*)
{
  _flags._owner_type = dbBoxOwner::UNKNOWN;
  _flags._is_tech_via = 0;
  _flags._is_block_via = 0;
  _flags._layer_id = 0;
  _flags._layer_mask = 0;
  _flags._via_id = 0;
  _flags._visited = 0;
  _flags._octilinear = false;
  _owner = 0;
  design_rule_width_ = -1;
}

inline _dbBox::_dbBox(_dbDatabase*, const _dbBox& b)
    : _flags(b._flags),
      _owner(b._owner),
      _next_box(b._next_box),
      design_rule_width_(b.design_rule_width_)
{
  if (b.isOct()) {
    new (&_shape._oct) Oct();
    _shape._oct = b._shape._oct;
  } else {
    new (&_shape._rect) Rect();
    _shape._rect = b._shape._rect;
  }
}

inline dbOStream& operator<<(dbOStream& stream, const _dbBox& box)
{
  uint* bit_field = (uint*) &box._flags;
  stream << *bit_field;
  if (box.isOct()) {
    stream << box._shape._oct;
  } else {
    stream << box._shape._rect;
  }
  stream << box._owner;
  stream << box._next_box;
  stream << box.design_rule_width_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbBox& box)
{
  if (box.getDatabase()->isSchema(db_schema_dbbox_mask)) {
    uint* bit_field = (uint*) &box._flags;
    stream >> *bit_field;
  } else if (box.getDatabase()->isSchema(db_schema_box_layer_bits)) {
    _dbBoxFlagsWithoutMask old;
    uint* bit_field = (uint*) &old;
    stream >> *bit_field;
    box._flags._owner_type = old._owner_type;
    box._flags._visited = old._visited;
    box._flags._octilinear = old._octilinear;
    box._flags._is_tech_via = old._is_tech_via;
    box._flags._is_block_via = old._is_block_via;
    box._flags._layer_id = old._layer_id;
    box._flags._via_id = old._via_id;
    box._flags._layer_mask = 0;
  } else {
    _dbBoxFlagsBackwardCompatability old;
    uint* bit_field = (uint*) &old;
    stream >> *bit_field;
    box._flags._owner_type = old._owner_type;
    box._flags._visited = old._visited;
    box._flags._octilinear = old._octilinear;
    box._flags._is_tech_via = old._is_tech_via;
    box._flags._is_block_via = old._is_block_via;
    box._flags._layer_id = old._layer_id;
    box._flags._via_id = old._via_id;
    box._flags._layer_mask = 0;
  }

  if (box.isOct()) {
    new (&box._shape._oct) Oct();
    stream >> box._shape._oct;
  } else {
    stream >> box._shape._rect;
  }
  stream >> box._owner;
  stream >> box._next_box;
  stream >> box.design_rule_width_;
  return stream;
}

}  // namespace odb
