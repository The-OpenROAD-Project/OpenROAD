// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "hierarchy_report.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/json/array.hpp"
#include "db_sta/dbSta.hh"
#include "module_color_palette.h"
#include "odb/db.h"

namespace web {

HierarchyReport::HierarchyReport(odb::dbBlock* block,
                                 sta::dbSta* sta,
                                 int name_group_depth)
    : block_(block), sta_(sta), name_group_depth_(name_group_depth)
{
}

struct ModuleStats
{
  int insts = 0;
  int macros = 0;
  int modules = 0;
  int64_t area_dbu2 = 0;
};

// Instance types that the GUI excludes from the hierarchy by default.
static bool isPhysicalType(sta::dbSta::InstType type)
{
  using IT = sta::dbSta::InstType;
  switch (type) {
    case IT::ENDCAP:
    case IT::FILL:
    case IT::TAPCELL:
    case IT::STD_PHYSICAL:
    case IT::BUMP:
    case IT::COVER:
    case IT::ANTENNA:
      return true;
    default:
      return false;
  }
}

// Map InstType enum to the display name used in the GUI.
static const char* typeBucketName(sta::dbSta::InstType type)
{
  using IT = sta::dbSta::InstType;
  switch (type) {
    case IT::BLOCK:
      return "Macro";
    case IT::PAD:
      return "Pad";
    case IT::PAD_INPUT:
      return "Input pad";
    case IT::PAD_OUTPUT:
      return "Output pad";
    case IT::PAD_INOUT:
      return "Input/output pad";
    case IT::PAD_POWER:
      return "Power pad";
    case IT::PAD_SPACER:
      return "Pad spacer";
    case IT::PAD_AREAIO:
      return "Area IO";
    case IT::TIE:
      return "Tie cell";
    case IT::LEF_OTHER:
      return "Other";
    case IT::STD_CELL:
      return "Standard cell";
    case IT::STD_BUF:
      return "Buffer";
    case IT::STD_INV:
      return "Inverter";
    case IT::STD_BUF_CLK_TREE:
      return "Clock buffer";
    case IT::STD_INV_CLK_TREE:
      return "Clock inverter";
    case IT::STD_BUF_TIMING_REPAIR:
      return "Timing Repair Buffer";
    case IT::STD_INV_TIMING_REPAIR:
      return "Timing Repair Inverter";
    case IT::STD_CLOCK_GATE:
      return "Clock gate cell";
    case IT::STD_LEVEL_SHIFT:
      return "Level shifter cell";
    case IT::STD_SEQUENTIAL:
      return "Sequential cell";
    case IT::STD_COMBINATIONAL:
      return "Multi-Input combinational cell";
    case IT::STD_OTHER:
      return "Other";
    default:
      return "Standard cell";
  }
}

// Info accumulated per instance-type group within a module.
struct TypeBucket
{
  int count = 0;
  int64_t area_dbu2 = 0;
  bool is_macro = false;
  // Individual macro instances (only populated for BLOCK type)
  std::vector<odb::dbInst*> macro_insts;
};

// Emit "Leaf instances" folder with type sub-groups for a module or name
// group.  Templated on the instance range so a dbModule's dbSet and a name
// group's vector both feed it without copying either into the other's shape.
template <typename InstRange>
static void emitLeafNodes(const InstRange& insts,
                          sta::dbSta* sta,
                          int parent_id,
                          int& next_id,
                          std::vector<HierarchyNode>& nodes)
{
  // Group instances by type
  std::map<std::string, TypeBucket> buckets;
  for (odb::dbInst* inst : insts) {
    sta::dbSta::InstType inst_type = sta->getInstanceType(inst);
    if (isPhysicalType(inst_type)) {
      continue;
    }
    const char* type_name = typeBucketName(inst_type);
    auto& bucket = buckets[type_name];
    bucket.count++;
    bucket.area_dbu2 += inst->getBBox()->getBox().area();
    if (inst_type == sta::dbSta::InstType::BLOCK) {
      bucket.is_macro = true;
      bucket.macro_insts.push_back(inst);
    }
  }

  if (buckets.empty()) {
    return;
  }

  // Emit "Leaf instances" parent node
  const int leaf_id = next_id++;
  nodes.emplace_back();
  HierarchyNode& leaf = nodes[leaf_id];
  leaf.id = leaf_id;
  leaf.parent_id = parent_id;
  leaf.inst_name = "Leaf instances";
  leaf.node_kind = HierarchyNodeKind::kLeafGroup;

  // Aggregate totals for the leaf group
  for (const auto& [name, bucket] : buckets) {
    leaf.area += static_cast<double>(bucket.area_dbu2);
    if (bucket.is_macro) {
      leaf.macros += bucket.count;
    } else {
      leaf.insts += bucket.count;
    }
  }
  leaf.local_insts = leaf.insts;
  leaf.local_macros = leaf.macros;

  // Emit type sub-group nodes
  for (const auto& [type_name, bucket] : buckets) {
    const int type_id = next_id++;
    nodes.emplace_back();
    HierarchyNode& type_node = nodes[type_id];
    type_node.id = type_id;
    type_node.parent_id = leaf_id;
    type_node.inst_name = type_name;
    type_node.node_kind = HierarchyNodeKind::kTypeGroup;
    type_node.area = static_cast<double>(bucket.area_dbu2);
    if (bucket.is_macro) {
      type_node.macros = bucket.count;
      type_node.local_macros = bucket.count;
    } else {
      type_node.insts = bucket.count;
      type_node.local_insts = bucket.count;
    }

    // For macros, emit individual instance rows
    if (bucket.is_macro) {
      for (odb::dbInst* inst : bucket.macro_insts) {
        const int inst_id = next_id++;
        nodes.emplace_back();
        HierarchyNode& inst_node = nodes[inst_id];
        inst_node.id = inst_id;
        inst_node.parent_id = type_id;
        inst_node.inst_name = inst->getConstName();
        inst_node.module_name = inst->getMaster()->getConstName();
        inst_node.node_kind = HierarchyNodeKind::kInstance;
        inst_node.macros = 1;
        inst_node.local_macros = 1;
        inst_node.area = static_cast<double>(inst->getBBox()->getBox().area());
      }
    }
  }
}

// Nodes that carry a color key the tile renderer looks up.  The two kinds
// never appear in one report -- see HierarchyResult::name_grouped.
static bool isColorable(HierarchyNodeKind kind)
{
  return kind == HierarchyNodeKind::kModule
         || kind == HierarchyNodeKind::kNameGroup;
}

// Recursive DFS: adds a node for the module, recurses into children,
// then writes back hierarchical totals. Returns hierarchical stats.
static ModuleStats addModule(odb::dbModule* module,
                             sta::dbSta* sta,
                             int parent_id,
                             const char* inst_name,
                             int& next_id,
                             std::vector<HierarchyNode>& nodes)
{
  const int my_id = next_id++;
  nodes.emplace_back();
  nodes[my_id].id = my_id;
  nodes[my_id].parent_id = parent_id;
  nodes[my_id].inst_name = inst_name;
  nodes[my_id].module_name = module->getName();
  nodes[my_id].odb_id = module->getId();

  // Count local instances and area (skip physical cell types to match GUI)
  ModuleStats local;
  for (odb::dbInst* inst : module->getInsts()) {
    sta::dbSta::InstType inst_type = sta->getInstanceType(inst);
    if (isPhysicalType(inst_type)) {
      continue;
    }
    local.area_dbu2 += inst->getBBox()->getBox().area();
    if (inst->isBlock()) {
      local.macros++;
    } else {
      local.insts++;
    }
  }
  local.modules = module->getModInstCount();

  nodes[my_id].local_insts = local.insts;
  nodes[my_id].local_macros = local.macros;
  nodes[my_id].local_modules = local.modules;

  // Hierarchical = local + sum of children
  ModuleStats hier = local;

  for (odb::dbModInst* child_mi : module->getChildren()) {
    odb::dbModule* child_mod = child_mi->getMaster();
    ModuleStats child_stats
        = addModule(child_mod, sta, my_id, child_mi->getName(), next_id, nodes);
    hier.insts += child_stats.insts;
    hier.macros += child_stats.macros;
    hier.modules += child_stats.modules;
    hier.area_dbu2 += child_stats.area_dbu2;
  }

  // Emit "Leaf instances" folder with type sub-groups
  emitLeafNodes(module->getInsts(), sta, my_id, next_id, nodes);

  // Write hierarchical totals (area stored as DBU², converted later)
  nodes[my_id].insts = hier.insts;
  nodes[my_id].macros = hier.macros;
  nodes[my_id].modules = hier.modules;
  nodes[my_id].area = static_cast<double>(hier.area_dbu2);

  return hier;
}

// ─── Name-group synthesis (flat designs) ───────────────────────────────

namespace {

// One level of the trie built from instance-name paths.
struct NameGroupNode
{
  std::string segment;  // this level's path segment
  int parent = -1;      // index into the group vector
  std::vector<int> children;
  std::unordered_map<std::string, int> child_index;
  std::vector<odb::dbInst*> insts;  // instances local to this group
};

}  // namespace

// Mirror of addModule() for a synthesized group: same node shape, same
// local-vs-hierarchical accounting, same leaf folder, so the client cannot
// tell the two apart except by node_kind.
static ModuleStats addNameGroup(std::vector<NameGroupNode>& groups,
                                const int g,
                                sta::dbSta* sta,
                                const int parent_id,
                                const std::string& inst_name,
                                int& next_id,
                                uint32_t& next_group_id,
                                std::vector<uint32_t>& inst_group,
                                std::vector<HierarchyNode>& nodes)
{
  const int my_id = next_id++;
  const uint32_t my_group = next_group_id++;
  nodes.emplace_back();
  nodes[my_id].id = my_id;
  nodes[my_id].parent_id = parent_id;
  nodes[my_id].inst_name = inst_name;
  nodes[my_id].node_kind = HierarchyNodeKind::kNameGroup;
  nodes[my_id].odb_id = my_group;

  // Every instance resolves to a group so the overlay colors the whole
  // design, but the counts skip physical cells exactly as the module walk
  // does -- the two trees have to report the same totals for the same design.
  ModuleStats local;
  for (odb::dbInst* inst : groups[g].insts) {
    const uint32_t inst_id = inst->getId();
    if (inst_id >= inst_group.size()) {
      inst_group.resize(inst_id + 1, 0);
    }
    inst_group[inst_id] = my_group;

    if (isPhysicalType(sta->getInstanceType(inst))) {
      continue;
    }
    local.area_dbu2 += inst->getBBox()->getBox().area();
    if (inst->isBlock()) {
      local.macros++;
    } else {
      local.insts++;
    }
  }
  local.modules = static_cast<int>(groups[g].children.size());

  nodes[my_id].local_insts = local.insts;
  nodes[my_id].local_macros = local.macros;
  nodes[my_id].local_modules = local.modules;

  ModuleStats hier = local;
  for (const int child : groups[g].children) {
    ModuleStats child_stats = addNameGroup(groups,
                                           child,
                                           sta,
                                           my_id,
                                           groups[child].segment,
                                           next_id,
                                           next_group_id,
                                           inst_group,
                                           nodes);
    hier.insts += child_stats.insts;
    hier.macros += child_stats.macros;
    hier.modules += child_stats.modules;
    hier.area_dbu2 += child_stats.area_dbu2;
  }

  emitLeafNodes(groups[g].insts, sta, my_id, next_id, nodes);

  nodes[my_id].insts = hier.insts;
  nodes[my_id].macros = hier.macros;
  nodes[my_id].modules = hier.modules;
  nodes[my_id].area = static_cast<double>(hier.area_dbu2);

  return hier;
}

bool HierarchyReport::addNameGroups(odb::dbModule* top,
                                    HierarchyResult& result) const
{
  std::vector<NameGroupNode> groups(1);  // groups[0] is the top level itself

  // Reused across instances: the split writes views into the name it was
  // handed, so nothing is allocated here after the first few instances.
  std::vector<std::string_view> segments;
  const auto max_depth = static_cast<size_t>(std::max(name_group_depth_, 0));
  bool capped = false;

  for (odb::dbInst* inst : top->getInsts()) {
    block_->getPathSegments(inst->getConstName(), segments);
    // The last segment is the leaf instance name, never a group: "riscv/dp/_1_"
    // groups under riscv/dp, it does not create a group called _1_.
    const size_t path_len = segments.empty() ? 0 : segments.size() - 1;
    const size_t depth = std::min(path_len, max_depth);

    int cur = 0;
    for (size_t i = 0; i < depth; i++) {
      // getPathSegments splits purely textually, so "a//b" hands us an empty
      // segment.  It names no group anyone could read; skip it.
      if (segments[i].empty()) {
        continue;
      }
      std::string segment(segments[i]);
      auto it = groups[cur].child_index.find(segment);
      if (it != groups[cur].child_index.end()) {
        cur = it->second;
        continue;
      }
      if (groups.size() > kMaxNameGroups) {
        // Backstop for the depth cap, which does not bound breadth: stop
        // growing and leave the instance on the deepest group that exists.
        capped = true;
        break;
      }
      const int child = static_cast<int>(groups.size());
      groups.emplace_back();
      groups[child].segment = std::move(segment);
      groups[child].parent = cur;
      groups[cur].children.push_back(child);
      groups[cur].child_index.emplace(groups[child].segment, child);
      cur = child;
    }
    groups[cur].insts.push_back(inst);
  }

  if (groups.size() == 1) {
    // No name carried the delimiter.  Nothing to recover, and the caller's
    // flat walk is already the right answer -- which is why the fallback
    // needs no separate "does this design look hierarchical" test.
    return false;
  }

  // Sort each level so the tree reads alphabetically and the palette lands
  // the same way on every run, rather than following db iteration order.
  for (auto& group : groups) {
    std::sort(
        group.children.begin(), group.children.end(), [&groups](int a, int b) {
          return groups[a].segment < groups[b].segment;
        });
  }

  int next_id = 0;
  uint32_t next_group_id = 0;
  addNameGroup(groups,
               /*g=*/0,
               sta_,
               /*parent_id=*/-1,
               top->getName(),
               next_id,
               next_group_id,
               result.inst_group,
               result.nodes);

  result.name_grouped = true;
  result.name_groups_capped = capped;
  return true;
}

HierarchyResult HierarchyReport::getReport() const
{
  HierarchyResult result;

  if (!block_ || !sta_) {
    return result;
  }

  odb::dbModule* top = block_->getTopModule();
  if (!top) {
    return result;
  }

  // Only a fully flat design synthesizes.  A partially flattened one would
  // mix real modules with synthesized groups in one tree, and so mix the
  // color key spaces the renderer looks up -- out of scope.
  const bool flat = top->getModInstCount() == 0;
  if (!flat || !addNameGroups(top, result)) {
    int next_id = 0;
    addModule(top, sta_, -1, top->getName(), next_id, result.nodes);
  }

  // Convert area from DBU² to μm²
  const int dbu_per_um = block_->getDbUnitsPerMicron();
  const double dbu_to_um_sq
      = 1.0 / (static_cast<double>(dbu_per_um) * dbu_per_um);
  for (auto& node : result.nodes) {
    node.area *= dbu_to_um_sq;
  }

  // Assign palette colors to MODULE nodes in DFS order (the order
  // they appear in result.nodes, guaranteed by addModule's recursion).
  int color_idx = 0;
  for (auto& node : result.nodes) {
    if (isColorable(node.node_kind)) {
      node.color = kModuleColorPalette[color_idx % kModuleColorPaletteSize];
      color_idx++;
    }
  }

  return result;
}

// ─── Shared serialization ──────────────────────────────────────────────

boost::json::object serializeHierarchyResult(const HierarchyResult& result)
{
  boost::json::array nodes;
  nodes.reserve(result.nodes.size());
  for (const auto& n : result.nodes) {
    boost::json::object o;
    o["id"] = n.id;
    o["parent_id"] = n.parent_id;
    o["inst_name"] = n.inst_name;
    o["module_name"] = n.module_name;
    o["insts"] = n.insts;
    o["macros"] = n.macros;
    o["modules"] = n.modules;
    o["area"] = n.area;
    o["local_insts"] = n.local_insts;
    o["local_macros"] = n.local_macros;
    o["local_modules"] = n.local_modules;
    if (n.node_kind != HierarchyNodeKind::kModule) {
      o["node_kind"] = static_cast<int>(n.node_kind);
    }
    if (isColorable(n.node_kind)) {
      o["odb_id"] = static_cast<int>(n.odb_id);
      o["color"] = boost::json::array{static_cast<int>(n.color.r),
                                      static_cast<int>(n.color.g),
                                      static_cast<int>(n.color.b)};
    }
    nodes.emplace_back(std::move(o));
  }
  boost::json::object out;
  out["nodes"] = std::move(nodes);
  // Only sent in name-group mode, so a client that predates it sees exactly
  // the payload it saw before on a design with real modules.
  if (result.name_grouped) {
    out["name_grouped"] = true;
    out["name_groups_capped"] = result.name_groups_capped;
  }
  return out;
}

// ─── Default module color computation ──────────────────────────────────

std::map<uint32_t, Color> computeDefaultModuleColors(
    const HierarchyResult& result)
{
  // Build tree: children map + node lookup.
  std::map<int, std::vector<int>> children;
  std::map<int, const HierarchyNode*> node_map;
  for (const auto& n : result.nodes) {
    children[n.id];
    node_map[n.id] = &n;
    if (n.parent_id >= 0) {
      auto it = children.find(n.parent_id);
      if (it != children.end()) {
        it->second.push_back(n.id);
      }
    }
  }

  // Default collapse state + read palette colors.
  struct ModState
  {
    Color color;
    Color effective_color;
  };
  std::map<unsigned int, ModState> mod_state;
  std::set<int> collapsed;

  for (const auto& n : result.nodes) {
    const bool has_kids = !children[n.id].empty();
    if (has_kids) {
      if (n.node_kind == HierarchyNodeKind::kLeafGroup
          || n.node_kind == HierarchyNodeKind::kTypeGroup) {
        collapsed.insert(n.id);
      } else if (isColorable(n.node_kind) && n.parent_id >= 0) {
        collapsed.insert(n.id);
      }
    }
    if (isColorable(n.node_kind)) {
      mod_state[n.odb_id] = {.color = n.color, .effective_color = n.color};
    }
  }

  // Effective colors: collapsed ancestors override descendant colors.
  for (const auto& n : result.nodes) {
    if (!isColorable(n.node_kind)) {
      continue;
    }
    auto it = mod_state.find(n.odb_id);
    if (it == mod_state.end()) {
      continue;
    }
    Color inherited;
    bool found_ancestor = false;
    int pid = n.parent_id;
    while (pid >= 0) {
      auto nit = node_map.find(pid);
      if (nit == node_map.end()) {
        break;
      }
      const HierarchyNode* parent = nit->second;
      if (isColorable(parent->node_kind) && collapsed.contains(parent->id)) {
        auto pit = mod_state.find(parent->odb_id);
        if (pit != mod_state.end()) {
          inherited = pit->second.effective_color;
          found_ancestor = true;
        }
      }
      pid = parent->parent_id;
    }
    if (found_ancestor) {
      it->second.effective_color = inherited;
    }
  }

  // Build color map: every visible module contributes its effective color.
  // The tile renderer looks up each instance's direct module, so parent
  // and child colors don't conflict.
  std::map<uint32_t, Color> colors;
  for (const auto& n : result.nodes) {
    if (!isColorable(n.node_kind)) {
      continue;
    }
    auto it = mod_state.find(n.odb_id);
    if (it == mod_state.end()) {
      continue;
    }
    colors[n.odb_id] = it->second.effective_color;
  }
  return colors;
}

}  // namespace web
