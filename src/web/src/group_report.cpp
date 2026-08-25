// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "group_report.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "boost/json/array.hpp"
#include "hierarchy_report.h"
#include "module_color_palette.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace web {

GroupReport::GroupReport(odb::dbBlock* block) : block_(block)
{
}

namespace {

struct GroupStats
{
  int insts = 0;
  int macros = 0;
  int groups = 0;
  int64_t area_dbu2 = 0;
  odb::Rect bbox;  // inverted (empty) until the first instance is seen
};

// Recursive DFS: adds a node for the group, recurses into child groups,
// then writes back hierarchical totals.  Returns hierarchical stats.
GroupStats addGroup(odb::dbGroup* group,
                    const int parent_id,
                    int& next_id,
                    std::vector<GroupNode>& nodes)
{
  const int my_id = next_id++;
  nodes.emplace_back();
  nodes[my_id].id = my_id;
  nodes[my_id].parent_id = parent_id;
  nodes[my_id].name = group->getName();
  nodes[my_id].type = group->getType().getString();
  nodes[my_id].odb_id = group->getId();

  // Fillers are skipped to match the _clusters overlay, which never colors
  // them.
  GroupStats local;
  local.bbox.mergeInit();
  for (odb::dbInst* inst : group->getInsts()) {
    if (inst->getMaster()->isFiller()) {
      continue;
    }
    const odb::Rect box = inst->getBBox()->getBox();
    local.area_dbu2 += box.area();
    local.bbox.merge(box);
    if (inst->isBlock()) {
      local.macros++;
    } else {
      local.insts++;
    }
  }

  GroupStats hier = local;
  for (odb::dbGroup* child : group->getGroups()) {
    local.groups++;
    const GroupStats child_stats = addGroup(child, my_id, next_id, nodes);
    hier.insts += child_stats.insts;
    hier.macros += child_stats.macros;
    hier.groups += child_stats.groups + 1;
    hier.area_dbu2 += child_stats.area_dbu2;
    if (!child_stats.bbox.isInverted()) {
      hier.bbox.merge(child_stats.bbox);
    }
  }

  nodes[my_id].local_insts = local.insts;
  nodes[my_id].local_macros = local.macros;
  nodes[my_id].local_groups = local.groups;

  nodes[my_id].insts = hier.insts;
  nodes[my_id].macros = hier.macros;
  nodes[my_id].groups = hier.groups;
  // Area stored as DBU² here; converted to μm² by getReport().
  nodes[my_id].area = static_cast<double>(hier.area_dbu2);
  if (!hier.bbox.isInverted()) {
    nodes[my_id].bbox = hier.bbox;
  }

  return hier;
}

}  // namespace

GroupResult GroupReport::getReport() const
{
  GroupResult result;

  if (!block_) {
    return result;
  }

  // One node per dbGroup, and getGroups() is the flat table of every group in
  // the block, so this is the exact final size.  dbSet over a dbTable reports
  // it in O(1).
  result.nodes.reserve(block_->getGroups().size());

  // dbBlock::getGroups() is the flat group table, so child groups appear
  // in it too — recurse only from the roots to build the tree once.
  int next_id = 0;
  for (odb::dbGroup* group : block_->getGroups()) {
    if (group->getParentGroup() == nullptr) {
      addGroup(group, -1, next_id, result.nodes);
    }
  }

  // Convert area from DBU² to μm²
  const int dbu_per_um = block_->getDbUnitsPerMicron();
  const double dbu_to_um_sq
      = 1.0 / (static_cast<double>(dbu_per_um) * dbu_per_um);
  for (auto& node : result.nodes) {
    node.area *= dbu_to_um_sq;
  }

  // Assign palette colors in DFS order; addGroup numbers nodes as it appends
  // them, so the id IS the DFS position.
  for (auto& node : result.nodes) {
    node.color = kModuleColorPalette[node.id % kModuleColorPaletteSize];
  }

  return result;
}

// ─── Shared serialization ──────────────────────────────────────────────

boost::json::object serializeGroupResult(const GroupResult& result)
{
  boost::json::array nodes;
  nodes.reserve(result.nodes.size());
  for (const auto& n : result.nodes) {
    boost::json::object o;
    o["id"] = n.id;
    o["parent_id"] = n.parent_id;
    o["name"] = n.name;
    o["type"] = n.type;
    o["odb_id"] = static_cast<int>(n.odb_id);
    o["insts"] = n.insts;
    o["macros"] = n.macros;
    o["groups"] = n.groups;
    o["area"] = n.area;
    o["local_insts"] = n.local_insts;
    o["local_macros"] = n.local_macros;
    o["local_groups"] = n.local_groups;
    o["bbox"] = boost::json::array{
        n.bbox.xMin(), n.bbox.yMin(), n.bbox.xMax(), n.bbox.yMax()};
    o["color"] = boost::json::array{static_cast<int>(n.color.r),
                                    static_cast<int>(n.color.g),
                                    static_cast<int>(n.color.b)};
    nodes.emplace_back(std::move(o));
  }
  boost::json::object out;
  out["nodes"] = std::move(nodes);
  return out;
}

// ─── Default group color computation ───────────────────────────────────

std::map<uint32_t, Color> computeDefaultGroupColors(const GroupResult& result)
{
  // Mirrors ClustersWidget._buildTree: every non-root group with children
  // starts collapsed, so a top-level cluster paints its whole subtree.
  std::vector<int> parent_ids;
  parent_ids.reserve(result.nodes.size());
  for (const auto& n : result.nodes) {
    parent_ids.push_back(n.parent_id);
  }
  const std::vector<char> has_children
      = hasChildrenByIndex(result.nodes.size(), parent_ids);

  std::vector<OwnerColorNode> nodes;
  nodes.reserve(result.nodes.size());
  for (const auto& n : result.nodes) {
    nodes.push_back({.parent = n.parent_id,
                     .odb_id = n.odb_id,
                     .color = n.color,
                     .collapsed = n.parent_id >= 0 && has_children[n.id] != 0,
                     .has_color = true});
  }
  // getReport() emits DFS order with id == index (see addGroup), which is the
  // parent-before-child order the shared helper requires.
  return computeEffectiveOwnerColors(nodes);
}

}  // namespace web
