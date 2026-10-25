// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cmath>
#include <vector>

#include "Dielectric.h"
#include "GaussianSurface.h"
#include "Philox.h"
#include "SpatialIndex.h"
#include "fsv/Geometry.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

template <int D>
using Pieces = std::vector<typename GaussianSurface<D>::Piece>;

// Every exposed piece of every face of the boxes.
template <int D>
Pieces<D> allPieces(const std::vector<AlignedBox<D>>& boxes)
{
  const SpatialIndex<D> index(boxes);
  Pieces<D> pieces;
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    for (int axis = 0; axis < D; axis++) {
      for (const bool high : {false, true}) {
        for (const auto& piece : exposedPieces(boxes, index, b, axis, high)) {
          pieces.push_back(piece);
        }
      }
    }
  }
  return pieces;
}

template <int D>
double totalArea(const Pieces<D>& pieces)
{
  double sum = 0;
  for (const auto& piece : pieces) {
    sum += piece.area();
  }
  return sum;
}

// The rule, point by point: whether q on face (axis, high) of box b lies on
// the boundary of the union and b owns it there.
template <int D>
bool exposedAt(const std::vector<AlignedBox<D>>& boxes,
               const int b,
               const int axis,
               const bool high,
               const Vec<D>& q)
{
  for (int c = 0; c < static_cast<int>(boxes.size()); c++) {
    const AlignedBox<D>& e = boxes[c];
    bool across = c != b;
    for (int t = 0; across && t < D; t++) {
      across = t == axis || (e.lo[t] < q[t] && q[t] < e.hi[t]);
    }
    if (!across) {
      continue;
    }
    const bool inside = e.lo[axis] < q[axis] && q[axis] < e.hi[axis];
    const bool continues = (high ? e.lo[axis] : e.hi[axis]) == q[axis];
    const bool lower_coplanar
        = c < b && (high ? e.hi[axis] : e.lo[axis]) == q[axis];
    if (inside || continues || lower_coplanar) {
      return false;
    }
  }
  return true;
}

TEST(GaussianSurface, SingleBoxIsItsFaces)
{
  const std::vector<Box> boxes{{{0, 0, 0}, {1, 2, 3}}};
  const Pieces<3> pieces = allPieces(boxes);
  EXPECT_EQ(pieces.size(), 6u);
  EXPECT_DOUBLE_EQ(totalArea<3>(pieces), 2 * (1 * 2 + 2 * 3 + 1 * 3));
}

// Boxes meeting face to face: the shared face is inside the union.
TEST(GaussianSurface, AbuttingBoxesDropTheSharedFace)
{
  const std::vector<Box> boxes{{{0, 0, 0}, {1, 1, 1}}, {{1, 0, 0}, {2, 1, 1}}};
  EXPECT_DOUBLE_EQ(totalArea<3>(allPieces(boxes)), 10);
}

// Overlapping boxes of the same height: their coplanar faces count once.
TEST(GaussianSurface, OverlappingBoxesCountCoplanarFacesOnce)
{
  const std::vector<Box> boxes{{{0, 0, 0}, {2, 1, 1}}, {{1, 0, 0}, {3, 1, 1}}};
  EXPECT_DOUBLE_EQ(totalArea<3>(allPieces(boxes)), 14);
}

// A 2D ring of four rectangles: the surface is its outer and inner
// boundaries.
TEST(GaussianSurface, RingHasAnInnerBoundary)
{
  const std::vector<Rect> rects{
      {{0, 0}, {4, 1}}, {{0, 3}, {4, 4}}, {{0, 0}, {1, 4}}, {{3, 0}, {4, 4}}};
  EXPECT_DOUBLE_EQ(totalArea<2>(allPieces(rects)), 16 + 8);
}

// Random boxes on an integer grid: every unit cell of a face is exposed
// whole or not at all, so the cells' centers give the exposed area exactly.
// Each exposed center lies in one piece of its face, and no other center
// lies in any.
template <int D>
void checkAgainstRule(const uint64_t seed)
{
  Philox rng(seed, 0);
  std::vector<AlignedBox<D>> boxes(2 + static_cast<int>(rng.uniform() * 6));
  for (AlignedBox<D>& box : boxes) {
    for (int k = 0; k < D; k++) {
      box.lo[k] = std::floor(rng.uniform() * 5);
      box.hi[k] = box.lo[k] + 1 + std::floor(rng.uniform() * 3);
    }
  }
  const SpatialIndex<D> index(boxes);
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    for (int axis = 0; axis < D; axis++) {
      for (const bool high : {false, true}) {
        const Pieces<D> pieces = exposedPieces(boxes, index, b, axis, high);
        const AlignedBox<D>& box = boxes[b];
        std::array<int, D - 1> tangent;
        for (int t = 0, n = 0; t < D; t++) {
          if (t != axis) {
            tangent[n++] = t;
          }
        }
        const int rows = D == 3 ? static_cast<int>(box.hi[tangent[D - 2]]
                                                   - box.lo[tangent[D - 2]])
                                : 1;
        const int cells
            = static_cast<int>(box.hi[tangent[0]] - box.lo[tangent[0]]);
        int exposed = 0;
        for (int r = 0; r < rows; r++) {
          for (int i = 0; i < cells; i++) {
            Vec<D> q;
            q[axis] = high ? box.hi[axis] : box.lo[axis];
            q[tangent[0]] = box.lo[tangent[0]] + i + 0.5;
            if constexpr (D == 3) {
              q[tangent[1]] = box.lo[tangent[1]] + r + 0.5;
            }
            int in = 0;
            for (const auto& piece : pieces) {
              bool inside = piece.axis == axis && piece.high == high
                            && piece.rect.lo[axis] == q[axis];
              for (int n = 0; inside && n < D - 1; n++) {
                const int t = tangent[n];
                inside = piece.rect.lo[t] < q[t] && q[t] < piece.rect.hi[t];
              }
              in += inside ? 1 : 0;
            }
            const bool rule = exposedAt<D>(boxes, b, axis, high, q);
            ASSERT_EQ(in, rule ? 1 : 0)
                << "seed " << seed << " box " << b << " axis " << axis
                << " high " << high << " cell " << i << "," << r;
            exposed += rule ? 1 : 0;
          }
        }
        double sum = 0;
        for (const auto& piece : pieces) {
          sum += piece.area();
        }
        ASSERT_DOUBLE_EQ(sum, exposed) << "seed " << seed;
      }
    }
  }
}

// The boundary area of a union of boxes on the integer grid, counted
// independently of the rule: unit faces between a cell inside the union and
// a neighbouring cell outside it.
template <int D>
double gridBoundaryArea(const std::vector<AlignedBox<D>>& boxes)
{
  constexpr int kLo = -1;
  constexpr int kHi = 9;  // past every box of checkUnionBoundary
  auto inside = [&](const std::array<int, D>& cell) {
    for (const AlignedBox<D>& box : boxes) {
      bool in = true;
      for (int k = 0; in && k < D; k++) {
        in = box.lo[k] < cell[k] + 0.5 && cell[k] + 0.5 < box.hi[k];
      }
      if (in) {
        return true;
      }
    }
    return false;
  };
  double area = 0;
  std::array<int, D> cell;
  cell.fill(kLo);
  while (true) {
    for (int a = 0; a < D; a++) {
      std::array<int, D> next = cell;
      next[a]++;
      if (next[a] < kHi && inside(cell) != inside(next)) {
        area += 1;
      }
    }
    int k = 0;
    while (k < D && ++cell[k] == kHi) {
      cell[k++] = kLo;
    }
    if (k == D) {
      return area;
    }
  }
}

// The pieces of all faces cover the boundary of the union exactly once.
template <int D>
void checkUnionBoundary(const uint64_t seed)
{
  Philox rng(seed, 1);
  std::vector<AlignedBox<D>> boxes(2 + static_cast<int>(rng.uniform() * 6));
  for (AlignedBox<D>& box : boxes) {
    for (int k = 0; k < D; k++) {
      box.lo[k] = std::floor(rng.uniform() * 5);
      box.hi[k] = box.lo[k] + 1 + std::floor(rng.uniform() * 3);
    }
  }
  ASSERT_DOUBLE_EQ(totalArea<D>(allPieces(boxes)), gridBoundaryArea(boxes))
      << "seed " << seed;
}

TEST(GaussianSurface, CoversTheUnionBoundaryOnce)
{
  for (uint64_t seed = 1; seed <= 300; seed++) {
    checkUnionBoundary<3>(seed);
    checkUnionBoundary<2>(seed);
  }
}

TEST(GaussianSurface, MatchesThePointRule3D)
{
  for (uint64_t seed = 1; seed <= 300; seed++) {
    checkAgainstRule<3>(seed);
  }
}

TEST(GaussianSurface, MatchesThePointRule2D)
{
  for (uint64_t seed = 1; seed <= 300; seed++) {
    checkAgainstRule<2>(seed);
  }
}

// Each piece's start probability times its scale is its area, so a walk's
// expected weight is the same as with start points uniform by area.
// Pieces where the first cube is smaller are drawn more often per unit area:
// a net of a wide box and a thin one, whose offset is smaller.
TEST(GaussianSurface, StartProbabilityTimesScaleIsArea)
{
  const std::vector<Box> boxes{{{0, 0, 1}, {4, 4, 2}},
                               {{4, 1.8, 1.4}, {8, 2.2, 1.6}}};
  const SpatialIndex<3> conductors(boxes);
  const Dielectric<3> dielectric(1.0, {}, {});
  const GaussianSurface<3> g = makeGaussianSurface<3>(
      boxes, [](int) { return 0; }, conductors, dielectric, 0, 0.5);
  const double total = g.cumulative.back();
  double previous = 0;
  double wide_density = 0;  // the wide box's face at low x
  double thin_density = 0;  // the thin box's face at high x
  for (size_t i = 0; i < g.pieces.size(); i++) {
    const auto& piece = g.pieces[i];
    const double probability = (g.cumulative[i] - previous) / total;
    previous = g.cumulative[i];
    EXPECT_NEAR(probability * piece.scale, piece.area(), 1e-12);
    if (piece.axis == 0) {
      (piece.high ? thin_density : wide_density) = probability / piece.area();
    }
  }
  // The offsets are 0.5 and 0.1, so the first cubes differ by 5x.
  EXPECT_NEAR(thin_density / wide_density, 5, 1e-9);
}

// A grid point where two interfaces cross has a first cube of size 0: here a
// planar interface on the surface's top face (z = 2.5, the box's top plus
// its offset of 0.5) and a shell face across it at x = 1.375, the second
// grid point along x. Every piece still gets a finite, positive scale.
TEST(GaussianSurface, InterfacesCrossingAtAGridPoint)
{
  const std::vector<Box> boxes{{{0, 0, 1}, {4, 1, 2}}};
  const SpatialIndex<3> conductors(boxes);
  Shell<3> shell{};
  shell.outer = {{1.375, -1, 2}, {3, 2, 3}};
  shell.inner = shell.outer;
  shell.eps = 7;
  shell.depth = 1;
  const Dielectric<3> dielectric(
      3.9, {{.z = 2.5, .eps_below = 3.9, .eps_above = 4.5}}, {shell});
  const GaussianSurface<3> g = makeGaussianSurface<3>(
      boxes, [](int) { return 0; }, conductors, dielectric, 0, 0.5);
  ASSERT_DOUBLE_EQ(firstCubeSize<3>({1.375, 0.25, 2.5}, conductors, dielectric),
                   0);
  for (const auto& piece : g.pieces) {
    EXPECT_TRUE(std::isfinite(piece.scale) && piece.scale > 0);
  }
}

// Inside its own shells a box grows by 0.4 of the distance to the nearest
// parallel face of them, so a first cube stays short of it; a neighbour's
// shell does not move it, nor does a face so near the box that the offset
// would be under 0.05 of the usual one. A lone box in one dielectric has first
// cubes of exactly its offset.
TEST(GaussianSurface, InsidePlacementStaysShortOfShellFaces)
{
  const std::vector<Box> boxes{{{0, 0, 1}, {4, 1, 2}}};
  const SpatialIndex<3> conductors(boxes);
  const auto own = [](int) { return 0; };
  Shell<3> shell{};
  shell.outer = {{-0.15, -0.15, 0.85}, {4.15, 1.15, 2.15}};
  shell.inner = boxes[0];
  shell.eps = 7;
  shell.depth = 1;
  const Dielectric<3> shelled(3.9, {}, {shell});
  const GaussianSurface<3> inside = makeGaussianSurface<3>(
      boxes, own, conductors, shelled, 0, 0.5, SurfacePlacement::kInsideShells);
  EXPECT_TRUE(inside.inside_shells);
  for (const auto& piece : inside.pieces) {
    const double c = piece.high ? boxes[0].hi[piece.axis] + 0.06
                                : boxes[0].lo[piece.axis] - 0.06;
    EXPECT_NEAR(piece.rect.lo[piece.axis], c, 1e-12);
  }
  // A neighbour's shell alone leaves the box where it is.
  const std::vector<Box> pair{boxes[0], {{0, 1.2, 1}, {4, 2.2, 2}}};
  Shell<3> neighbour = shell;
  neighbour.outer = {{-0.05, 1.15, 0.95}, {4.05, 2.25, 2.05}};
  neighbour.inner = pair[1];
  neighbour.box = 1;
  const GaussianSurface<3> beside = makeGaussianSurface<3>(
      pair,
      [](const int b) { return b; },
      SpatialIndex<3>(pair),
      Dielectric<3>(3.9, {}, {neighbour}),
      0,
      0.5,
      SurfacePlacement::kInsideShells);
  EXPECT_FALSE(beside.inside_shells);

  // A shell face a rounding error from the box leaves it where it is.
  Shell<3> touching = shell;
  touching.outer
      = {{-1e-15, -1e-15, 1 - 1e-15}, {4 + 1e-15, 1 + 1e-15, 2 + 1e-15}};
  const GaussianSurface<3> thin
      = makeGaussianSurface<3>(boxes,
                               own,
                               conductors,
                               Dielectric<3>(3.9, {}, {touching}),
                               0,
                               0.5,
                               SurfacePlacement::kInsideShells);
  EXPECT_FALSE(thin.inside_shells);

  // Two fingers of the master: the second finger's shell comes nearer the
  // first (0.15) than the first's own shell face (0.2), and only the own
  // one places the first finger, at 0.4 * 0.2.
  const std::vector<Box> fingers{boxes[0], {{0, 1.5, 1}, {4, 2.5, 2}}};
  Shell<3> first = shell;
  first.outer = {{-0.2, -0.2, 1}, {4.2, 1.2, 2.2}};
  first.box = 0;
  Shell<3> second = shell;
  second.outer = {{-0.35, 1.15, 1}, {4.35, 2.85, 2.35}};
  second.inner = fingers[1];
  second.box = 1;
  const GaussianSurface<3> pair_inside
      = makeGaussianSurface<3>(fingers,
                               own,
                               SpatialIndex<3>(fingers),
                               Dielectric<3>(3.9, {}, {first, second}),
                               0,
                               0.5,
                               SurfacePlacement::kInsideShells);
  bool found = false;
  for (const auto& piece : pair_inside.pieces) {
    if (piece.axis == 0 && !piece.high && piece.rect.hi[1] < 1.5) {
      EXPECT_NEAR(piece.rect.lo[0], -0.08, 1e-12);
      found = true;
    }
  }
  EXPECT_TRUE(found);

  const Dielectric<3> uniform(3.9, {}, {});
  const GaussianSurface<3> lone = makeGaussianSurface<3>(
      boxes, own, conductors, uniform, 0, 0.5, SurfacePlacement::kInsideShells);
  EXPECT_FALSE(lone.inside_shells);
  EXPECT_NEAR(lone.first_cube_shrink, 1, 1e-12);
}

}  // namespace
}  // namespace fsv
