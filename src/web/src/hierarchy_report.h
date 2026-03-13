// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace odb {
class dbBlock;
}  // namespace odb

namespace sta {
class dbSta;
}  // namespace sta

namespace web {

// Node types in the hierarchy tree
enum class HierarchyNodeKind
{
  MODULE = 0,      // Module (default)
  LEAF_GROUP = 1,  // "Leaf instances" folder
  TYPE_GROUP = 2,  // Instance type sub-group (e.g. "Standard cell", "Macro")
  INSTANCE = 3,    // Individual instance row (only for macros)
};

struct HierarchyNode
{
  int id = 0;
  int parent_id = -1;  // -1 for root (top module)
  std::string inst_name;
  std::string module_name;  // master name for TYPE_GROUP/INSTANCE nodes
  int insts = 0;            // hierarchical stdcell count
  int macros = 0;           // hierarchical macro count
  int modules = 0;          // hierarchical sub-module count
  double area = 0.0;        // hierarchical area (μm²)
  int local_insts = 0;      // direct stdcell count
  int local_macros = 0;     // direct macro count
  int local_modules = 0;    // direct child module count
  HierarchyNodeKind node_kind = HierarchyNodeKind::MODULE;
  unsigned int odb_id = 0;  // dbModule::getId() for MODULE nodes
};

struct HierarchyResult
{
  std::vector<HierarchyNode> nodes;
};

class HierarchyReport
{
 public:
  HierarchyReport(odb::dbBlock* block, sta::dbSta* sta);

  HierarchyResult getReport() const;

 private:
  odb::dbBlock* block_;
  sta::dbSta* sta_;
};

}  // namespace web
