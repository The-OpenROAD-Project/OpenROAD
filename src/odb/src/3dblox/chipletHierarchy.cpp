// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "chipletHierarchy.h"

#include <algorithm>
#include <queue>

#include "utl/Logger.h"

namespace odb {

ChipletHierarchyAnalyzer::ChipletHierarchyAnalyzer(utl::Logger* logger)
    : logger_(logger)
{
}

std::vector<ChipletLevel> ChipletHierarchyAnalyzer::analyzeHierarchy(
    odb::dbDatabase* db)
{
  levels_.clear();
  chiplet_levels_.clear();

  // Build dependency graph
  auto dependencies = buildDependencyGraph(db);

  // Perform topological sort to determine levels
  performTopologicalSort(dependencies);

  // Convert to vector format
  std::vector<ChipletLevel> result;
  for (const auto& [level, chiplets] : levels_) {
    ChipletLevel chiplet_level(level);
    chiplet_level.chiplets = chiplets;
    result.push_back(chiplet_level);
  }

  // Sort by level number
  std::sort(result.begin(),
            result.end(),
            [](const ChipletLevel& a, const ChipletLevel& b) {
              return a.level < b.level;
            });

  return result;
}

std::vector<dbChip*> ChipletHierarchyAnalyzer::getChipletsAtLevel(int level)
{
  auto it = levels_.find(level);
  if (it != levels_.end()) {
    return it->second;
  }
  return {};
}

int ChipletHierarchyAnalyzer::getChipletLevel(dbChip* chiplet)
{
  auto it = chiplet_levels_.find(chiplet);
  if (it != chiplet_levels_.end()) {
    return it->second;
  }
  return -1;
}

std::map<dbChip*, std::set<dbChip*>>
ChipletHierarchyAnalyzer::buildDependencyGraph(odb::dbDatabase* db)
{
  std::map<dbChip*, std::set<dbChip*>> dependencies;

  // Find dependencies by analyzing chiplet instances
  for (auto chiplet : db->getChips()) {
    dependencies[chiplet] = std::set<dbChip*>();
    for (auto inst : chiplet->getChipInsts()) {
      auto master_chip = inst->getMasterChip();
      if (master_chip != nullptr) {
        // The chiplet depends on its master chip
        dependencies[chiplet].insert(master_chip);
      }
    }
  }

  return dependencies;
}

void ChipletHierarchyAnalyzer::performTopologicalSort(
    const std::map<dbChip*, std::set<dbChip*>>& dependencies)
{
  // Calculate in-degrees (number of dependencies)
  std::map<dbChip*, int> in_degree;
  for (const auto& [chiplet, deps] : dependencies) {
    in_degree[chiplet] = deps.size();
  }

  // Find all chiplets with no dependencies (level 0)
  std::queue<dbChip*> queue;
  for (const auto& [chiplet, degree] : in_degree) {
    if (degree == 0) {
      queue.push(chiplet);
    }
  }

  int current_level = 0;
  int remaining_at_level = queue.size();

  while (!queue.empty()) {
    auto chiplet = queue.front();
    queue.pop();

    // Add to current level
    levels_[current_level].push_back(chiplet);
    chiplet_levels_[chiplet] = current_level;

    remaining_at_level--;

    // If we've processed all chiplets at this level, move to next level
    if (remaining_at_level == 0) {
      current_level++;
      remaining_at_level = queue.size();
    }

    // Find all chiplets that depend on this one and reduce their in-degree
    for (const auto& [dependent_chiplet, deps] : dependencies) {
      if (deps.find(chiplet) != deps.end()) {
        in_degree[dependent_chiplet]--;
        if (in_degree[dependent_chiplet] == 0) {
          queue.push(dependent_chiplet);
        }
      }
    }
  }

  // Check for circular dependencies
  for (const auto& [chiplet, degree] : in_degree) {
    if (degree > 0) {
      logger_->warn(utl::ODB,
                    536,
                    "Circular dependency detected for chiplet: {}",
                    chiplet->getName());
    }
  }
}

bool ChipletHierarchyAnalyzer::hasInstances(dbChip* chiplet,
                                            odb::dbDatabase* db)
{
  for (auto other_chiplet : db->getChips()) {
    for (auto inst : other_chiplet->getChipInsts()) {
      if (inst->getMasterChip() == chiplet) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace odb
