///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbHashTable.h"
#include "odb/odb.h"
// User Code Begin Includes
#include <boost/property_tree/json_parser.hpp>
#include <fstream>
#include <set>
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
class _dbMarker;
template <class T>
class dbTable;
class _dbMarkerCategory;

class _dbMarkerCategory : public _dbObject
{
 public:
  _dbMarkerCategory(_dbDatabase*, const _dbMarkerCategory& r);
  _dbMarkerCategory(_dbDatabase*);

  ~_dbMarkerCategory();

  bool operator==(const _dbMarkerCategory& rhs) const;
  bool operator!=(const _dbMarkerCategory& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbMarkerCategory& rhs) const;
  void differences(dbDiff& diff,
                   const char* field,
                   const _dbMarkerCategory& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  dbObjectTable* getObjectTable(dbObjectType type);
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