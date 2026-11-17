// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <vector>

namespace sta {
class Pin;
}  // namespace sta

namespace web {

// Worst slack (STA units) of each endpoint within one path group.
using EndpointSlacks = std::map<const sta::Pin*, float>;

struct EndpointGroupSlack
{
  size_t group;  // index into the groups passed to assignEndpointsToWorstGroup
  float slack;
};

// Keep each endpoint once, in the group where its slack is worst; ties go to
// the earlier group.
inline std::map<const sta::Pin*, EndpointGroupSlack>
assignEndpointsToWorstGroup(const std::vector<const EndpointSlacks*>& groups)
{
  std::map<const sta::Pin*, EndpointGroupSlack> winners;
  for (size_t group = 0; group < groups.size(); ++group) {
    for (const auto& [pin, slack] : *groups[group]) {
      auto [it, inserted]
          = winners.try_emplace(pin, EndpointGroupSlack{group, slack});
      if (!inserted && slack < it->second.slack) {
        it->second = {group, slack};
      }
    }
  }
  return winners;
}

}  // namespace web
