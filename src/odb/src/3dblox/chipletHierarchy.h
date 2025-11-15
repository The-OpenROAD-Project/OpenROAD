// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "odb/db.h"

namespace odb {

struct ChipletNode
{
  dbChip* chip;
  std::vector<ChipletNode*> children;
  std::vector<ChipletNode*> parents;
  explicit ChipletNode(dbChip* c = nullptr) : chip(c) {}
};

class ChipletHierarchy
{
 public:
  ChipletHierarchy() = default;
  void buildHierarchy(const std::vector<dbChip*>& all_chips);
  ChipletNode* findNodeForChip(odb::dbChip* chip) const;

 private:
  ChipletNode* addChip(odb::dbChip* chip);
  void addDependency(odb::dbChip* parent, odb::dbChip* child);
  std::vector<ChipletNode*> getRoots() const;
  std::unordered_map<odb::dbChip*, std::unique_ptr<ChipletNode>> nodes_;
};

}  // namespace odb
