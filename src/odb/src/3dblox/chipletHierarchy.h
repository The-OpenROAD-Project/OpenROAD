// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace odb {

struct ChipletLevel
{
  int level;  // 0 = leaf nodes (no dependencies), higher numbers = more
              // dependent
  std::vector<dbChip*> chiplets;

  ChipletLevel(int l = 0) : level(l) {}
};

class ChipletHierarchyAnalyzer
{
 public:
  ChipletHierarchyAnalyzer(utl::Logger* logger);

  std::vector<ChipletLevel> analyzeHierarchy(odb::dbDatabase* db);
  std::vector<dbChip*> getChipletsAtLevel(int level);
  int getChipletLevel(dbChip* chiplet);
  const std::map<int, std::vector<dbChip*>>& getLevels() const
  {
    return levels_;
  }

 private:
  utl::Logger* logger_;
  std::map<int, std::vector<dbChip*>> levels_;
  std::map<dbChip*, int> chiplet_levels_;

  // Build dependency graph by analyzing chiplet instances
  std::map<dbChip*, std::set<dbChip*>> buildDependencyGraph(
      odb::dbDatabase* db);

  // Perform topological sort to determine levels
  void performTopologicalSort(
      const std::map<dbChip*, std::set<dbChip*>>& dependencies);

  // Check if a chiplet has any instances (is referenced by other chiplets)
  bool hasInstances(dbChip* chiplet, odb::dbDatabase* db);
};

}  // namespace odb
