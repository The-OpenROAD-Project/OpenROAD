// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "chipletHierarchy.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace odb {

ChipletNode* ChipletHierarchy::addChip(dbChip* chip)
{
  if (!chip) {
    return nullptr;
  }

  auto it = nodes_.find(chip);
  if (it == nodes_.end()) {
    auto node = std::make_unique<ChipletNode>(chip);
    ChipletNode* ptr = node.get();
    nodes_[chip] = std::move(node);
    return ptr;
  }
  return it->second.get();
}

void ChipletHierarchy::addDependency(dbChip* parent, dbChip* child)
{
  if (!parent || !child) {
    return;
  }

  ChipletNode* parent_node = addChip(parent);
  ChipletNode* child_node = addChip(child);

  parent_node->children.push_back(child_node);
  child_node->parents.push_back(parent_node);
}

void ChipletHierarchy::buildHierarchy(const std::vector<dbChip*>& all_chips)
{
  nodes_.clear();

  for (odb::dbChip* parent : all_chips) {
    ChipletNode* parent_node = addChip(parent);
    if (!parent_node) {
      continue;
    }

    // Iterate instances to find dependencies
    for (dbChipInst* inst : parent->getChipInsts()) {
      // The instance master points to the child chiplets
      odb::dbChip* child = inst->getMasterChip();
      if (child) {
        addDependency(parent, child);
      }
    }
  }
}

std::vector<ChipletNode*> ChipletHierarchy::getRoots() const
{
  std::vector<ChipletNode*> roots;
  for (auto& [chip, node] : nodes_) {
    if (node->parents.empty()) {
      roots.push_back(node.get());
    }
  }
  return roots;
}

ChipletNode* ChipletHierarchy::findNodeForChip(dbChip* chip) const
{
  if (!chip) {
    return nullptr;
  }

  auto it = nodes_.find(chip);
  if (it == nodes_.end()) {
    return nullptr;
  }

  return it->second.get();
}

}  // namespace odb
