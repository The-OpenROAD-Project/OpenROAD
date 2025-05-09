// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <boost/fusion/container.hpp>
#include <boost/optional/optional.hpp>
#include <boost/spirit/include/support_unused.hpp>
#include <string>

#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

class CellEdgeSpacingTableParser
{
 public:
  CellEdgeSpacingTableParser(dbTech* tech, lefinReader* lefin)
      : tech_(tech), lefin_(lefin)
  {
  }
  void parse(const std::string&);

 private:
  bool parseEntry(std::string);
  void createEntry();
  void setSpacing(double);
  void setBool(void (dbCellEdgeSpacing::*func)(bool));
  void setString(const std::string& val,
                 void (dbCellEdgeSpacing::*func)(const std::string&));
  dbTech* tech_;
  lefinReader* lefin_;
  dbCellEdgeSpacing* curr_entry_{nullptr};
};
}  // namespace odb
