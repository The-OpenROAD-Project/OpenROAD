// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <string_view>

#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {
class lefMacroClassTypeParser
{
 public:
  static bool parse(std::string_view, dbMaster*);
};
class lefMacroEdgeTypeParser
{
 public:
  lefMacroEdgeTypeParser(dbMaster* master, lefinReader* lefin)
      : master_(master), lefin_(lefin)
  {
  }
  void parse(const std::string&);
  bool parseSubRule(std::string);
  void setRange(boost::fusion::vector<double, double>& params);

 private:
  dbMaster* master_;
  lefinReader* lefin_;
  dbMasterEdgeType* edge_type_{nullptr};
};
}  // namespace odb
