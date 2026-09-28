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
class dbModule;
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
  kNameGroup = 4,  // Group synthesized from instance-name paths
};

// A flat design (DEF-only: defin creates no dbModules) reaches the browser
// with one top module holding every instance, so there is no tree to walk.
// When the instance names still carry their pre-flattening path, the tree is
// reconstructed from those names instead -- see getReport().
//
// Depth cap: the cardinality control.  Chosen over a group budget or a
// minimum-size fold because it is the one a reader can predict from a name
// without knowing the implementation.  It does not bound group count on its
// own -- branching at the capped depth still can -- so kMaxNameGroups
// backstops it.
inline constexpr int kDefaultNameGroupDepth = 4;
inline constexpr size_t kMaxNameGroups = 5000;

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

  // True when the tree was synthesized from instance-name paths rather than
  // walked from dbModules.  The two modes never mix: in name-group mode every
  // colorable node is a kNameGroup whose odb_id is a synthetic group id, and
  // instances resolve to one through inst_group rather than through
  // dbInst::getModule().  That keeps one key space on the wire, so the color
  // map, the client protocol and the tile lookup are unchanged -- only the
  // renderer's source of the key differs.
  bool name_grouped = false;

  // dbInst::getId() -> synthetic group id; 0 where the instance sits at the
  // top level directly or has no group.  Empty unless name_grouped.  Indexed
  // rather than mapped because the tile renderer reads it per instance per
  // tile, where a hash lookup (let alone a name split) would not pay.
  std::vector<uint32_t> inst_group;

  // True when kMaxNameGroups stopped the tree growing; deeper instances were
  // folded into their nearest existing ancestor.  The UI says so.
  bool name_groups_capped = false;
};

class HierarchyReport
{
 public:
  HierarchyReport(odb::dbBlock* block,
                  sta::dbSta* sta,
                  int name_group_depth = kDefaultNameGroupDepth);

  // Walks the dbModule tree.  When the design has none -- a flat design, where
  // the top module has no children -- falls back to synthesizing groups from
  // instance-name paths.  If no name carries the hierarchy delimiter that
  // synthesis yields nothing and the result is exactly the flat tree, so the
  // fallback needs no detection heuristic of its own.
  HierarchyResult getReport() const;

 private:
  // Returns false when the design carries no recoverable paths, leaving
  // `result` untouched for the caller to fill the flat way.
  bool addNameGroups(odb::dbModule* top, HierarchyResult& result) const;

  odb::dbBlock* block_;
  sta::dbSta* sta_;
  int name_group_depth_;
};

// JSON serialization (shared by handleModuleHierarchy and saveReport).
boost::json::object serializeHierarchyResult(const HierarchyResult& result);

// Compute the module color map for the default UI state (depth-1+ modules
// collapsed, all visible).  Returns odb module id → RGBA color, ready for
// use in tile rendering.
std::map<uint32_t, Color> computeDefaultModuleColors(
    const HierarchyResult& result);

}  // namespace web
