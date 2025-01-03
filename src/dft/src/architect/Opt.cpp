///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Myrtle Shah
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

#include "Opt.hh"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <iostream>

#include "ClockDomain.hh"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace dft {

namespace {
// Max number of cells we assume could be placed at the same point (hopefully,
// after DP, this is never more than one)
constexpr int kMaxCellsToSearch = 10;
}  // namespace

void OptimizeScanWirelength(std::vector<std::unique_ptr<ScanCell>>& cells,
                            utl::Logger* logger)
{
  // Nothing to order
  if (cells.empty()) {
    return;
  }
  // No point running this if the cells aren't placed yet
  for (const auto& cell : cells) {
    if (!cell->isPlaced()) {
      return;
    }
  }
  // Define the starting node as the lower leftmost, so we don't accidentally
  // start somewhere in the middle
  size_t start_index = 0;
  int64_t lowest_dist = std::numeric_limits<int64_t>::max();
  // Get points in a form ready to insert into index
  using bg_point = bg::model::point<int, 2, bg::cs::cartesian>;
  std::vector<std::pair<bg_point, size_t>> transformed;

  for (size_t i = 0; i < cells.size(); i++) {
    // Find the lower leftmost cell by looking for the cell with the lowest
    // manhattan distance to the origin
    auto origin = cells[i]->getOrigin();
    const int64_t dist = origin.x() + origin.y();
    if (dist < lowest_dist) {
      start_index = i;
      lowest_dist = dist;
    }
    // Add the point to the set of points
    transformed.emplace_back(bg_point(origin.x(), origin.y()), i);
  }
  // Update the index
  bgi::rtree<std::pair<bg_point, size_t>, bgi::rstar<4>> rtree(transformed);
  auto cursor = transformed[start_index];

  // Search nearest neighbours
  std::vector<std::unique_ptr<ScanCell>> result;
  result.emplace_back(std::move(cells[cursor.second]));
  while (result.size() < cells.size()) {
    // Search for next nearest
    auto next = cursor;
    for (auto it = rtree.qbegin(bgi::nearest(cursor.first, kMaxCellsToSearch));
         it != rtree.qend();
         ++it) {
      next = *it;
      if (next.second != cursor.second) {
        break;
      }
    }
    if (next.second == cursor.second) {
      logger->error(
          utl::DFT,
          10,
          "Couldn't find nearest neighbor, too many overlapping cells");
    }
    // Make sure we only visit things once
    rtree.remove(cursor);
    cursor = std::move(next);
    result.emplace_back(std::move(cells[cursor.second]));
  }
  // Replace with sorted vector
  std::swap(cells, result);
}

}  // namespace dft
