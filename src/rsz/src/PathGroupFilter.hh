// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sta {
class MinMax;
class Network;
class Pin;
class Sta;
class Vertex;
}  // namespace sta

namespace utl {
class Logger;
}

namespace rsz {

class Resizer;

// The path groups `repair_timing -path_group` accepts.  Each one pairs a
// startpoint kind with an endpoint kind, which is all that is needed to decide
// whether a timing path belongs to the group.
enum class PathGroupType
{
  kNone,  // No restriction; every path is in the group.
  kReg2Reg,
  kIn2Reg,
  kReg2Out,
  kIn2Out,
  kGatedClock
};

// What a path ends at.  Clock gating checks end at the enable input of a gate
// that drives the clock network, which is neither a register data pin nor a
// primary output, so they form their own group.
enum class EndpointKind
{
  kRegister,
  kPrimaryOutput,
  kGatedClockEnable
};

// What a path starts at.  kAny belongs to a group that does not split by
// startpoint.
enum class StartpointKind
{
  kAny,
  kRegister,
  kPrimaryInput
};

// Names accepted by `repair_timing -path_group`, in the order they are
// reported to the user.
const std::vector<std::string_view>& pathGroupNames();

// kNone for an empty or unrecognized name.
PathGroupType findPathGroupType(std::string_view name);

// Validates a `repair_timing -path_group` name and returns the group repair
// should be restricted to, or "" to leave every path group eligible.  An
// unrecognized name warns and falls back to no restriction rather than failing
// the command.  A supported name that the SDC has no group_path for yet gets
// one here, so that timing reports group paths the same way repair_timing
// optimizes them.
//
// Reached from Tcl through Resizer::resolvePathGroup(), which is what keeps
// this header out of the swig wrapper.
std::string resolvePathGroupName(Resizer* resizer, const char* name);

// Decides whether the critical path at a start/endpoint belongs to the path
// group selected with `repair_timing -path_group`.  Classification is
// structural (primary port vs register vs clock gate enable) rather than SDC
// based, so it tracks the group names above no matter how the matching OpenSTA
// group_path was written.
//
// Cheap to build - construct one where it is used instead of caching it, so
// that it always reflects the group in effect for the current repair run.
class PathGroupFilter
{
 public:
  explicit PathGroupFilter(Resizer* resizer);

  bool enabled() const { return type_ != PathGroupType::kNone; }

  // True when the worst slack path to `endpoint` for `min_max` is in the
  // selected group.  Always true when no group is selected.
  bool endpointInGroup(sta::Vertex* endpoint, const sta::MinMax* min_max) const;

  // True when `startpoint` can launch a path in the selected group.  Only the
  // start side of the group is checked here; the end side is enforced by
  // endpointInGroup().
  bool startpointInGroup(sta::Vertex* startpoint) const;

 private:
  bool isPrimaryInput(const sta::Pin* pin) const;
  bool isPrimaryOutput(const sta::Pin* pin) const;
  bool isGatedClockEnable(const sta::Vertex* vertex) const;
  EndpointKind endpointKind(const sta::Vertex* endpoint) const;

  sta::Sta* sta_;
  sta::Network* network_;
  utl::Logger* logger_;
  PathGroupType type_;
};

}  // namespace rsz
