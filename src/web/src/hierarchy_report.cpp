// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "hierarchy_report.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
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
  leaf.node_kind = HierarchyNodeKind::LEAF_GROUP;

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
    type_node.node_kind = HierarchyNodeKind::TYPE_GROUP;
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
        inst_node.node_kind = HierarchyNodeKind::INSTANCE;
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

  return result;
}

}  // namespace web
