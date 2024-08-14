///////////////////////////////////////////////////////////////////////////////
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

#include "odb/geom.h"

#include "odb/geom_boost.h"

namespace odb {

void Polygon::setPoints(const std::vector<Point>& points)
{
  points_ = points;
  boost::geometry::correct(points_);
}

Polygon Polygon::bloat(int margin) const
{
  // convert to native boost types to avoid needed a mutable access
  // to odb::Polygon
  using BoostPolygon = boost::polygon::polygon_data<int>;
  using BoostPolygonSet = boost::polygon::polygon_set_data<int>;
  using boost::polygon::operators::operator+=;
  using boost::polygon::operators::operator+;

  // convert to boost polygon
  const BoostPolygon polygon_in(points_.begin(), points_.end());

  // add to polygon set
  BoostPolygonSet poly_in_set;
  poly_in_set += polygon_in;

  // bloat polygon set
  const BoostPolygonSet poly_out_set = poly_in_set + margin;

  // extract new polygon
  std::vector<BoostPolygon> output_polygons;
  poly_out_set.get(output_polygons);
  const BoostPolygon& polygon_out = output_polygons[0];

  std::vector<odb::Point> new_coord;
  new_coord.reserve(polygon_out.coords_.size());
  for (const auto& pt : polygon_out.coords_) {
    new_coord.emplace_back(pt.x(), pt.y());
  }

  return Polygon(new_coord);
}

}  // namespace odb
