// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbCorner.h"

#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbVector.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbBlock.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbCorner>;
// User Code Begin Static
// User Code End Static

bool _dbCorner::operator==(const _dbCorner& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (name_ != rhs.name_) {
    return false;
  }

  // User Code Begin ==
  // User Code End ==
  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbCorner::operator<(const _dbCorner& rhs) const
{
  if (name_ >= rhs.name_) {
    return false;
  }

  // User Code Begin <
  // User Code End <
  return true;
}

_dbCorner::_dbCorner(_dbDatabase* db)
{
  // User Code Begin Constructor
  // User Code End Constructor
}

dbIStream& operator>>(dbIStream& stream, _dbCorner& obj)
{
  stream >> obj.name_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbCorner& obj)
{
  stream << obj.name_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

void _dbCorner::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["name"].add(name_);

  // User Code Begin collectMemInfo
  // User Code End collectMemInfo
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

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
  _dbCorner* corner = block->corner_tbl_->create();
  corner->name_ = corner_name;
  return (dbCorner*) corner;
}

void dbCorner::destroy(dbCorner* corner_)
{
  _dbCorner* corner = (_dbCorner*) corner_;
  _dbBlock* block = (_dbBlock*) corner->getOwner();
  block->corner_tbl_->destroy(corner);
}

// User Code End dbCornerPublicMethods
}  // namespace odb
// Generator Code End Cpp