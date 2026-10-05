// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "sta/Delay.hh"

namespace sta {
class MinMax;
class Network;
class Path;
class PathEnd;
class Pin;
class Sdc;
class Sta;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}

namespace rsz {

class Resizer;

// The path groups `repair_timing -path_group` accepts, each a startpoint kind
// paired with an endpoint kind.
enum class PathGroupType
{
  kNone,  // No restriction; every path is in the group.
  kReg2Reg,
  kIn2Reg,
  kReg2Out,
  kIn2Out,
  kGatedClock
};

// What a path ends at.  A clock gate enable is neither a register data pin nor
// a primary output, so it forms its own group.
enum class EndpointKind
{
  kRegister,
  kPrimaryOutput,
  kGatedClockEnable
};

// What a path starts at.  kAny is a group that does not split by startpoint.
enum class StartpointKind
{
  kAny,
  kRegister,
  kPrimaryInput
};

// Names accepted by `repair_timing -path_group`, in report order.
const std::vector<std::string_view>& pathGroupNames();

// kNone for an empty or unrecognized name.
PathGroupType findPathGroupType(std::string_view name);

// Validates a `repair_timing -path_group` name, returning the group to
// restrict repair to or "" for no restriction.  An unrecognized name warns
// instead of failing the command.  A supported name with no group_path in the
// SDC gets one here, so timing reports group paths the way repair_timing
// optimizes them.
//
// Reached from Tcl through Resizer::resolvePathGroup(), which is what keeps
// this header out of the swig wrapper.
std::string resolvePathGroupName(Resizer* resizer, const char* name);

// Decides whether the critical path at a start/endpoint belongs to the group
// selected with `repair_timing -path_group`.  Classification is structural
// (primary port vs register vs clock gate enable), so it holds however the
// matching OpenSTA group_path was written.
//
// Cheap to build - construct one where it is used rather than caching it, so
// it always reflects the group in effect for the current repair run.
class PathGroupFilter
{
 public:
  explicit PathGroupFilter(Resizer* resizer);

  bool enabled() const { return type_ != PathGroupType::kNone; }

  // True when `endpoint` has any path in the selected group for `min_max`.
  // Always true when no group is selected.
  bool endpointInGroup(sta::Vertex* endpoint, const sta::MinMax* min_max) const;

  // Worst slack among the group's paths to `endpoint`, or nullopt when it
  // hosts none; the endpoint's plain slack when no group is selected.
  //
  // An endpoint hosts paths from several groups at once and its worst one
  // often belongs to another group, so the group's slack must come from
  // OpenSTA rather than from the endpoint's worst path.
  std::optional<sta::Slack> groupSlack(sta::Vertex* endpoint,
                                       const sta::MinMax* min_max) const;

  // The group's worst path to `endpoint`, or nullptr when it hosts none or no
  // group is selected, leaving callers on their own path lookup.
  //
  // An endpoint qualifies by hosting *a* path in the group, so repairing its
  // worst path would optimize whatever group that one belongs to instead.
  //
  // The path is owned by the graph, not the query, since asking for one path
  // end skips enumeration; it lives as long as a vertexWorstSlackPath() would.
  sta::Path* groupPath(sta::Vertex* endpoint, const sta::MinMax* min_max) const;

  // The selected group's worst path across the whole design.
  struct GroupWorst
  {
    sta::Slack slack;
    const sta::Pin* endpoint;
  };

  // Worst slack in the selected group and the endpoint it lands on, or nullopt
  // when no group is selected or the group has no path.
  //
  // This is the group's analogue of sta::worstSlack(), and like it reflects
  // the design as it stands.  Scanning a list of endpoints collected earlier
  // in the run would go stale the moment a repair pushes some other endpoint
  // of the group negative.
  std::optional<GroupWorst> groupWorst(const sta::MinMax* min_max) const;

  // True when `startpoint` can launch a path in the selected group.  Only the
  // start side is checked; endpointInGroup() enforces the end side.
  bool startpointInGroup(sta::Vertex* startpoint) const;

 private:
  // Worst path end at `endpoint` within the group, nullptr when none.  Shared
  // by groupSlack() and groupPath() so both answer from one query.
  sta::PathEnd* worstGroupEnd(sta::Vertex* endpoint,
                              const sta::MinMax* min_max) const;
  bool isPrimaryInput(const sta::Pin* pin) const;
  bool isPrimaryOutput(const sta::Pin* pin) const;
  bool isGatedClockEnable(const sta::Vertex* vertex) const;
  EndpointKind endpointKind(const sta::Vertex* endpoint) const;

  sta::Sta* sta_;
  sta::Network* network_;
  sta::Sdc* sdc_;
  utl::Logger* logger_;
  PathGroupType type_;
};

}  // namespace rsz
