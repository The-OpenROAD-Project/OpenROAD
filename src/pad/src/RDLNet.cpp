// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "RDLNet.h"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "RDLRouter.h"
#include "RDLSegment.h"
#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon_90_set_data.hpp"
#include "boost/polygon/polygon_90_with_holes_data.hpp"
#include "boost/polygon/rectangle_concept.hpp"
#include "boost/polygon/rectangle_data.hpp"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace pad {

RDLNet::RDLNet(odb::dbNet* net) : net_(net)
{
}

bool RDLNet::isRouted() const
{
  for (const auto& segment : segments_) {
    if (!segment->isRouted()) {
      return false;
    }
  }
  return true;
}

bool RDLNet::isFailed() const
{
  for (const auto& segment : segments_) {
    if (segment->isFailed()) {
      return true;
    }
  }
  return false;
}

void RDLNet::addSegment(odb::dbITerm* iterm,
                        const std::vector<odb::dbITerm*>& terminals)
{
  segments_.emplace_back(std::make_unique<RDLSegment>(this, iterm, terminals));
}

void RDLNet::finalizeSegments()
{
  bool has_non_cover = false;
  for (const auto& segment : segments_) {
    for (odb::dbITerm* dest : segment->getTerminals()) {
      if (!RDLRouter::isCoverTerm(dest)) {
        has_non_cover = true;
        break;
      }
    }
    if (has_non_cover) {
      break;
    }
  }

  if (!has_non_cover && !segments_.empty()) {
    // Remove one cover segment only route N-1 segments
    // Overwise we route the same cover more than once.
    segments_.pop_back();
  }
}

void RDLNet::updateRoute(RDLSegment* segment)
{
  if (segment->isRouted()) {
    odb::dbITerm* source = segment->getTerminal();
    if (!RDLRouter::isCoverTerm(source)) {
      routed_noncover_terminals_.insert(source);
    }
    for (odb::dbITerm* dest : segment->getRoutedTerminals()) {
      routed_pairs_[source].insert(dest);
      routed_pairs_[dest].insert(source);
      if (!RDLRouter::isCoverTerm(dest)) {
        routed_noncover_terminals_.insert(dest);
      }
    }
  } else if (!routed_pairs_.empty() || !routed_noncover_terminals_.empty()) {
    odb::dbITerm* source = segment->getTerminal();
    if (!RDLRouter::isCoverTerm(source)) {
      routed_noncover_terminals_.erase(source);
    }
    for (odb::dbITerm* dest : segment->getRoutedTerminals()) {
      routed_pairs_[source].erase(dest);
      routed_pairs_[dest].erase(source);
      if (!RDLRouter::isCoverTerm(dest)) {
        routed_noncover_terminals_.erase(dest);
      }
    }
  }
}

bool RDLNet::isRouted(odb::dbITerm* source, odb::dbITerm* dest) const
{
  if (source == dest) {
    return true;
  }

  odb::PtrSet<odb::dbITerm> visited;
  return isRouted(source, dest, visited);
}

bool RDLNet::isRouted(odb::dbITerm* source,
                      odb::dbITerm* dest,
                      odb::PtrSet<odb::dbITerm>& visited) const
{
  auto find_source = routed_pairs_.find(source);
  if (find_source == routed_pairs_.end()) {
    return false;
  }

  const auto& dests = find_source->second;
  if (dests.find(dest) != dests.end()) {
    return true;
  }

  // Decend the routes to check
  for (odb::dbITerm* routed_dest : dests) {
    if (visited.find(routed_dest) == visited.end()) {
      visited.insert(routed_dest);
      if (isRouted(routed_dest, dest, visited)) {
        return true;
      }
    }
  }

  return false;
}

bool RDLNet::isNonCoverRouted(odb::dbITerm* iterm) const
{
  return routed_noncover_terminals_.find(iterm)
         != routed_noncover_terminals_.end();
}

}  // namespace pad
