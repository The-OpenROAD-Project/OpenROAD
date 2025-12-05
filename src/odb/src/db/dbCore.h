// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/container/flat_map.hpp"
#include "dbAttrTable.h"
#include "odb/dbId.h"
#include "odb/dbObject.h"
#include "odb/odb.h"

namespace utl {
class Logger;
}

namespace odb {

template <class T, uint page_size = 128>
class dbTable;
class _dbDatabase;
class _dbProperty;
class dbObjectTable;
template <typename T, uint page_size = 128>
class dbHashTable;
template <typename T>
class dbIntHashTable;
template <typename T>
class dbMatrix;
template <class T, const uint P, const uint S>
class dbPagedVector;

constexpr uint DB_ALLOC_BIT = 0x80000000;
constexpr uint DB_OFFSET_MASK = ~DB_ALLOC_BIT;

using GetObjTbl_t = dbObjectTable* (dbObject::*) (dbObjectType);

struct MemInfo
{
  void add(const char* str)
  {
    if (str) {
      cnt++;
      size += strlen(str);
    }
  }

  void add(const std::string& str) { add(str.c_str()); }

  template <typename T>
  void add(const std::vector<T>& vec)
  {
    cnt += 1;
    size += vec.size() * sizeof(T);
  }

  template <class T, const uint P, const uint S>
  void add(const dbPagedVector<T, P, S>& vec)
  {
    cnt += 1;
    size += vec.size() * sizeof(T);
  }

  template <class T, uint page_size>
  void add(const dbHashTable<T, page_size>& table)
  {
    cnt += 1;
    size += table.hash_tbl_.size() * sizeof(dbId<T>);
  }

  template <class T>
  void add(const dbIntHashTable<T>& table)
  {
    cnt += 1;
    size += table.hash_tbl_.size() * sizeof(dbId<T>);
  }

  template <typename T>
  void add(const dbMatrix<T>& matrix)
  {
    cnt += 1;
    size += matrix.numElems() * sizeof(T);
  }

  template <typename Key, typename T>
  void add(const std::map<Key, T>& map)
  {
    cnt += 1;
    size += map.size() * (sizeof(Key) + sizeof(T));
  }

  template <typename T>
  void add(const std::map<std::string, T>& map)
  {
    cnt += 1;
    size += map.size() * (sizeof(std::string) + sizeof(T));
    MemInfo& key_info = children_["key"];
    for (const auto& [key, value] : map) {
      key_info.cnt += 1;
      key_info.size += key.size();
    }
  }

  template <typename Key, typename T>
  void add(const std::unordered_map<Key, T>& map)
  {
    cnt += 1;
    size += map.size() * (sizeof(Key) + sizeof(T));
  }

  template <typename Key, typename T>
  void add(const boost::container::flat_map<Key, T>& map)
  {
    cnt += 1;
    size += map.size() * (sizeof(Key) + sizeof(T));
  }

  template <typename T>
  void add(const std::unordered_map<std::string, T>& map)
  {
    cnt += 1;
    size += map.size() * (sizeof(std::string) + sizeof(T));
    MemInfo& key_info = children_["key"];
    for (const auto& [key, value] : map) {
      key_info.cnt += 1;
      key_info.size += key.size();
    }
  }

  template <typename T>
  void add(const std::set<T>& set)
  {
    cnt += 1;
    size += set.size() * sizeof(T);
  }

  std::map<const char*, MemInfo> children_;
  int cnt{0};
  uint64_t size{0};
};

///////////////////////////////////////////////////////////////
/// _dbObject definition
///////////////////////////////////////////////////////////////
class _dbObject : public dbObject
{
 public:
  _dbDatabase* getDatabase() const;
  dbObjectTable* getTable() const;
  dbObjectPage* getObjectPage() const;
  dbObject* getOwner() const;
  dbObjectType getType() const;
  uint getOID() const;
  utl::Logger* getLogger() const;
  bool isValid() const { return oid_ & DB_ALLOC_BIT; };

 private:
  uint oid_;

  template <class T, uint page_size>
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
  dbObjectTable(_dbDatabase* db,
                dbObject* owner,
                dbObjectTable* (dbObject::*m)(dbObjectType),
                dbObjectType type,
                uint size);
  virtual ~dbObjectTable() = default;

  dbId<_dbProperty> getPropList(uint oid) { return prop_list_.getAttr(oid); }

  void setPropList(uint oid, const dbId<_dbProperty>& propList)
  {
    prop_list_.setAttr(oid, propList);
  }

  virtual dbObject* getObject(uint id, ...) = 0;
  virtual bool validObject(uint id, ...) = 0;

  dbObjectTable* getObjectTable(dbObjectType type)
  {
    return (owner_->*getObjectTable_)(type);
  }

  // NON-PERSISTANT DATA
  _dbDatabase* db_;
  dbObject* owner_;
  dbObjectType type_;
  uint obj_size_;
  dbObjectTable* (dbObject::*getObjectTable_)(dbObjectType type);

  // PERSISTANT DATA
  dbAttrTable<dbId<_dbProperty>> prop_list_;
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
  bool valid_page() const { return _alloccnt != 0; }

  // NON-PERSISTANT DATA
  dbObjectTable* _table;
  uint _page_addr;
  uint _alloccnt;
};

///////////////////////////////////////////////////////////////
/// dbObjectTable implementation
///////////////////////////////////////////////////////////////
inline dbObjectTable::dbObjectTable(_dbDatabase* db,
                                    dbObject* owner,
                                    dbObjectTable* (dbObject::*m)(dbObjectType),
                                    dbObjectType type,
                                    uint size)
    : db_(db), owner_(owner), type_(type), obj_size_(size), getObjectTable_(m)
{
  // Objects must be greater than 16-bytes
  assert(size >= sizeof(_dbFreeObject));
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
  dbObjectPage* page = getObjectPage();
  uint offset = (oid_ & DB_OFFSET_MASK);
  return page->_page_addr | offset / page->_table->obj_size_;
}

inline dbObjectTable* _dbObject::getTable() const
{
  dbObjectPage* page = getObjectPage();
  return page->_table;
}

inline _dbDatabase* _dbObject::getDatabase() const
{
  dbObjectPage* page = getObjectPage();
  return page->_table->db_;
}

inline dbObject* _dbObject::getOwner() const
{
  dbObjectPage* page = getObjectPage();
  return page->_table->owner_;
}

inline dbObjectType _dbObject::getType() const
{
  dbObjectPage* page = getObjectPage();
  return page->_table->type_;
}

inline dbObjectPage* _dbObject::getObjectPage() const
{
  uint offset = (oid_ & DB_OFFSET_MASK);
  char* base = (char*) this - offset;
  dbObjectPage* page = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page;
}

}  // namespace odb
