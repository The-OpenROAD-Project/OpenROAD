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
  DIFF_END
}

void _dbGDSStructure::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);

  DIFF_END
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db)
{
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db, const _dbGDSStructure& r)
{
  _name = r._name;
}

dbIStream& operator>>(dbIStream& stream, _dbGDSStructure& obj)
{
  stream >> obj._name;
  stream >> obj._elements;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSStructure& obj)
{
  stream << obj._name;
  stream << obj._elements;
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

void dbGDSStructure::setElements(std::vector<dbGDSElement> elements)
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;

  obj->_elements = elements;
}

std::vector<dbGDSElement> dbGDSStructure::getElements() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return obj->_elements;
}

// User Code Begin dbGDSStructurePublicMethods

dbGDSStructure* dbGDSStructure::create(dbGDSLib* lib_, const char* name_)
{
  if (lib_->findGDSStructure(name_)) {
    return nullptr;
  }

  _dbGDSLib* lib = (_dbGDSLib*) lib_;
  _dbDatabase* db = lib->getDatabase();
  _dbGDSStructure* structure = lib->_structure_tbl->create();
  structure->_name = strdup(name_);
  ZALLOCATED(structure->_name);

  // TODO: ID for structure

  lib->_structure_hash.insert(structure);
}

void dbGDSStructure::destroy(dbGDSStructure* structure)
{
  auto db = structure->getDb();
  _dbGDSStructure* str_impl = (_dbGDSStructure*) structure;
  _dbGDSLib* lib = (_dbGDSLib*) structure->getGDSLib();
  lib->_structure_hash.remove(str_impl);
  lib->_structure_tbl->destroy(str_impl);
}

// User Code End dbGDSStructurePublicMethods
}  // namespace odb
   // Generator Code End Cpp