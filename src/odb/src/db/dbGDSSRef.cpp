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
#include "dbGDSSRef.h"

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
// User Code Begin Includes
#include "dbGDSLib.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGDSSRef>;

bool _dbGDSSRef::operator==(const _dbGDSSRef& rhs) const
{
  if (_origin != rhs._origin) {
    return false;
  }
  if (_structure != rhs._structure) {
    return false;
  }

  return true;
}

bool _dbGDSSRef::operator<(const _dbGDSSRef& rhs) const
{
  return true;
}

_dbGDSSRef::_dbGDSSRef(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbGDSSRef& obj)
{
  stream >> obj._origin;
  stream >> obj._propattr;
  stream >> obj._transform;
  stream >> obj._structure;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSSRef& obj)
{
  stream << obj._origin;
  stream << obj._propattr;
  stream << obj._transform;
  stream << obj._structure;
  return stream;
}

void _dbGDSSRef::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["propattr"].add(_propattr);
  for (auto& [i, s] : _propattr) {
    info.children_["propattr"].add(s);
  }
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbGDSSRef - Methods
//
////////////////////////////////////////////////////////////////////

void dbGDSSRef::setOrigin(Point origin)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_origin = origin;
}

Point dbGDSSRef::getOrigin() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_origin;
}

void dbGDSSRef::setTransform(dbGDSSTrans transform)
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;

  obj->_transform = transform;
}

dbGDSSTrans dbGDSSRef::getTransform() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  return obj->_transform;
}

// User Code Begin dbGDSSRefPublicMethods

dbGDSStructure* dbGDSSRef::getStructure() const
{
  _dbGDSSRef* obj = (_dbGDSSRef*) this;
  if (obj->_structure == 0) {
    return nullptr;
  }
  _dbGDSStructure* parent = (_dbGDSStructure*) obj->getOwner();
  _dbGDSLib* lib = (_dbGDSLib*) parent->getOwner();
  return (dbGDSStructure*) lib->_gdsstructure_tbl->getPtr(obj->_structure);
}

std::vector<std::pair<std::int16_t, std::string>>& dbGDSSRef::getPropattr()
{
  auto* obj = (_dbGDSSRef*) this;
  return obj->_propattr;
}

dbGDSSRef* dbGDSSRef::create(dbGDSStructure* parent, dbGDSStructure* child)
{
  auto* obj = (_dbGDSStructure*) parent;
  _dbGDSSRef* sref = obj->srefs_->create();
  sref->_structure = child->getImpl()->getOID();
  return (dbGDSSRef*) sref;
}

void dbGDSSRef::destroy(dbGDSSRef* sRef)
{
  auto* obj = (_dbGDSSRef*) sRef;
  auto* structure = (_dbGDSStructure*) obj->getOwner();
  structure->srefs_->destroy(obj);
}

// User Code End dbGDSSRefPublicMethods
}  // namespace odb
   // Generator Code End Cpp
