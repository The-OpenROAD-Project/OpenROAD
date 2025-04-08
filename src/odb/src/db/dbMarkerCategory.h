// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/odb.h"
// User Code Begin Includes
#include <boost/property_tree/json_parser.hpp>
#include <fstream>
#include <set>
#include <string>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbMarker;
template <class T>
class dbTable;
class _dbMarkerCategory;
// User Code Begin Classes
class _dbBlock;
// User Code End Classes

class _dbMarkerCategory : public _dbObject
{
 public:
  _dbMarkerCategory(_dbDatabase*);

  ~_dbMarkerCategory();

  bool operator==(const _dbMarkerCategory& rhs) const;
  bool operator!=(const _dbMarkerCategory& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbMarkerCategory& rhs) const;
  dbObjectTable* getObjectTable(dbObjectType type);
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  using PropertyTree = boost::property_tree::ptree;
  bool isTopCategory() const;
  bool hasMaxMarkerLimit() const;

  _dbBlock* getBlock() const;
  void populatePTree(PropertyTree& tree) const;
  void fromPTree(const PropertyTree& tree);
  static void writeJSON(std::ofstream& report,
                        const std::set<_dbMarkerCategory*>& categories);
  void writeTR(std::ofstream& report) const;
  // User Code End Methods

  char* _name;
  std::string description_;
  std::string source_;
  int max_markers_;
  dbTable<_dbMarker>* marker_tbl_;
  dbTable<_dbMarkerCategory>* categories_tbl_;
  dbHashTable<_dbMarkerCategory> categories_hash_;
  dbId<_dbMarkerCategory> _next_entry;
};
dbIStream& operator>>(dbIStream& stream, _dbMarkerCategory& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMarkerCategory& obj);
}  // namespace odb
   // Generator Code End Header
