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
#include "dbGDSStructure.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbGDSLib.h"
#include "dbHashTable.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
namespace odb {
template class dbTable<_dbGDSStructure>;

bool _dbGDSStructure::operator==(const _dbGDSStructure& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }

  return true;
}

bool _dbGDSStructure::operator<(const _dbGDSStructure& rhs) const
{
  return true;
}

void _dbGDSStructure::differences(dbDiff& diff,
                                  const char* field,
                                  const _dbGDSStructure& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_END
}

void _dbGDSStructure::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);

  DIFF_END
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db)
{
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db, const _dbGDSStructure& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSStructure& obj)
{
  stream >> obj._name;
  stream >> obj._elements;
  stream >> obj._next_entry;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSStructure& obj)
{
  stream << obj._name;
  stream << obj._elements;
  stream << obj._next_entry;
  return stream;
}

_dbGDSStructure::~_dbGDSStructure()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbGDSStructure - Methods
//
////////////////////////////////////////////////////////////////////

char* dbGDSStructure::getName() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return obj->_name;
}

// User Code Begin dbGDSStructurePublicMethods

dbIStream& operator>>(dbIStream& stream, _dbGDSElement* obj)
{
  stream >> *obj;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSElement* obj)
{
  stream << *obj;
  return stream;
}

dbGDSStructure* dbGDSStructure::create(dbGDSLib* lib_, const char* name_)
{
  if (lib_->findGDSStructure(name_)) {
    return nullptr;
  }

  _dbGDSLib* lib = (_dbGDSLib*) lib_;
  _dbGDSStructure* structure = lib->_structure_tbl->create();
  structure->_name = strdup(name_);
  ZALLOCATED(structure->_name);

  // TODO: ID for structure

  lib->_structure_hash.insert(structure);
  return (dbGDSStructure*) structure;
}

void dbGDSStructure::destroy(dbGDSStructure* structure)
{
  _dbGDSStructure* str_impl = (_dbGDSStructure*) structure;
  _dbGDSLib* lib = (_dbGDSLib*) structure->getGDSLib();
  lib->_structure_hash.remove(str_impl);
  lib->_structure_tbl->destroy(str_impl);
}

dbGDSLib* dbGDSStructure::getGDSLib()
{
  return (dbGDSLib*) getImpl()->getOwner();
}

void dbGDSStructure::removeElement(int index)
{
  auto& elements = ((_dbGDSStructure*) this)->_elements;
  elements.erase(elements.begin() + index);
}

void dbGDSStructure::addElement(dbGDSElement* element)
{
  ((_dbGDSStructure*) this)->_elements.push_back((_dbGDSElement*) element);
}

dbGDSElement* dbGDSStructure::getElement(int index)
{
  return (dbGDSElement*) ((_dbGDSStructure*) this)->_elements[index];
}

dbGDSElement* dbGDSStructure::operator[](int index)
{
  return getElement(index);
}

int dbGDSStructure::getNumElements()
{
  return ((_dbGDSStructure*) this)->_elements.size();
}

// User Code End dbGDSStructurePublicMethods
}  // namespace odb
   // Generator Code End Cpp
