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

namespace utl {
class Logger;
}

namespace odb {

template <class T, uint32_t page_size = 128>
class dbTable;
class _dbDatabase;
class _dbProperty;
class dbObjectTable;
template <typename T, uint32_t page_size = 128>
class dbHashTable;
template <typename T>
class dbIntHashTable;
template <typename T>
class dbMatrix;
template <class T, const uint32_t P, const uint32_t S>
class dbPagedVector;

constexpr uint32_t kAllocBit = 0x80000000;
constexpr uint32_t kOffsetMask = ~kAllocBit;

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

  template <class T, const uint32_t P, const uint32_t S>
  void add(const dbPagedVector<T, P, S>& vec)
  {
    cnt += 1;
    size += vec.size() * sizeof(T);
  }

  template <class T, uint32_t page_size>
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
    MemInfo& key_info = children["key"];
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
    MemInfo& key_info = children["key"];
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

  std::map<const char*, MemInfo> children;
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
  uint32_t getOID() const;
  utl::Logger* getLogger() const;
  bool isValid() const { return oid_ & kAllocBit; };

 private:
  uint32_t oid_;

  template <class T, uint32_t page_size>
  friend class dbTable;
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
                uint32_t size);
  virtual ~dbObjectTable() = default;

  dbId<_dbProperty> getPropList(uint32_t oid)
  {
    return prop_list_.getAttr(oid);
  }

  void setPropList(uint32_t oid, const dbId<_dbProperty>& propList)
  {
    prop_list_.setAttr(oid, propList);
  }

  virtual dbObject* getObject(uint32_t id, ...) = 0;
  virtual bool validObject(uint32_t id, ...) = 0;

  dbObjectTable* getObjectTable(dbObjectType type)
  {
    return (owner_->*getObjectTable_)(type);
  }

  // NON-PERSISTANT DATA
  _dbDatabase* db_;
  dbObject* owner_;
  dbObjectType type_;
  uint32_t obj_size_;
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
  uint32_t next_;
  uint32_t prev_;
};

///////////////////////////////////////////////////////////////
/// dbObjectPage definition
///////////////////////////////////////////////////////////////
class dbObjectPage
{
 public:
  bool valid_page() const { return alloc_cnt_ != 0; }

  // NON-PERSISTANT DATA
  dbObjectTable* table_;
  uint32_t page_addr_;
  uint32_t alloc_cnt_;
};

///////////////////////////////////////////////////////////////
/// dbObjectTable implementation
///////////////////////////////////////////////////////////////
inline dbObjectTable::dbObjectTable(_dbDatabase* db,
                                    dbObject* owner,
                                    dbObjectTable* (dbObject::*m)(dbObjectType),
                                    dbObjectType type,
                                    uint32_t size)
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

inline uint32_t _dbObject::getOID() const
{
  dbObjectPage* page = getObjectPage();
  uint32_t offset = (oid_ & kOffsetMask);
  return page->page_addr_ | (offset / page->table_->obj_size_);
}

inline dbObjectTable* _dbObject::getTable() const
{
  dbObjectPage* page = getObjectPage();
  return page->table_;
}

inline _dbDatabase* _dbObject::getDatabase() const
{
  dbObjectPage* page = getObjectPage();
  return page->table_->db_;
}

inline dbObject* _dbObject::getOwner() const
{
  dbObjectPage* page = getObjectPage();
  return page->table_->owner_;
}

inline dbObjectType _dbObject::getType() const
{
  dbObjectPage* page = getObjectPage();
  return page->table_->type_;
}

inline dbObjectPage* _dbObject::getObjectPage() const
{
  uint32_t offset = (oid_ & kOffsetMask);
  char* base = (char*) this - offset;
  dbObjectPage* page = (dbObjectPage*) (base - sizeof(dbObjectPage));
  return page;
}

}  // namespace odb
