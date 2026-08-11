// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "boost/json/object.hpp"
#include "color.h"

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
  kModule = 0,     // Module (default)
  kLeafGroup = 1,  // "Leaf instances" folder
  kTypeGroup = 2,  // Instance type sub-group (e.g. "Standard cell", "Macro")
  kInstance = 3,   // Individual instance row (only for macros)
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
  HierarchyNodeKind node_kind = HierarchyNodeKind::kModule;
  unsigned int odb_id = 0;  // dbModule::getId() for MODULE nodes
  Color color;              // set by getReport() for MODULEs
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

// JSON serialization (shared by handleModuleHierarchy and saveReport).
boost::json::object serializeHierarchyResult(const HierarchyResult& result);

// One node of a "color by owner" tree, as the effective-color rule sees it.
// Both the module hierarchy and the cluster (dbGroup) tree reduce to this.
struct OwnerColorNode
{
  int parent = -1;         // index into the vector, -1 for a root
  uint32_t odb_id = 0;     // key of the resulting color map
  Color color;             // the node's own palette color
  bool collapsed = false;  // its subtree is folded into it in the UI
  bool has_color = false;  // false for structural rows that carry no color
};

// Which nodes have at least one child, indexed by node id.  Both reports need
// it to decide the default collapse state, and both rely on the same invariant
// their DFS gives them: a node's id IS its position.
std::vector<char> hasChildrenByIndex(size_t node_count,
                                     const std::vector<int>& parent_ids);

// Resolve each node's effective color and return odb id → color.
//
// The rule the panels implement: a collapsed node paints its whole subtree, and
// with collapsed nodes nested the highest one wins.  Callers must pass nodes in
// an order where a parent precedes its children (both reports emit DFS order),
// which is what lets this resolve in one pass.
//
// Shared so that the two reports, and therefore `save_image`/`web_save_report`
// and the live viewer, cannot disagree about what a collapsed row means.
std::map<uint32_t, Color> computeEffectiveOwnerColors(
    const std::vector<OwnerColorNode>& nodes);

// Compute the module color map for the default UI state (depth-1+ modules
// collapsed, all visible).  Returns odb module id → RGBA color, ready for
// use in tile rendering.
std::map<uint32_t, Color> computeDefaultModuleColors(
    const HierarchyResult& result);

}  // namespace web
