/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
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

#include "RDLRoute.h"

#include "odb/geom_boost.h"

namespace pad {

RDLRoute::RDLRoute(odb::dbITerm* source,
                   const std::vector<odb::dbITerm*>& dests,
                   const std::map<grid_vertex, odb::Point>& vertex_point_map)
    : iterm_(source),
      priority_(0),
      vertex_point_map_(vertex_point_map),
      terminals_(dests)
{
  terminals_.erase(std::remove_if(terminals_.begin(),
                                  terminals_.end(),
                                  [this](odb::dbITerm* other) {
                                    return iterm_->getInst()
                                           == other->getInst();
                                  }),
                   terminals_.end());

  const odb::Point iterm_center = iterm_->getBBox().center();
  std::stable_sort(terminals_.begin(),
                   terminals_.end(),
                   [&iterm_center](odb::dbITerm* lhs, odb::dbITerm* rhs) {
                     const bool lhs_cover = RDLRouter::isCoverTerm(lhs);
                     const bool rhs_cover = RDLRouter::isCoverTerm(rhs);

                     const auto lhs_dist = odb::Point::squaredDistance(
                         iterm_center, lhs->getBBox().center());
                     const auto rhs_dist = odb::Point::squaredDistance(
                         iterm_center, rhs->getBBox().center());
                     // sort non-cover terms first
                     return std::tie(lhs_cover, lhs_dist)
                            < std::tie(rhs_cover, rhs_dist);
                   });

  resetRoute();
}

odb::dbITerm* RDLRoute::peakNextTerminal() const
{
  return *next_;
}

odb::dbITerm* RDLRoute::getNextTerminal()
{
  odb::dbITerm* iterm = peakNextTerminal();

  next_++;

  return iterm;
}

bool RDLRoute::compare(const std::shared_ptr<RDLRoute>& other) const
{
  const auto lhs_priority = -getPriority();
  const auto rhs_priority = -other->getPriority();
  if (!hasNextTerminal() || !other->hasNextTerminal()
      || lhs_priority != rhs_priority) {
    return lhs_priority > rhs_priority;
  }

  const auto& lhs_shortest = peakNextTerminal();
  const auto lhs_dist = odb::Point::squaredDistance(
      getTerminal()->getBBox().center(), lhs_shortest->getBBox().center());

  const auto& rhs_shortest = other->peakNextTerminal();
  const auto rhs_dist
      = odb::Point::squaredDistance(other->getTerminal()->getBBox().center(),
                                    rhs_shortest->getBBox().center());

  return lhs_dist > rhs_dist;
}

void RDLRoute::setRoute(
    const std::vector<grid_vertex>& vertex,
    const std::set<std::tuple<odb::Point, odb::Point, float>>& removed_edges,
    const RouteTarget* source,
    const RouteTarget* target)
{
  route_vertex_ = vertex;
  route_edges_ = removed_edges;
  route_source_ = source;
  route_dest_ = target;
}

void RDLRoute::resetRoute()
{
  route_vertex_.clear();
  route_edges_.clear();
  route_source_ = nullptr;
  route_dest_ = nullptr;
  route_pending_ = true;

  next_ = terminals_.begin();
}

bool RDLRoute::isIntersecting(RDLRoute* other, int extent) const
{
  // check current next_
  // if at end of next, select begin + priority offset (modulo)
  // this ensures the ripup does something different (by checking
  // different destinations, where possible) if a route fails to
  // route more than one time.
  const int dst_idx = priority_ % terminals_.size();
  odb::dbITerm* dst = terminals_[dst_idx];

  // create line with width (extent * (priority + 1))
  const odb::Point pt0 = iterm_->getBBox().center();
  const odb::Point pt1 = dst->getBBox().center();
  const std::vector<odb::Point> line_segment = {pt0, pt1};

  const int margin = (priority_ + 1) * extent;

  // check intersection with routed other
  for (const auto& vertex : other->getRouteVerticies()) {
    const odb::Point& pt = vertex_point_map_.at(vertex);
    const odb::Rect rect(
        pt.x() - margin, pt.y() - margin, pt.x() + margin, pt.y() + margin);

    if (boost::geometry::intersects(line_segment, rect)) {
      return true;
    }
  }

  return false;
}

}  // namespace pad
