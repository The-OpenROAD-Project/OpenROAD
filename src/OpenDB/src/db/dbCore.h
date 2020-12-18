///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#pragma once

///
/// This file contains the core objects that everything is
/// built from.
///
/// The core objects are:
///
///  dbId - included
///  dbObjectTable
///  dbObjectPage
///  dbTablePage
///

#include "dbAttrTable.h"
#include "dbId.h"
#include "dbObject.h"
#include "odb.h"

namespace odb {

class _dbDatabase;
class _dbProperty;
class dbObjectTable;

#define DB_ALLOC_BIT 0x80000000
#define DB_OFFSET_MASK (~DB_ALLOC_BIT)

typedef dbObjectTable* (dbObject::*GetObjTbl_t)(dbObjectType);

///////////////////////////////////////////////////////////////
/// _dbObject definition
///////////////////////////////////////////////////////////////
class _dbObject : public dbObject
{
 private:
  uint _oid;

 public:
  _dbDatabase*   getDatabase() const;
  dbObjectTable* getTable() const;
  dbObjectPage*  getObjectPage() const;
  dbObject*      getOwner() const;
  dbObjectType   getType() const;
  uint           getOID() const;

  template <class T>
  friend class dbTable;
  template <class T>
  friend class dbArrayTable;
};

///////////////////////////////////////////////////////////////
/// dbObjectTable definition
///////////////////////////////////////////////////////////////
class dbObjectTable
{
 public:
  // NON-PERSISTANT DATA
  _dbDatabase* _db;
  dbObject*    _owner;
  dbObjectType _type;
  uint         _obj_size;
  dbObjectTable* (dbObject::*_getObjectTable)(dbObjectType type);

  // PERSISTANT DATA
  dbAttrTable<dbId<_dbProperty> > _prop_list;

  virtual ~dbObjectTable(){};
  dbObjectTable();
  dbObjectTable(_dbDatabase* db,
                dbObject*    owner,
                dbObjectTable* (dbObject::*m)(dbObjectType),
                dbObjectType type,
                uint         size);

  dbId<_dbProperty> getPropList(uint oid) { return _prop_list.getAttr(oid); }

  void setPropList(uint oid, dbId<_dbProperty> propList)
  {
    _prop_list.setAttr(oid, propList);
  }

  virtual dbObject* getObject(uint id, ...) = 0;

  dbObjectTable* getObjectTable(dbObjectType type)
  {
    return (_owner->*_getObjectTable)(type);
  }
};

///////////////////////////////////////////////////////////////
/// _dbFreeObject definition - free-list object
///////////////////////////////////////////////////////////////

class _dbFreeObject : public _dbObject
{
 public:
  uint _next;
  uint _prev;
};

///////////////////////////////////////////////////////////////
/// dbObjectPage definition
///////////////////////////////////////////////////////////////
class dbObjectPage
{
 public:
  // NON-PERSISTANT DATA
  dbObjectTable* _table;
  uint           _page_addr;
  uint           _alloccnt;

  bool valid_page() const { return _alloccnt != 0; }
};

///////////////////////////////////////////////////////////////
/// dbObjectTable implementation
///////////////////////////////////////////////////////////////
inline dbObjectTable::dbObjectTable()
{
  _db    = NULL;
  _owner = NULL;
}

inline dbObjectTable::dbObjectTable(_dbDatabase* db,
                                    dbObject*    owner,
                                    dbObjectTable* (dbObject::*m)(dbObjectType),
                                    dbObjectType type,
                                    uint         size)
{
  _db             = db;
  _owner          = owner;
  _getObjectTable = m;
  _type           = type;

  // Objects must be greater than 16-bytes
  assert(size >= sizeof(_dbFreeObject));
  _obj_size = size;
}

///////////////////////////////////////////////////////////////
/// _dbObject inlines
///////////////////////////////////////////////////////////////

inline _dbObject* dbObject::getImpl()
{
  return (_dbObject*) this;
}

inline const _dbObject* dbObject::getImpl() const
{
  return (_dbObject*) this;
}

inline uint _dbObject::getOID() const
{
  uint          offset = (_oid & DB_OFFSET_MASK);
  char*         base   = (char*) this - offset;
  dbObjectPage* page   = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page->_page_addr | offset / page->_table->_obj_size;
}

inline dbObjectTable* _dbObject::getTable() const
{
  uint          offset = (_oid & DB_OFFSET_MASK);
  char*         base   = (char*) this - offset;
  dbObjectPage* page   = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page->_table;
}

inline _dbDatabase* _dbObject::getDatabase() const
{
  uint          offset = (_oid & DB_OFFSET_MASK);
  char*         base   = (char*) this - offset;
  dbObjectPage* page   = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page->_table->_db;
}

inline dbObject* _dbObject::getOwner() const
{
  uint          offset = (_oid & DB_OFFSET_MASK);
  char*         base   = (char*) this - offset;
  dbObjectPage* page   = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page->_table->_owner;
}

inline dbObjectType _dbObject::getType() const
{
  uint          offset = (_oid & DB_OFFSET_MASK);
  char*         base   = (char*) this - offset;
  dbObjectPage* page   = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page->_table->_type;
}

inline dbObjectPage* _dbObject::getObjectPage() const
{
  uint          offset = (_oid & DB_OFFSET_MASK);
  char*         base   = (char*) this - offset;
  dbObjectPage* page   = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page;
}

}  // namespace odb
