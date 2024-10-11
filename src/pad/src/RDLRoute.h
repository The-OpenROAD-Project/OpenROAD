/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "RDLRouter.h"

namespace odb {
class dbITerm;
class dbNet;
}  // namespace odb

namespace pad {

class RDLRoute
{
 public:
  RDLRoute(odb::dbITerm* source,
           const std::vector<odb::dbITerm*>& dests,
           const std::map<grid_vertex, odb::Point>& vertex_point_map);

  void setRoute(
      const std::vector<grid_vertex>& vertex,
      const std::set<std::tuple<odb::Point, odb::Point, float>>& removed_edges,
      const RouteTarget* source,
      const RouteTarget* target);
  void resetRoute();

  bool isRouted() const { return !route_vertex_.empty(); }
  bool isFailed() const
  {
    return !route_pending_ && !isRouted() && !hasNextTerminal();
  }
  bool allowRipup(int other_priority) const
  {
    return isRouted() && other_priority >= priority_;
  }

  void markRouting() { route_pending_ = false; }

  int getPriority() const { return priority_; }
  odb::dbITerm* getTerminal() const { return iterm_; }
  odb::dbNet* getNet() const { return iterm_->getNet(); }

  const std::vector<odb::dbITerm*>& getTerminals() const { return terminals_; }

  void increasePriority() { priority_++; }

  bool hasNextTerminal() const { return next_ != terminals_.end(); }
  odb::dbITerm* getNextTerminal();
  odb::dbITerm* peakNextTerminal() const;
  void moveNextTerminalToEnd() { next_ = terminals_.end(); }

  bool compare(const std::shared_ptr<RDLRoute>& other) const;

  const std::vector<grid_vertex>& getRouteVerticies() const
  {
    return route_vertex_;
  }
  const std::set<std::tuple<odb::Point, odb::Point, float>>& getRouteEdges()
      const
  {
    return route_edges_;
  }
  const RouteTarget* getRouteTargetSource() const { return route_source_; }
  const RouteTarget* getRouteTargetDestination() const { return route_dest_; }

  bool isIntersecting(RDLRoute* other, int extent) const;

 private:
  odb::dbITerm* iterm_;
  int priority_;

  bool route_pending_;

  const std::map<grid_vertex, odb::Point>& vertex_point_map_;

  std::vector<odb::dbITerm*> terminals_;
  std::vector<odb::dbITerm*>::iterator next_;

  std::vector<grid_vertex> route_vertex_;
  std::set<std::tuple<odb::Point, odb::Point, float>> route_edges_;
  const RouteTarget* route_source_;
  const RouteTarget* route_dest_;
};

}  // namespace pad
