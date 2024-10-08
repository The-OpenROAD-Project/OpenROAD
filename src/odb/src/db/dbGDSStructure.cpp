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
#include "dbGDSBoundary.h"
#include "dbGDSBox.h"
#include "dbGDSLib.h"
#include "dbGDSNode.h"
#include "dbGDSPath.h"
#include "dbGDSSRef.h"
#include "dbGDSText.h"
#include "dbHashTable.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbSet.h"
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
  if (*boundaries_ != *rhs.boundaries_) {
    return false;
  }
  if (*boxes_ != *rhs.boxes_) {
    return false;
  }
  if (*nodes_ != *rhs.nodes_) {
    return false;
  }
  if (*paths_ != *rhs.paths_) {
    return false;
  }
  if (*srefs_ != *rhs.srefs_) {
    return false;
  }
  if (*texts_ != *rhs.texts_) {
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
  DIFF_TABLE(boundaries_);
  DIFF_TABLE(boxes_);
  DIFF_TABLE(nodes_);
  DIFF_TABLE(paths_);
  DIFF_TABLE(srefs_);
  DIFF_TABLE(texts_);
  DIFF_END
}

void _dbGDSStructure::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_TABLE(boundaries_);
  DIFF_OUT_TABLE(boxes_);
  DIFF_OUT_TABLE(nodes_);
  DIFF_OUT_TABLE(paths_);
  DIFF_OUT_TABLE(srefs_);
  DIFF_OUT_TABLE(texts_);

  DIFF_END
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db)
{
  _name = nullptr;
  boundaries_ = new dbTable<_dbGDSBoundary>(
      db,
      this,
      (GetObjTbl_t) &_dbGDSStructure::getObjectTable,
      dbGDSBoundaryObj);
  boxes_ = new dbTable<_dbGDSBox>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSBoxObj);
  nodes_ = new dbTable<_dbGDSNode>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSNodeObj);
  paths_ = new dbTable<_dbGDSPath>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSPathObj);
  srefs_ = new dbTable<_dbGDSSRef>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSSRefObj);
  texts_ = new dbTable<_dbGDSText>(
      db, this, (GetObjTbl_t) &_dbGDSStructure::getObjectTable, dbGDSTextObj);
}

_dbGDSStructure::_dbGDSStructure(_dbDatabase* db, const _dbGDSStructure& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  boundaries_ = new dbTable<_dbGDSBoundary>(db, this, *r.boundaries_);
  boxes_ = new dbTable<_dbGDSBox>(db, this, *r.boxes_);
  nodes_ = new dbTable<_dbGDSNode>(db, this, *r.nodes_);
  paths_ = new dbTable<_dbGDSPath>(db, this, *r.paths_);
  srefs_ = new dbTable<_dbGDSSRef>(db, this, *r.srefs_);
  texts_ = new dbTable<_dbGDSText>(db, this, *r.texts_);
}

dbIStream& operator>>(dbIStream& stream, _dbGDSStructure& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> *obj.boundaries_;
  stream >> *obj.boxes_;
  stream >> *obj.nodes_;
  stream >> *obj.paths_;
  stream >> *obj.srefs_;
  stream >> *obj.texts_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGDSStructure& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << *obj.boundaries_;
  stream << *obj.boxes_;
  stream << *obj.nodes_;
  stream << *obj.paths_;
  stream << *obj.srefs_;
  stream << *obj.texts_;
  return stream;
}

dbObjectTable* _dbGDSStructure::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbGDSBoundaryObj:
      return boundaries_;
    case dbGDSBoxObj:
      return boxes_;
    case dbGDSNodeObj:
      return nodes_;
    case dbGDSPathObj:
      return paths_;
    case dbGDSSRefObj:
      return srefs_;
    case dbGDSTextObj:
      return texts_;
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}

_dbGDSStructure::~_dbGDSStructure()
{
  if (_name) {
    free((void*) _name);
  }
  delete boundaries_;
  delete boxes_;
  delete nodes_;
  delete paths_;
  delete srefs_;
  delete texts_;
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

dbSet<dbGDSBoundary> dbGDSStructure::getGDSBoundarys() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSBoundary>(obj, obj->boundaries_);
}

dbSet<dbGDSBox> dbGDSStructure::getGDSBoxs() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSBox>(obj, obj->boxes_);
}

dbSet<dbGDSNode> dbGDSStructure::getGDSNodes() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSNode>(obj, obj->nodes_);
}

dbSet<dbGDSPath> dbGDSStructure::getGDSPaths() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSPath>(obj, obj->paths_);
}

dbSet<dbGDSSRef> dbGDSStructure::getGDSSRefs() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSSRef>(obj, obj->srefs_);
}

dbSet<dbGDSText> dbGDSStructure::getGDSTexts() const
{
  _dbGDSStructure* obj = (_dbGDSStructure*) this;
  return dbSet<dbGDSText>(obj, obj->texts_);
}

// User Code Begin dbGDSStructurePublicMethods

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

// User Code End dbGDSStructurePublicMethods
}  // namespace odb
   // Generator Code End Cpp
