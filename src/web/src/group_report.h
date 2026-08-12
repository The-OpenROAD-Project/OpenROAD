// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "boost/json/object.hpp"
#include "color.h"
#include "odb/geom.h"

namespace odb {
class dbBlock;
}  // namespace odb

namespace web {

// One row of the Clusters view: a dbGroup.  MPL writes its clustering tree as
// nested VISUAL_DEBUG groups; power/voltage domains show up too, told apart by
// `type`.
struct GroupNode
{
  int id = 0;
  int parent_id = -1;  // -1 for a top-level group
  std::string name;
  std::string type;         // dbGroupType string, e.g. "VISUAL_DEBUG"
  unsigned int odb_id = 0;  // dbGroup::getId()
  int insts = 0;            // hierarchical stdcell count
  int macros = 0;           // hierarchical macro count
  int groups = 0;           // hierarchical sub-group count
  double area = 0.0;        // hierarchical area (μm²)
  int local_insts = 0;      // direct stdcell count
  int local_macros = 0;     // direct macro count
  int local_groups = 0;     // direct child group count
  odb::Rect bbox;           // union of member instance boxes (DBU)
  Color color;              // set by getReport()
};

struct GroupResult
{
  std::vector<GroupNode> nodes;
};

class GroupReport
{
 public:
  explicit GroupReport(odb::dbBlock* block);

  GroupResult getReport() const;

 private:
  odb::dbBlock* block_;
};

// JSON serialization (shared by handleGroupHierarchy and saveReport).
boost::json::object serializeGroupResult(const GroupResult& result);

// Group color map for the default UI state (child groups collapsed under their
// top-level parent, all visible): odb group id → RGBA.
std::map<uint32_t, Color> computeDefaultGroupColors(const GroupResult& result);

}  // namespace web
