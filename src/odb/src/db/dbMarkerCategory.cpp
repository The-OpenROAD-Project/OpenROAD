///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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
#include "dbMarkerCategory.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbMarker.h"
#include "dbMarkerGroup.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
// User Code Begin Includes
#include "dbHashTable.hpp"
// User Code End Includes
namespace odb {
template class dbTable<_dbMarkerCategory>;

bool _dbMarkerCategory::operator==(const _dbMarkerCategory& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (description_ != rhs.description_) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (*marker_tbl_ != *rhs.marker_tbl_) {
    return false;
  }

  return true;
}

bool _dbMarkerCategory::operator<(const _dbMarkerCategory& rhs) const
{
  return true;
}

void _dbMarkerCategory::differences(dbDiff& diff,
                                    const char* field,
                                    const _dbMarkerCategory& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(description_);
  DIFF_FIELD_NO_DEEP(_next_entry);
  DIFF_TABLE(marker_tbl_);
  DIFF_END
}

void _dbMarkerCategory::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(description_);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);
  DIFF_OUT_TABLE(marker_tbl_);

  DIFF_END
}

_dbMarkerCategory::_dbMarkerCategory(_dbDatabase* db)
{
  _name = nullptr;
  marker_tbl_ = new dbTable<_dbMarker>(
      db, this, (GetObjTbl_t) &_dbMarkerCategory::getObjectTable, dbMarkerObj);
}

_dbMarkerCategory::_dbMarkerCategory(_dbDatabase* db,
                                     const _dbMarkerCategory& r)
{
  _name = r._name;
  description_ = r.description_;
  _next_entry = r._next_entry;
  marker_tbl_ = new dbTable<_dbMarker>(db, this, *r.marker_tbl_);
}

dbIStream& operator>>(dbIStream& stream, _dbMarkerCategory& obj)
{
  stream >> obj._name;
  stream >> obj.description_;
  stream >> obj._next_entry;
  stream >> *obj.marker_tbl_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbMarkerCategory& obj)
{
  stream << obj._name;
  stream << obj.description_;
  stream << obj._next_entry;
  stream << *obj.marker_tbl_;
  return stream;
}

dbObjectTable* _dbMarkerCategory::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbMarkerObj:
      return marker_tbl_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbMarkerCategory::~_dbMarkerCategory()
{
  if (_name) {
    free((void*) _name);
  }
  delete marker_tbl_;
}

////////////////////////////////////////////////////////////////////
//
// dbMarkerCategory - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbMarkerCategory::getName() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return obj->_name;
}

void dbMarkerCategory::setDescription(const std::string& description)
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;

  obj->description_ = description;
}

std::string dbMarkerCategory::getDescription() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return obj->description_;
}

dbSet<dbMarker> dbMarkerCategory::getMarkers() const
{
  _dbMarkerCategory* obj = (_dbMarkerCategory*) this;
  return dbSet<dbMarker>(obj, obj->marker_tbl_);
}

// User Code Begin dbMarkerCategoryPublicMethods

dbMarkerGroup* dbMarkerCategory::getGroup() const
{
  _dbMarkerCategory* _category = (_dbMarkerCategory*) this;
  return (dbMarkerGroup*) _category->getOwner();
}

dbMarkerCategory* dbMarkerCategory::create(dbMarkerGroup* group,
                                           const char* name)
{
  _dbMarkerGroup* _group = (_dbMarkerGroup*) group;

  if (_group->categories_hash_.hasMember(name)) {
    return nullptr;
  }

  _dbMarkerCategory* category = _group->categories_tbl_->create();

  category->_name = strdup(name);
  ZALLOCATED(category->_name);

  _group->categories_hash_.insert(category);

  return (dbMarkerCategory*) category;
}

void dbMarkerCategory::destroy(dbMarkerCategory* category)
{
  _dbMarkerCategory* _category = (_dbMarkerCategory*) category;
  _dbMarkerGroup* group = (_dbMarkerGroup*) _category->getOwner();
  group->categories_hash_.remove(_category);
  group->categories_tbl_->destroy(_category);
}

// User Code End dbMarkerCategoryPublicMethods
}  // namespace odb
// Generator Code End Cpp