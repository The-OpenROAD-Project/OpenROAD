// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "GaussianSurface.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "Dielectric.h"
#include "SpatialIndex.h"
#include "fsv/Geometry.h"

namespace fsv {

namespace {

// The smallest first cube, as a fraction of the box's offset, that start
// probabilities count.
constexpr double kMinCubeFraction = 1e-3;

// Inside the shells, the offset as a fraction of the distance to the
// nearest parallel face of the master's own shells. Tuned on the benchmark
// (see test/README.md, Surface placement).
constexpr double kInsideFraction = 0.4;
// The least inside offset, as a share of the usual one. A shell of nearly
// no thickness would otherwise leave no room for a first cube; its box
// keeps the usual offset.
constexpr double kMinInsideShare = 0.05;

// Coordinates closer than this (µm) are equal when boxes merge. A merged box
// only shapes the Gaussian surface, which stays valid as long as it
// contains the master's boxes and clears other conductors, so the
// tolerance cannot make the surface wrong; it lets pieces computed in
// floating point merge.
constexpr double kMergeTolerance = 1e-9;

// Whether two boxes are equal on every axis but k.
template <int D>
bool equalExcept(const AlignedBox<D>& a, const AlignedBox<D>& b, const int k)
{
  for (int t = 0; t < D; t++) {
    if (t != k
        && (std::abs(a.lo[t] - b.lo[t]) > kMergeTolerance
            || std::abs(a.hi[t] - b.hi[t]) > kMergeTolerance)) {
      return false;
    }
  }
  return true;
}

template <int D>
bool contains(const AlignedBox<D>& outer, const AlignedBox<D>& inner)
{
  for (int k = 0; k < D; k++) {
    if (inner.lo[k] < outer.lo[k] || inner.hi[k] > outer.hi[k]) {
      return false;
    }
  }
  return true;
}

// The offset by which one master box grows: a fraction of its smallest
// dimension, at most half the gap to the nearest other conductor or the
// ground plane.
template <int D>
double boxOffset(const AlignedBox<D>& box,
                 const std::vector<AlignedBox<D>>& boxes,
                 const std::function<int(int)>& box_net,
                 const Dielectric<D>& dielectric,
                 const int master,
                 const double offset_frac,
                 const SurfacePlacement placement,
                 bool& inside)
{
  double gap = box.lo[D - 1];
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    if (box_net(b) != master) {
      gap = std::min(gap, linfGap(box, boxes[b]));
    }
  }
  if (gap <= 0) {
    throw std::invalid_argument("fsv: another conductor touches the master");
  }
  double min_dim = std::numeric_limits<double>::max();
  for (int k = 0; k < D; k++) {
    min_dim = std::min(min_dim, box.hi[k] - box.lo[k]);
  }
  const double limit = 0.5 * gap;
  double offset = std::min(limit, offset_frac * min_dim);
  // Keep the surface clear of every shell face parallel to it nearby, so
  // that first hops near it never meet two interfaces at once (their
  // weight grows as the cube shrinks). Each face is an offset to avoid by a
  // clearance of a quarter of the offset. If the offset is too close to one,
  // move it to the nearest point clear of them all within (0, limit], or
  // into the middle of the widest gap between them.
  std::vector<double> faces;
  // The nearest outer face of the shells wrapping this box: those around the
  // master's boxes merged into it. Another master box's shell nearby (a via
  // sitting in a wire's shell, a parallel finger) does not move it.
  double own_face = std::numeric_limits<double>::infinity();
  for (const Shell<D>& shell : dielectric.shells()) {
    if (linfGap(box, shell.outer) >= limit) {
      continue;
    }
    const bool own
        = box_net(shell.box) == master && contains(box, boxes[shell.box]);
    for (int k = 0; k < D; k++) {
      for (const double c : {shell.outer.lo[k], shell.outer.hi[k]}) {
        for (const double d : {box.lo[k] - c, c - box.hi[k]}) {
          if (d > 0 && d < limit) {
            faces.push_back(d);
            if (own) {
              own_face = std::min(own_face, d);
            }
          }
        }
      }
    }
  }
  std::ranges::sort(faces);
  const double inside_offset = kInsideFraction * own_face;
  if (placement == SurfacePlacement::kInsideShells && std::isfinite(own_face)
      && inside_offset >= kMinInsideShare * offset) {
    // The first cube reaches twice the offset from the box, so it stays
    // short of the face.
    inside = true;
    return std::min(offset, inside_offset);
  }
  // Moving the offset changes its clearance, so repeat until it holds.
  for (int attempt = 0; attempt < 4; attempt++) {
    const double clearance = 0.25 * offset;
    if (std::ranges::none_of(faces, [&](const double d) {
          return std::abs(d - offset) < clearance;
        })) {
      break;
    }
    // Gaps between consecutive faces, bounded by 0 and the limit, which are
    // not faces.
    double best = offset;
    double best_move = std::numeric_limits<double>::infinity();
    double widest = -1;
    double widest_mid = offset;
    for (size_t i = 0; i <= faces.size(); i++) {
      const double gap_lo = i == 0 ? 0 : faces[i - 1];
      const double gap_hi = i == faces.size() ? limit : faces[i];
      const double lo = i == 0 ? 0 : gap_lo + clearance;
      const double hi = i == faces.size() ? limit : gap_hi - clearance;
      if (gap_hi - gap_lo > widest) {
        widest = gap_hi - gap_lo;
        widest_mid = 0.5 * (gap_lo + gap_hi);
      }
      if (!(hi > lo)) {
        continue;
      }
      const double candidate = std::clamp(offset, lo, hi);
      if (candidate > 0 && std::abs(candidate - offset) < best_move) {
        best_move = std::abs(candidate - offset);
        best = candidate;
      }
    }
    offset = std::isfinite(best_move) ? best : widest_mid;
  }

  return offset;
}

}  // namespace

template <int D>
std::vector<AlignedBox<D>> mergeBoxes(std::vector<AlignedBox<D>> boxes)
{
  for (bool changed = true; changed;) {
    changed = false;
    for (int k = 0; k < D; k++) {
      // Boxes equal on the other axes become adjacent, in order along k.
      std::ranges::sort(boxes, {}, [k](const AlignedBox<D>& b) {
        std::array<double, 2 * D> key;
        int n = 0;
        for (int t = 0; t < D; t++) {
          if (t != k) {
            key[n++] = b.lo[t];
            key[n++] = b.hi[t];
          }
        }
        key[n++] = b.lo[k];
        key[n++] = b.hi[k];
        return key;
      });
      std::vector<AlignedBox<D>> merged;
      for (const AlignedBox<D>& b : boxes) {
        AlignedBox<D>* last = merged.empty() ? nullptr : &merged.back();
        if (last != nullptr && equalExcept(*last, b, k)
            && b.lo[k] <= last->hi[k] + kMergeTolerance) {
          // The hull, which contains both.
          for (int t = 0; t < D; t++) {
            last->lo[t] = std::min(last->lo[t], b.lo[t]);
            last->hi[t] = std::max(last->hi[t], b.hi[t]);
          }
        } else {
          merged.push_back(b);
        }
      }
      changed = changed || merged.size() < boxes.size();
      boxes = std::move(merged);
    }
    std::vector<AlignedBox<D>> kept;
    for (size_t i = 0; i < boxes.size(); i++) {
      bool inside = false;
      for (size_t j = 0; j < boxes.size() && !inside; j++) {
        inside = j != i && contains(boxes[j], boxes[i]);
      }
      if (!inside) {
        kept.push_back(boxes[i]);
      }
    }
    changed = changed || kept.size() < boxes.size();
    boxes = std::move(kept);
  }
  return boxes;
}

template <int D>
std::vector<typename GaussianSurface<D>::Piece> exposedPieces(
    const std::vector<AlignedBox<D>>& boxes,
    const SpatialIndex<D>& index,
    const int b,
    const int axis,
    const bool high)
{
  constexpr int kT = D - 1;  // tangent axes
  const AlignedBox<D>& box = boxes[b];
  const double x = high ? box.hi[axis] : box.lo[axis];
  AlignedBox<D> face = box;
  face.lo[axis] = x;
  face.hi[axis] = x;
  std::array<int, kT> tangent;
  for (int t = 0, n = 0; t < D; t++) {
    if (t != axis) {
      tangent[n++] = t;
    }
  }
  // The boxes covering the face, over their open tangent extents.
  std::vector<int> covers;
  index.forEachOverlapping(face, [&](const int c) {
    const AlignedBox<D>& e = boxes[c];
    const bool inside = e.lo[axis] < x && x < e.hi[axis];
    const bool continues = (high ? e.lo[axis] : e.hi[axis]) == x;
    const bool lower_coplanar = c < b && (high ? e.hi[axis] : e.lo[axis]) == x;
    if (c != b && (inside || continues || lower_coplanar)) {
      covers.push_back(c);
    }
  });
  // Cut the face at the covering boxes' tangent bounds. Each cell between
  // the cuts is then covered whole, or not at all.
  std::array<std::vector<double>, kT> cuts;
  for (int n = 0; n < kT; n++) {
    const int t = tangent[n];
    cuts[n] = {face.lo[t], face.hi[t]};
    for (const int c : covers) {
      for (const double v : {boxes[c].lo[t], boxes[c].hi[t]}) {
        if (v > face.lo[t] && v < face.hi[t]) {
          cuts[n].push_back(v);
        }
      }
    }
    std::ranges::sort(cuts[n]);
    cuts[n].erase(std::unique(cuts[n].begin(), cuts[n].end()), cuts[n].end());
  }
  auto covered = [&](const std::array<double, kT>& mid) {
    return std::ranges::any_of(covers, [&](const int c) {
      for (int n = 0; n < kT; n++) {
        const int t = tangent[n];
        if (!(boxes[c].lo[t] < mid[n] && mid[n] < boxes[c].hi[t])) {
          return false;
        }
      }
      return true;
    });
  };
  // Rows along the second tangent axis (one row in 2D), each cut into runs
  // of uncovered cells along the first.
  std::vector<typename GaussianSurface<D>::Piece> pieces;
  const int rows = kT == 2 ? static_cast<int>(cuts[kT - 1].size()) - 1 : 1;
  const int cells = static_cast<int>(cuts[0].size()) - 1;
  for (int r = 0; r < rows; r++) {
    std::array<double, kT> mid;
    if constexpr (kT == 2) {
      mid[1] = 0.5 * (cuts[1][r] + cuts[1][r + 1]);
    }
    for (int i = 0; i < cells;) {
      mid[0] = 0.5 * (cuts[0][i] + cuts[0][i + 1]);
      if (covered(mid)) {
        i++;
        continue;
      }
      int end = i + 1;
      for (; end < cells; end++) {
        mid[0] = 0.5 * (cuts[0][end] + cuts[0][end + 1]);
        if (covered(mid)) {
          break;
        }
      }
      AlignedBox<D> rect = face;
      rect.lo[tangent[0]] = cuts[0][i];
      rect.hi[tangent[0]] = cuts[0][end];
      if constexpr (kT == 2) {
        rect.lo[tangent[1]] = cuts[1][r];
        rect.hi[tangent[1]] = cuts[1][r + 1];
      }
      pieces.push_back({rect, axis, high});
      i = end;
    }
  }
  return pieces;
}

template <int D>
double firstCubeSize(const std::type_identity_t<Vec<D>>& p,
                     const SpatialIndex<D>& conductors,
                     const Dielectric<D>& dielectric)
{
  return dielectric.cube(p, conductors.nearest(p).dist).half_size;
}

template <int D>
GaussianSurface<D> makeGaussianSurface(const std::vector<AlignedBox<D>>& boxes,
                                       const std::function<int(int)>& box_net,
                                       const SpatialIndex<D>& conductors,
                                       const Dielectric<D>& dielectric,
                                       const int master,
                                       const double offset_frac,
                                       const SurfacePlacement placement)
{
  std::vector<AlignedBox<D>> own;
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    if (box_net(b) == master) {
      own.push_back(boxes[b]);
    }
  }
  std::vector<AlignedBox<D>> grown;
  std::vector<double> offsets;
  bool inside_shells = false;
  for (const AlignedBox<D>& box : mergeBoxes(std::move(own))) {
    const double offset = boxOffset(box,
                                    boxes,
                                    box_net,
                                    dielectric,
                                    master,
                                    offset_frac,
                                    placement,
                                    inside_shells);
    offsets.push_back(offset);
    AlignedBox<D>& g = grown.emplace_back();
    for (int k = 0; k < D; k++) {
      g.lo[k] = box.lo[k] - offset;
      g.hi[k] = box.hi[k] + offset;
    }
  }
  if (grown.empty()) {
    throw std::invalid_argument("fsv: master net has no boxes");
  }
  const SpatialIndex<D> index(grown);
  GaussianSurface<D> g;
  g.inside_shells = inside_shells;
  std::vector<double> areas;
  std::vector<double> draws;
  double shrink_sum = 0;
  for (int b = 0; b < static_cast<int>(grown.size()); b++) {
    for (int axis = 0; axis < D; axis++) {
      for (const bool high : {false, true}) {
        for (const auto& piece : exposedPieces(grown, index, b, axis, high)) {
          // The mean of 1 / h is taken at the centers of a grid of cells.
          // h is 0 where two interfaces cross at a grid point; it is floored
          // at a small part of the offset, which only shifts the
          // probabilities, so the piece is drawn often rather than always.
          const double h_floor = kMinCubeFraction * offsets[b];
          const double area = piece.area();
          constexpr int kGrid = 4;  // cells per tangent axis
          constexpr int kCells = D == 3 ? kGrid * kGrid : kGrid;
          double inverse_sum = 0;
          for (int cell = 0; cell < kCells; cell++) {
            Vec<D> p = piece.rect.lo;
            for (int t = 0, rest = cell; t < D; t++) {
              if (t != axis) {
                const double f = (rest % kGrid + 0.5) / kGrid;
                p[t] += f * (piece.rect.hi[t] - piece.rect.lo[t]);
                rest /= kGrid;
              }
            }
            inverse_sum
                += 1
                   / std::max(h_floor,
                              firstCubeSize<D>(p, conductors, dielectric));
          }
          areas.push_back(area);
          draws.push_back(area * inverse_sum / kCells);
          shrink_sum += draws.back() * offsets[b];
          g.pieces.push_back(piece);
        }
      }
    }
  }
  double area_sum = 0;
  for (const double area : areas) {
    area_sum += area;
  }
  g.first_cube_shrink = shrink_sum / area_sum;
  double total = 0;
  for (const double draw : draws) {
    total += draw;
    g.cumulative.push_back(total);
  }
  for (size_t i = 0; i < g.pieces.size(); i++) {
    g.pieces[i].scale = areas[i] * total / draws[i];
  }
  return g;
}

#define FSV_INSTANTIATE(D)                                                    \
  template std::vector<AlignedBox<D>> mergeBoxes(std::vector<AlignedBox<D>>); \
  template std::vector<GaussianSurface<D>::Piece> exposedPieces(              \
      const std::vector<AlignedBox<D>>&,                                      \
      const SpatialIndex<D>&,                                                 \
      int,                                                                    \
      int,                                                                    \
      bool);                                                                  \
  template GaussianSurface<D> makeGaussianSurface(                            \
      const std::vector<AlignedBox<D>>&,                                      \
      const std::function<int(int)>&,                                         \
      const SpatialIndex<D>&,                                                 \
      const Dielectric<D>&,                                                   \
      int,                                                                    \
      double,                                                                 \
      SurfacePlacement);                                                      \
  template double firstCubeSize<D>(                                           \
      const Vec<D>&, const SpatialIndex<D>&, const Dielectric<D>&);
FSV_INSTANTIATE(2)
FSV_INSTANTIATE(3)
#undef FSV_INSTANTIATE

}  // namespace fsv
