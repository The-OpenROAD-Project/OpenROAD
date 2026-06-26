// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbCorner.h"

#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <algorithm>

#include "dbBlock.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbCorner>;

bool _dbCorner::operator==(const _dbCorner& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (name_ != rhs.name_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbCorner::operator<(const _dbCorner& rhs) const
{
  if (name_ >= rhs.name_) {
    return false;
  }

  return true;
}

_dbCorner::_dbCorner(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbCorner& obj)
{
  stream >> obj.name_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbCorner& obj)
{
  stream << obj.name_;
  return stream;
}

void _dbCorner::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);
}

////////////////////////////////////////////////////////////////////
//
// dbCorner - Methods
//
////////////////////////////////////////////////////////////////////

const std::string& dbCorner::getName() const
{
  _dbCorner* obj = (_dbCorner*) this;
  return obj->name_;
}

// User Code Begin dbCornerPublicMethods

dbCorner* dbCorner::create(dbBlock* block_, const std::string& corner_name)
{
  _dbBlock* block = (_dbBlock*) block_;
  utl::Logger* logger = block->getImpl()->getLogger();

  if (corner_name.empty()) {
    logger->error(
        utl::ODB, 9, "Could not add corner. Empty name is not allowed.");
  }

  if (block_->findCorner(corner_name)) {
    logger->error(
        utl::ODB,
        5,
        "Could not add corner {}. A corner with that name already exists.",
        corner_name);
  }

  _dbCorner* corner = block->corner_tbl_->create();
  corner->name_ = corner_name;
  block->corners_.push_back(corner->getId());

  return (dbCorner*) corner;
}

void dbCorner::destroy(dbCorner* corner_)
{
  _dbCorner* corner = (_dbCorner*) corner_;
  _dbBlock* block = (_dbBlock*) corner->getOwner();

  // Note that std::ranges::find requires the search value
  // to be the same type as the range's elements.
  const dbId<_dbCorner> corner_id = corner->getId();
  auto corner_position = std::ranges::find(block->corners_, corner_id);
  block->corners_.erase(corner_position);

  dbProperty::destroyProperties(corner_);
  block->corner_tbl_->destroy(corner);
}

// User Code End dbCornerPublicMethods
}  // namespace odb
// Generator Code End Cpp