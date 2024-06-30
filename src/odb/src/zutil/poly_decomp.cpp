///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include <algorithm>
#include <list>
#include <vector>

#include "odb/geom_boost.h"

namespace gtl = boost::polygon;

namespace odb {

void decompose_polygon(const std::vector<Point>& points,
                       std::vector<Rect>& rects)
{
  using boost::polygon::operators::operator+=;

  gtl::polygon_90_data<int> polygon;
  polygon.set(points.begin(), points.end());

  gtl::polygon_90_set_data<int> polygon_set;
  polygon_set += polygon;

  polygon_set.get_rectangles(rects);
}

// See "Orientation of a simple polygon" in
// https://en.wikipedia.org/wiki/Curve_orientation
// The a, b, c point names are used to match the wiki page
bool polygon_is_clockwise(const std::vector<Point>& P)
{
  const int n = P.size();

  if (n < 3) {
    return false;
  }

  // find a point on the convex hull of the polygon
  // Here we use the lowest-most in Y with lowest in X as a tie breaker
  int yMin = std::numeric_limits<int>::max();
  int xMin = std::numeric_limits<int>::max();
  int b = 0;  // the index of the point we are seeking
  for (int i = 0; i < n; ++i) {
    const int x = P[i].x();
    const int y = P[i].y();
    if (y < yMin || (y == yMin && x < xMin)) {
      b = i;
      yMin = y;
      xMin = x;
    }
  }

  const int a = b > 0 ? b - 1 : n - 1;  // previous pt to b
  const int c = b < n - 1 ? b + 1 : 0;  // next pt to b

  const double xa = P[a].getX();
  const double ya = P[a].getY();

  const double xb = P[b].getX();
  const double yb = P[b].getY();

  const double xc = P[c].getX();
  const double yc = P[c].getY();

  const double det
      = (xb * yc + xa * yb + ya * xc) - (ya * xb + yb * xc + xa * yc);
  return det < 0.0;
}

}  // namespace odb
