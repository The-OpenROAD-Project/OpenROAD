// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "CubePanels.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "BoxFace.h"
#include "fsv/Geometry.h"

namespace fsv {

std::vector<std::vector<double>> panelSineIntegrals(const int n)
{
  std::vector<std::vector<double>> ints(kMaxMode + 1, std::vector<double>(n));
  for (int m = 1; m <= kMaxMode; m++) {
    const double w = m * std::numbers::pi / 2;
    for (int i = 0; i < n; i++) {
      const double x0 = -1.0 + 2.0 * i / n;
      const double x1 = -1.0 + 2.0 * (i + 1) / n;
      ints[m][i] = (std::cos(w * (x0 + 1)) - std::cos(w * (x1 + 1))) / w;
    }
  }
  return ints;
}

Vec3 cubeFacePoint(const BoxFace face,
                   const int n,
                   const int i,
                   const int j,
                   const double u,
                   const double v)
{
  const auto [t0, t1] = tangentAxes(face.axis);
  Vec3 p;
  p[index(face.axis)] = face.normalSign();
  p[index(t0)] = -1.0 + 2.0 * (i + u) / n;
  p[index(t1)] = -1.0 + 2.0 * (j + v) / n;
  return p;
}

Vec3 squareEdgePoint(const BoxFace edge,
                     const int n,
                     const int i,
                     const double u)
{
  // The edge's tangent in the y-z plane is the other of y and z.
  const Axis tangent = edge.axis == Axis::kY ? Axis::kZ : Axis::kY;
  Vec3 p{0, 0, 0};
  p[index(edge.axis)] = edge.normalSign();
  p[index(tangent)] = -1.0 + 2.0 * (i + u) / n;
  return p;
}

}  // namespace fsv
