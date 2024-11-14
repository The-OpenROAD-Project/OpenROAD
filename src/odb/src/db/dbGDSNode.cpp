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

// Generator Code Begin Cpp
#include "dbGDSNode.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSNode>;

bool _dbGDSNode::operator==(const _dbGDSNode& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }
  if (_datatype != rhs._datatype) {
    return false;
  }

  return true;
}

bool _dbGDSNode::operator<(const _dbGDSNode& rhs) const
{
  return true;
}

void _dbGDSNode::differences(dbDiff& diff,
                             const char* field,
                             const _dbGDSNode& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_FIELD(_datatype);
  DIFF_END
}

void _dbGDSNode::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_FIELD(_datatype);

  DIFF_END
}

_dbGDSNode::_dbGDSNode(_dbDatabase* db)
{
  _layer = 0;
  _datatype = 0;
}

_dbGDSNode::_dbGDSNode(_dbDatabase* db, const _dbGDSNode& r)
{
  _layer = r._layer;
  _datatype = r._datatype;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSNode& obj)
{
  stream >> obj._layer;
  stream >> obj._datatype;
  stream >> obj._xy;
  stream >> obj._propattr;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSNode& obj)
{
  stream << obj._layer;
  stream << obj._datatype;
  stream << obj._xy;
  stream << obj._propattr;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbGDSNode - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSNode::setLayer(int16_t layer)
{
  _dbGDSNode* obj = (_dbGDSNode*) this;

  obj->_layer = layer;
}

int16_t dbGDSNode::getLayer() const
{
  _dbGDSNode* obj = (_dbGDSNode*) this;
  return obj->_layer;
}

void dbGDSNode::setDatatype(int16_t datatype)
{
  _dbGDSNode* obj = (_dbGDSNode*) this;

  obj->_datatype = datatype;
}

int16_t dbGDSNode::getDatatype() const
{
  _dbGDSNode* obj = (_dbGDSNode*) this;
  return obj->_datatype;
}

void dbGDSNode::setXy(const std::vector<Point>& xy)
{
  _dbGDSNode* obj = (_dbGDSNode*) this;

  obj->_xy = xy;
}

void dbGDSNode::getXy(std::vector<Point>& tbl) const
{
  _dbGDSNode* obj = (_dbGDSNode*) this;
  tbl = obj->_xy;
}

// User Code Begin dbGDSNodePublicMethods
std::vector<std::pair<std::int16_t, std::string>>& dbGDSNode::getPropattr()
{
  auto* obj = (_dbGDSNode*) this;
  return obj->_propattr;
}

const std::vector<Point>& dbGDSNode::getXY()
{
  auto obj = (_dbGDSNode*) this;
  return obj->_xy;
}

dbGDSNode* dbGDSNode::create(dbGDSStructure* structure)
{
  auto* obj = (_dbGDSStructure*) structure;
  return (dbGDSNode*) obj->nodes_->create();
}

void dbGDSNode::destroy(dbGDSNode* node)
{
  auto* obj = (_dbGDSNode*) node;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->nodes_->destroy(obj);
}
// User Code End dbGDSNodePublicMethods
}  // namespace odb
   // Generator Code End Cpp
