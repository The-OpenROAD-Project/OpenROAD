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
#include "dbMasterEdgeType.h"

#include "dbDatabase.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbMasterEdgeType>;

bool _dbMasterEdgeType::operator==(const _dbMasterEdgeType& rhs) const
{
  if (edge_dir_ != rhs.edge_dir_) {
    return false;
  }
  if (edge_type_ != rhs.edge_type_) {
    return false;
  }
  if (cell_row_ != rhs.cell_row_) {
    return false;
  }
  if (half_row_ != rhs.half_row_) {
    return false;
  }
  if (range_begin_ != rhs.range_begin_) {
    return false;
  }
  if (range_end_ != rhs.range_end_) {
    return false;
  }

  return true;
}

bool _dbMasterEdgeType::operator<(const _dbMasterEdgeType& rhs) const
{
  return true;
}

_dbMasterEdgeType::_dbMasterEdgeType(_dbDatabase* db)
{
  cell_row_ = -1;
  half_row_ = -1;
  range_begin_ = -1;
  range_end_ = -1;
}

dbIStream& operator>>(dbIStream& stream, _dbMasterEdgeType& obj)
{
  stream >> obj.edge_dir_;
  stream >> obj.edge_type_;
  stream >> obj.cell_row_;
  stream >> obj.half_row_;
  stream >> obj.range_begin_;
  stream >> obj.range_end_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMasterEdgeType& obj)
{
  stream << obj.edge_dir_;
  stream << obj.edge_type_;
  stream << obj.cell_row_;
  stream << obj.half_row_;
  stream << obj.range_begin_;
  stream << obj.range_end_;
  return stream;
}

void _dbMasterEdgeType::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["edge_type"].add(edge_type_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbMasterEdgeType - Methods
//
////////////////////////////////////////////////////////////////////

void dbMasterEdgeType::setEdgeType(const std::string& edge_type)
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;

  obj->edge_type_ = edge_type;
}

std::string dbMasterEdgeType::getEdgeType() const
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;
  return obj->edge_type_;
}

void dbMasterEdgeType::setCellRow(int cell_row)
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;

  obj->cell_row_ = cell_row;
}

int dbMasterEdgeType::getCellRow() const
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;
  return obj->cell_row_;
}

void dbMasterEdgeType::setHalfRow(int half_row)
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;

  obj->half_row_ = half_row;
}

int dbMasterEdgeType::getHalfRow() const
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;
  return obj->half_row_;
}

void dbMasterEdgeType::setRangeBegin(int range_begin)
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;

  obj->range_begin_ = range_begin;
}

int dbMasterEdgeType::getRangeBegin() const
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;
  return obj->range_begin_;
}

void dbMasterEdgeType::setRangeEnd(int range_end)
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;

  obj->range_end_ = range_end;
}

int dbMasterEdgeType::getRangeEnd() const
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;
  return obj->range_end_;
}

// User Code Begin dbMasterEdgeTypePublicMethods

void dbMasterEdgeType::setEdgeDir(dbMasterEdgeType::EdgeDir edge_dir)
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;

  obj->edge_dir_ = (uint) edge_dir;
}

dbMasterEdgeType::EdgeDir dbMasterEdgeType::getEdgeDir() const
{
  _dbMasterEdgeType* obj = (_dbMasterEdgeType*) this;
  return (dbMasterEdgeType::EdgeDir) obj->edge_dir_;
}

dbMasterEdgeType* dbMasterEdgeType::create(dbMaster* master)
{
  _dbMaster* _master = (_dbMaster*) master;
  auto edge_type = _master->edge_types_tbl_->create();
  return (dbMasterEdgeType*) edge_type;
}

void dbMasterEdgeType::destroy(dbMasterEdgeType* edge_type)
{
  _dbMaster* master = (_dbMaster*) edge_type->getImpl()->getOwner();
  dbProperty::destroyProperties(edge_type);
  master->edge_types_tbl_->destroy((_dbMasterEdgeType*) edge_type);
}
// User Code End dbMasterEdgeTypePublicMethods
}  // namespace odb
   // Generator Code End Cpp
