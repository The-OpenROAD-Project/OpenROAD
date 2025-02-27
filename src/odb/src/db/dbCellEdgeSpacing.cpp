///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2025, The Regents of the University of California
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
#include "dbCellEdgeSpacing.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbCellEdgeSpacing>;

bool _dbCellEdgeSpacing::operator==(const _dbCellEdgeSpacing& rhs) const
{
  if (flags_.except_abutted_ != rhs.flags_.except_abutted_) {
    return false;
  }
  if (flags_.except_non_filler_in_between_
      != rhs.flags_.except_non_filler_in_between_) {
    return false;
  }
  if (flags_.optional_ != rhs.flags_.optional_) {
    return false;
  }
  if (flags_.soft_ != rhs.flags_.soft_) {
    return false;
  }
  if (flags_.exact_ != rhs.flags_.exact_) {
    return false;
  }
  if (first_edge_type_ != rhs.first_edge_type_) {
    return false;
  }
  if (second_edge_type_ != rhs.second_edge_type_) {
    return false;
  }
  if (spacing != rhs.spacing) {
    return false;
  }

  return true;
}

bool _dbCellEdgeSpacing::operator<(const _dbCellEdgeSpacing& rhs) const
{
  return true;
}

_dbCellEdgeSpacing::_dbCellEdgeSpacing(_dbDatabase* db)
{
  flags_ = {};
  spacing = -1;
}

dbIStream& operator>>(dbIStream& stream, _dbCellEdgeSpacing& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.first_edge_type_;
  stream >> obj.second_edge_type_;
  stream >> obj.spacing;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbCellEdgeSpacing& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.first_edge_type_;
  stream << obj.second_edge_type_;
  stream << obj.spacing;
  return stream;
}

void _dbCellEdgeSpacing::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["first_edge_type"].add(first_edge_type_);
  info.children_["second_edge_type"].add(second_edge_type_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbCellEdgeSpacing - Methods
//
////////////////////////////////////////////////////////////////////

void dbCellEdgeSpacing::setFirstEdgeType(const std::string& first_edge_type)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->first_edge_type_ = first_edge_type;
}

std::string dbCellEdgeSpacing::getFirstEdgeType() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;
  return obj->first_edge_type_;
}

void dbCellEdgeSpacing::setSecondEdgeType(const std::string& second_edge_type)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->second_edge_type_ = second_edge_type;
}

std::string dbCellEdgeSpacing::getSecondEdgeType() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;
  return obj->second_edge_type_;
}

void dbCellEdgeSpacing::setSpacing(int spacing)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->spacing = spacing;
}

int dbCellEdgeSpacing::getSpacing() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;
  return obj->spacing;
}

void dbCellEdgeSpacing::setExceptAbutted(bool except_abutted)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->flags_.except_abutted_ = except_abutted;
}

bool dbCellEdgeSpacing::isExceptAbutted() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  return obj->flags_.except_abutted_;
}

void dbCellEdgeSpacing::setExceptNonFillerInBetween(
    bool except_non_filler_in_between)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->flags_.except_non_filler_in_between_ = except_non_filler_in_between;
}

bool dbCellEdgeSpacing::isExceptNonFillerInBetween() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  return obj->flags_.except_non_filler_in_between_;
}

void dbCellEdgeSpacing::setOptional(bool optional)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->flags_.optional_ = optional;
}

bool dbCellEdgeSpacing::isOptional() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  return obj->flags_.optional_;
}

void dbCellEdgeSpacing::setSoft(bool soft)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->flags_.soft_ = soft;
}

bool dbCellEdgeSpacing::isSoft() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  return obj->flags_.soft_;
}

void dbCellEdgeSpacing::setExact(bool exact)
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  obj->flags_.exact_ = exact;
}

bool dbCellEdgeSpacing::isExact() const
{
  _dbCellEdgeSpacing* obj = (_dbCellEdgeSpacing*) this;

  return obj->flags_.exact_;
}

// User Code Begin dbCellEdgeSpacingPublicMethods
dbCellEdgeSpacing* dbCellEdgeSpacing::create(dbTech* tech)
{
  _dbTech* _tech = (_dbTech*) tech;
  auto entry = _tech->cell_edge_spacing_tbl_->create();
  return (dbCellEdgeSpacing*) entry;
}

void dbCellEdgeSpacing::destroy(dbCellEdgeSpacing* entry)
{
  _dbTech* _tech = (_dbTech*) entry->getImpl()->getOwner();
  dbProperty::destroyProperties(entry);
  _tech->cell_edge_spacing_tbl_->destroy((_dbCellEdgeSpacing*) entry);
}
// User Code End dbCellEdgeSpacingPublicMethods
}  // namespace odb
   // Generator Code End Cpp
