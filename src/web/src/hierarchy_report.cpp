// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "hierarchy_report.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "json_builder.h"
#include "module_color_palette.h"
#include "odb/db.h"

namespace web {

HierarchyReport::HierarchyReport(odb::dbBlock* block, sta::dbSta* sta)
    : block_(block), sta_(sta)
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

// Emit "Leaf instances" folder with type sub-groups for a module.
static void emitLeafNodes(odb::dbModule* module,
                          sta::dbSta* sta,
                          int parent_id,
                          int& next_id,
                          std::vector<HierarchyNode>& nodes)
{
  // Group instances by type
  std::map<std::string, TypeBucket> buckets;
  for (odb::dbInst* inst : module->getInsts()) {
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
  emitLeafNodes(module, sta, my_id, next_id, nodes);

  // Write hierarchical totals (area stored as DBU², converted later)
  nodes[my_id].insts = hier.insts;
  nodes[my_id].macros = hier.macros;
  nodes[my_id].modules = hier.modules;
  nodes[my_id].area = static_cast<double>(hier.area_dbu2);

  return hier;
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

  int next_id = 0;
  addModule(top, sta_, -1, top->getName(), next_id, result.nodes);

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
    if (node.node_kind == HierarchyNodeKind::kModule) {
      node.color = kModuleColorPalette[color_idx % kModuleColorPaletteSize];
      color_idx++;
    }
  }

  return result;
}

// ─── Shared serialization ──────────────────────────────────────────────

void serializeHierarchyResult(JsonBuilder& b, const HierarchyResult& result)
{
  b.beginObject();
  b.beginArray("nodes");
  for (const auto& n : result.nodes) {
    b.beginObject();
    b.field("id", n.id);
    b.field("parent_id", n.parent_id);
    b.field("inst_name", n.inst_name);
    b.field("module_name", n.module_name);
    b.field("insts", n.insts);
    b.field("macros", n.macros);
    b.field("modules", n.modules);
    b.field("area", n.area);
    b.field("local_insts", n.local_insts);
    b.field("local_macros", n.local_macros);
    b.field("local_modules", n.local_modules);
    if (n.node_kind != HierarchyNodeKind::kModule) {
      b.field("node_kind", static_cast<int>(n.node_kind));
    }
    if (n.node_kind == HierarchyNodeKind::kModule) {
      b.field("odb_id", static_cast<int>(n.odb_id));
      b.beginArray("color");
      b.value(static_cast<int>(n.color.r));
      b.value(static_cast<int>(n.color.g));
      b.value(static_cast<int>(n.color.b));
      b.endArray();
    }
    b.endObject();
  }
  b.endArray();
  b.endObject();
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
      } else if (n.node_kind == HierarchyNodeKind::kModule
                 && n.parent_id >= 0) {
        collapsed.insert(n.id);
      }
    }
    if (n.node_kind == HierarchyNodeKind::kModule) {
      mod_state[n.odb_id] = {.color = n.color, .effective_color = n.color};
    }
  }

  // Effective colors: collapsed ancestors override descendant colors.
  for (const auto& n : result.nodes) {
    if (n.node_kind != HierarchyNodeKind::kModule) {
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
      if (parent->node_kind == HierarchyNodeKind::kModule
          && collapsed.contains(parent->id)) {
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
    if (n.node_kind != HierarchyNodeKind::kModule) {
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
