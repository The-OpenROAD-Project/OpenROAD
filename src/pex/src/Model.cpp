// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Model.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pex {

namespace {

// A grid of a table: non-empty, strictly ascending, finite, and positive
// when it must be.
void requireGrid(const char* name,
                 const std::vector<double>& grid,
                 const bool positive)
{
  if (grid.empty()) {
    throw std::invalid_argument(std::string(name) + " grid is empty");
  }
  for (size_t i = 0; i < grid.size(); i++) {
    if (!std::isfinite(grid[i]) || (positive && grid[i] <= 0)) {
      throw std::invalid_argument(std::string(name) + " grid has a value "
                                  + std::to_string(grid[i]));
    }
    if (i > 0 && grid[i] <= grid[i - 1]) {
      throw std::invalid_argument(std::string(name) + " grid is not ascending");
    }
  }
}

void requireFinite(const char* name, const double v)
{
  if (!std::isfinite(v)) {
    throw std::invalid_argument(std::string(name) + " value is not finite");
  }
}

void requireIndex(const char* name, const int i, const int count)
{
  if (i < 0 || i >= count) {
    throw std::invalid_argument(std::string(name) + " index "
                                + std::to_string(i) + " is off the grid");
  }
}

}  // namespace

LateralTable::LateralTable(std::vector<double> widths,
                           std::vector<double> spacings)
    : widths_(std::move(widths)), spacings_(std::move(spacings))
{
  requireGrid("lateral width", widths_, true);
  requireGrid("lateral spacing", spacings_, true);
  const int n = spacingCount();
  values_.assign(widths_.size() * n * n, LateralCap{});
}

int LateralTable::index(const int width, const int left, const int right) const
{
  const int n = spacingCount();
  return (width * n + left) * n + right;
}

void LateralTable::set(const int width,
                       const int left,
                       const int right,
                       const LateralCap& cap)
{
  requireIndex("lateral width", width, widthCount());
  requireIndex("lateral left spacing", left, spacingCount());
  requireIndex("lateral right spacing", right, spacingCount());
  for (const double v :
       {cap.ground, cap.left, cap.right, cap.below, cap.above}) {
    requireFinite("lateral", v);
  }
  values_[index(width, left, right)] = cap;
  if (left != right) {
    LateralCap mirror = cap;
    std::swap(mirror.left, mirror.right);
    values_[index(width, right, left)] = mirror;
  }
}

CrossingTable::CrossingTable(std::vector<double> gaps,
                             std::vector<CrossingShare> values,
                             std::vector<double> spacings)
    : gaps_(std::move(gaps)),
      spacings_(std::move(spacings)),
      values_(std::move(values))
{
  requireGrid("crossing gap", gaps_, true);
  if (!spacings_.empty()) {
    requireGrid("crossing spacing", spacings_, true);
  }
  if (values_.size() != gaps_.size() * spacingCount()) {
    throw std::invalid_argument(
        "crossing table has " + std::to_string(values_.size()) + " values for "
        + std::to_string(gaps_.size()) + " gaps and "
        + std::to_string(spacingCount()) + " spacing columns");
  }
  for (const CrossingShare& s : values_) {
    requireFinite("crossing", s.total);
    requireFinite("crossing", s.cross);
  }
}

void CrossingTable::setReach(CrossingReach reach)
{
  if (!(reach.width > 0) || !std::isfinite(reach.width)) {
    throw std::invalid_argument(
        "crossing reach needs a finite, positive width");
  }
  requireGrid("crossing reach past", reach.past, false);
  requireGrid("crossing reach extent", reach.extents, false);
  if (reach.past.front() != 0 || reach.extents.front() != 0) {
    throw std::invalid_argument("crossing reach grids start at 0");
  }
  if (reach.values.size() != reach.past.size() * reach.extents.size()) {
    throw std::invalid_argument(
        "crossing reach has " + std::to_string(reach.values.size())
        + " values for " + std::to_string(reach.past.size()) + " x "
        + std::to_string(reach.extents.size()));
  }
  for (const CrossingShare& v : reach.values) {
    requireFinite("crossing reach", v.total);
    requireFinite("crossing reach", v.cross);
  }
  reach_ = std::move(reach);
}

DiagonalTable::DiagonalTable(std::vector<double> widths,
                             std::vector<double> gaps,
                             std::vector<double> spacings,
                             std::vector<double> far_spacings)
    : widths_(std::move(widths)),
      gaps_(std::move(gaps)),
      spacings_(std::move(spacings)),
      far_spacings_(std::move(far_spacings))
{
  requireGrid("diagonal width", widths_, true);
  requireGrid("diagonal gap", gaps_, false);
  requireGrid("diagonal spacing", spacings_, true);
  requireGrid("diagonal far spacing", far_spacings_, true);
  values_.assign(widths_.size() * gaps_.size() * spacingCount() * farCount(),
                 DiagonalCap{});
}

int DiagonalTable::index(const int width,
                         const int gap,
                         const int spacing,
                         const int far) const
{
  return ((width * gapCount() + gap) * spacingCount() + spacing) * farCount()
         + far;
}

void DiagonalTable::set(const int width,
                        const int gap,
                        const int spacing,
                        const int far,
                        const DiagonalCap& cap)
{
  requireIndex("diagonal width", width, widthCount());
  requireIndex("diagonal gap", gap, gapCount());
  requireIndex("diagonal spacing", spacing, spacingCount());
  requireIndex("diagonal far spacing", far, farCount());
  for (const double v :
       {cap.coupling, cap.total_delta, cap.lateral_delta, cap.plane_delta}) {
    requireFinite("diagonal", v);
  }
  values_[index(width, gap, spacing, far)] = cap;
}

EndTable::EndTable(std::vector<double> widths,
                   std::vector<double> spacings,
                   std::vector<double> gaps)
    : widths_(std::move(widths)),
      spacings_(std::move(spacings)),
      gaps_(std::move(gaps))
{
  requireGrid("end width", widths_, true);
  if (!spacings_.empty()) {
    requireGrid("end spacing", spacings_, true);
  }
  if (!gaps_.empty()) {
    requireGrid("end gap", gaps_, true);
  }
  values_.resize(widths_.size() * spacingCount() * gapCount());
}

int EndTable::index(const int width, const int spacing, const int gap) const
{
  return (((width * spacingCount()) + spacing) * gapCount()) + gap;
}

void EndTable::set(const int width,
                   const int spacing,
                   const int gap,
                   const EndValue& value)
{
  requireIndex("end width", width, static_cast<int>(widths_.size()));
  requireIndex("end spacing", spacing, spacingCount());
  requireIndex("end gap", gap, gapCount());
  for (const double v :
       {value.total, value.below, value.above, value.lateral, value.facing}) {
    requireFinite("end", v);
  }
  values_[index(width, spacing, gap)] = value;
}

EndSides EndTable::sides(const int width, const int left, const int right) const
{
  if (left == right) {
    const EndValue& v = at(width, left, gapCount() - 1);
    return {v.total, v.below, v.above, v.lateral / 2, v.lateral / 2};
  }
  return sides_.at(sideIndex(width, left, right));
}

int EndTable::sideIndex(const int width, const int left, const int right) const
{
  const int ns = spacingCount();
  return (((width * ns) + left) * ns) + right;
}

void EndTable::setSides(const int width,
                        const int left,
                        const int right,
                        const EndSides& value)
{
  requireIndex("end width", width, static_cast<int>(widths_.size()));
  requireIndex("end spacing", left, spacingCount());
  requireIndex("end spacing", right, spacingCount());
  if (left >= right) {
    throw std::invalid_argument("end sides need the left spacing first");
  }
  for (const double v :
       {value.total, value.below, value.above, value.left, value.right}) {
    requireFinite("end", v);
  }
  if (sides_.empty()) {
    sides_.resize(widths_.size() * spacingCount() * spacingCount());
  }
  sides_[sideIndex(width, left, right)] = value;
  sides_[sideIndex(width, right, left)]
      = {value.total, value.below, value.above, value.right, value.left};
}

void EndTable::setCorners(const int width, const Corners& value)
{
  requireIndex("end width", width, static_cast<int>(widths_.size()));
  for (const CornerValue& c : {value.inner, value.outer}) {
    for (const double v : {c.total, c.below, c.above}) {
      requireFinite("corner", v);
    }
  }
  if (corners_.empty()) {
    corners_.resize(widths_.size());
  }
  corners_[width] = value;
}

LateralTable& Model::addLateral(const LateralKey& key, LateralTable table)
{
  return laterals_.insert_or_assign(key, std::move(table)).first->second;
}

const LateralTable* Model::lateral(const LateralKey& key) const
{
  const auto it = laterals_.find(key);
  return it == laterals_.end() ? nullptr : &it->second;
}

namespace {

// value = (1 - t) * v[lo] + t * v[hi]
struct Bracket
{
  int lo;
  int hi;
  double t;
};

// The grid interval around x, interpolating in the logarithm; x is clamped
// to the grid.
Bracket bracketLog(const std::vector<double>& grid, const double x)
{
  if (grid.empty()) {
    throw std::out_of_range("empty table grid");
  }
  if (x <= grid.front() || grid.size() == 1) {
    return {0, 0, 0};
  }
  if (x >= grid.back()) {
    const int last = static_cast<int>(grid.size()) - 1;
    return {last, last, 0};
  }
  const int hi = static_cast<int>(std::upper_bound(grid.begin(), grid.end(), x)
                                  - grid.begin());
  const int lo = hi - 1;
  const double t = std::log(x / grid[lo]) / std::log(grid[hi] / grid[lo]);
  return {lo, hi, t};
}

// The grid interval around x, interpolating linearly; x is clamped.
Bracket bracketLinear(const std::vector<double>& grid, const double x)
{
  if (grid.empty()) {
    throw std::out_of_range("empty table grid");
  }
  if (x <= grid.front() || grid.size() == 1) {
    return {0, 0, 0};
  }
  if (x >= grid.back()) {
    const int last = static_cast<int>(grid.size()) - 1;
    return {last, last, 0};
  }
  const int hi = static_cast<int>(std::upper_bound(grid.begin(), grid.end(), x)
                                  - grid.begin());
  const int lo = hi - 1;
  return {lo, hi, (x - grid[lo]) / (grid[hi] - grid[lo])};
}

// A spacing beyond the grid, or negative, is no neighbor: the open column.
Bracket bracketSpacing(const std::vector<double>& grid, const double s)
{
  if (s < 0 || grid.empty() || s > grid.back()) {
    const int open = static_cast<int>(grid.size());
    return {open, open, 0};
  }
  return bracketLog(grid, s);
}

}  // namespace

LateralCap Model::lateral(const LateralKey& key,
                          const double width,
                          const double left,
                          const double right) const
{
  const LateralTable* table = lateral(key);
  if (table == nullptr) {
    throw std::out_of_range("no lateral table for layer " + key.layer
                            + " below " + key.below + " above " + key.above);
  }
  const Bracket w = bracketLinear(table->widths(), width);
  // Past the widest wire, the ground and the plane couplings grow with the
  // width along the last segment; the lateral couplings stay.
  Bracket wx = w;
  const int nw = static_cast<int>(table->widths().size());
  if (nw >= 2 && width > table->widths().back()) {
    wx = {nw - 2,
          nw - 1,
          (width - table->widths()[nw - 2])
              / (table->widths()[nw - 1] - table->widths()[nw - 2])};
  }
  const Bracket l = bracketSpacing(table->spacings(), left);
  const Bracket r = bracketSpacing(table->spacings(), right);
  // A coupling interpolates geometrically along its own side's spacing
  // when both ends are positive.
  auto couple = [](const double a, const double b, const double t) {
    if (t == 0 || a <= 0 || b <= 0) {
      return (1 - t) * a + t * b;
    }
    return std::exp((1 - t) * std::log(a) + t * std::log(b));
  };
  // Over the right spacing, then the left, then the width.
  auto overRight = [&](const int wi, const int li) {
    const LateralCap& a = table->at(wi, li, r.lo);
    const LateralCap& b = table->at(wi, li, r.hi);
    LateralCap out;
    out.ground = (1 - r.t) * a.ground + r.t * b.ground;
    out.left = (1 - r.t) * a.left + r.t * b.left;
    out.right = couple(a.right, b.right, r.t);
    out.below = (1 - r.t) * a.below + r.t * b.below;
    out.above = (1 - r.t) * a.above + r.t * b.above;
    return out;
  };
  auto overLeft = [&](const int wi) {
    const LateralCap a = overRight(wi, l.lo);
    const LateralCap b = overRight(wi, l.hi);
    LateralCap out;
    out.ground = (1 - l.t) * a.ground + l.t * b.ground;
    out.left = couple(a.left, b.left, l.t);
    out.right = (1 - l.t) * a.right + l.t * b.right;
    out.below = (1 - l.t) * a.below + l.t * b.below;
    out.above = (1 - l.t) * a.above + l.t * b.above;
    return out;
  };
  const LateralCap a = overLeft(wx.lo);
  const LateralCap b = overLeft(wx.hi);
  LateralCap out;
  out.ground = (1 - wx.t) * a.ground + wx.t * b.ground;
  out.below = (1 - wx.t) * a.below + wx.t * b.below;
  out.above = (1 - wx.t) * a.above + wx.t * b.above;
  const LateralCap c = overLeft(w.lo);
  const LateralCap d = overLeft(w.hi);
  out.left = (1 - w.t) * c.left + w.t * d.left;
  out.right = (1 - w.t) * c.right + w.t * d.right;
  return out;
}

CrossingTable& Model::addCrossing(const CrossingKey& key, CrossingTable table)
{
  return crossings_.insert_or_assign(key, std::move(table)).first->second;
}

const CrossingTable* Model::crossing(const CrossingKey& key) const
{
  const auto it = crossings_.find(key);
  return it == crossings_.end() ? nullptr : &it->second;
}

namespace {

const CrossingTable& requireCrossing(const Model& model, const CrossingKey& key)
{
  const CrossingTable* table = model.crossing(key);
  if (table == nullptr) {
    throw std::out_of_range("no crossing table for layer " + key.layer
                            + " under or over " + key.cover);
  }
  return *table;
}

// The amount of the plane's effect kept over a gap, gap × share, is
// linear between the tabulated gaps (from 0 at a gap of 0) and constant
// past the last. Its slope on each segment is the one-sided share there,
// held within [0, 1]: a table whose kept amount dips from one gap to the
// next (solver noise at a saturated gap) keeps the last amount instead of
// taking any away.
struct Segment
{
  double g0;  // the segment's gaps
  double g1;
  double rate;  // the one-sided share along it
};

// The segment holding d in a spacing's column, with d past the last gap on
// a final segment of rate 0.
Segment segmentAt(const CrossingTable& t,
                  const int column,
                  const double d,
                  const bool cross)
{
  auto value = [&](const int i) {
    const CrossingShare& v = t.at(column, i);
    return cross ? v.cross : v.total;
  };
  double g0 = 0;
  double k0 = 0;
  for (size_t i = 0; i < t.gaps().size(); i++) {
    const double g1 = t.gaps()[i];
    const double k1 = g1 * value(static_cast<int>(i));
    if (d <= g1) {
      return {g0, g1, std::clamp((k1 - k0) / (g1 - g0), 0.0, 1.0)};
    }
    g0 = g1;
    k0 = k1;
  }
  return {g0, std::numeric_limits<double>::infinity(), 0};
}

double kept(const CrossingTable& t,
            const int column,
            const double d,
            const bool cross)
{
  double sum = 0;
  double g0 = 0;
  while (g0 < d) {
    const Segment s = segmentAt(t, column, std::nextafter(g0, d), cross);
    const double g1 = std::min(s.g1, d);
    sum += s.rate * (g1 - g0);
    g0 = g1;
  }
  return sum;
}

// A per-column value over the two sides' spacings: each side half, from
// its spacing's column, interpolated in the spacing's logarithm.
template <class F>
CrossingShare overSides(const CrossingTable& t,
                        const double left,
                        const double right,
                        F&& column)
{
  auto side = [&](const double s) {
    const Bracket b = bracketSpacing(t.spacings(), s);
    const CrossingShare lo = column(b.lo);
    const CrossingShare hi = b.hi == b.lo ? lo : column(b.hi);
    return CrossingShare{(1 - b.t) * lo.total + b.t * hi.total,
                         (1 - b.t) * lo.cross + b.t * hi.cross};
  };
  const CrossingShare l = side(left);
  const CrossingShare r = left == right ? l : side(right);
  return {(l.total + r.total) / 2, (l.cross + r.cross) / 2};
}

}  // namespace

CrossingShare Model::crossingKept(const CrossingKey& key,
                                  const double d,
                                  const double left,
                                  const double right) const
{
  const CrossingTable& t = requireCrossing(*this, key);
  if (d <= 0) {
    return {0, 0};
  }
  return overSides(t, left, right, [&](const int c) {
    return CrossingShare{kept(t, c, d, false), kept(t, c, d, true)};
  });
}

CrossingShare Model::crossing(const CrossingKey& key,
                              const double gap,
                              const double left,
                              const double right) const
{
  if (gap <= 0) {
    requireCrossing(*this, key);
    return {1, 1};
  }
  const CrossingShare k = crossingKept(key, gap, left, right);
  return {k.total / gap, k.cross / gap};
}

CrossingShare Model::crossingOneSided(const CrossingKey& key,
                                      const double d,
                                      const double left,
                                      const double right) const
{
  const CrossingTable& t = requireCrossing(*this, key);
  if (d <= 0) {
    return {1, 1};
  }
  return overSides(t, left, right, [&](const int c) {
    return CrossingShare{segmentAt(t, c, d, false).rate,
                         segmentAt(t, c, d, true).rate};
  });
}

CrossingShare Model::crossingReach(const CrossingKey& key,
                                   const double from,
                                   const double to,
                                   const double extent_a,
                                   const double extent_b,
                                   const double left,
                                   const double right) const
{
  const CrossingTable* t = &requireCrossing(*this, key);
  if (!t->hasReach() && !key.opposite.empty()) {
    if (const CrossingTable* plain = crossing({key.layer, key.cover, ""})) {
      t = plain;
    }
  }
  if (!t->hasReach()) {
    return {1, 1};
  }
  const CrossingReach& r = t->reach();
  auto ratio = [&](const double d, const double e) {
    if (e >= r.extents.back()) {
      return CrossingShare{1, 1};
    }
    const Bracket p = bracketLinear(r.past, d);
    const Bracket x = bracketLinear(r.extents, std::max(0.0, e));
    CrossingShare out;
    for (const auto& [pi, pw] :
         {std::pair{p.lo, 1 - p.t}, std::pair{p.hi, p.t}}) {
      for (const auto& [xi, xw] :
           {std::pair{x.lo, 1 - x.t}, std::pair{x.hi, x.t}}) {
        out.total += pw * xw * r.at(pi, xi).total;
        out.cross += pw * xw * r.at(pi, xi).cross;
      }
    }
    return out;
  };
  auto both = [&](const double d) {
    const CrossingShare a = ratio(d, extent_a);
    const CrossingShare b = ratio(d, extent_b);
    return CrossingShare{(a.total + b.total) / 2, (a.cross + b.cross) / 2};
  };
  const CrossingShare at_to = both(std::max(0.0, to));
  if (to <= 0) {
    return at_to;
  }
  const double d0 = std::max(0.0, from);
  const CrossingShare at_from = both(d0);
  const CrossingShare k0 = crossingKept(key, d0, left, right);
  const CrossingShare k1 = crossingKept(key, to, left, right);
  auto part = [&](const double kept0,
                  const double kept1,
                  const double ratio0,
                  const double ratio1) {
    const double g0 = (r.width + kept0) / 2;
    const double g1 = (r.width + kept1) / 2;
    if (g1 - g0 <= 1e-12) {
      return ratio1;
    }
    return std::clamp(((ratio1 * g1) - (ratio0 * g0)) / (g1 - g0), 0.0, 2.0);
  };
  return {part(k0.total, k1.total, at_from.total, at_to.total),
          part(k0.cross, k1.cross, at_from.cross, at_to.cross)};
}

DiagonalTable& Model::addDiagonal(const DiagonalKey& key, DiagonalTable table)
{
  return diagonals_.insert_or_assign(key, std::move(table)).first->second;
}

const DiagonalTable* Model::diagonal(const DiagonalKey& key) const
{
  const auto it = diagonals_.find(key);
  return it == diagonals_.end() ? nullptr : &it->second;
}

DiagonalCap Model::diagonal(const DiagonalKey& key,
                            const double width,
                            const double gap,
                            const double spacing,
                            const double far_spacing) const
{
  const DiagonalTable* t = diagonal(key);
  if (t == nullptr) {
    throw std::out_of_range("no diagonal table for layer " + key.layer
                            + " beside " + key.other);
  }
  const Bracket w = bracketLog(t->widths(), width);
  const Bracket g = bracketLinear(t->gaps(), gap);
  const Bracket s = bracketSpacing(t->spacings(), spacing);
  const Bracket f = bracketSpacing(t->farSpacings(), far_spacing);
  DiagonalCap out;
  for (const auto& [wi, ww] :
       {std::pair{w.lo, 1 - w.t}, std::pair{w.hi, w.t}}) {
    for (const auto& [gi, gw] :
         {std::pair{g.lo, 1 - g.t}, std::pair{g.hi, g.t}}) {
      for (const auto& [si, sw] :
           {std::pair{s.lo, 1 - s.t}, std::pair{s.hi, s.t}}) {
        for (const auto& [fi, fw] :
             {std::pair{f.lo, 1 - f.t}, std::pair{f.hi, f.t}}) {
          const double weight = ww * gw * sw * fw;
          if (weight == 0) {
            continue;
          }
          const DiagonalCap& v = t->at(wi, gi, si, fi);
          out.coupling += weight * v.coupling;
          out.total_delta += weight * v.total_delta;
          out.lateral_delta += weight * v.lateral_delta;
          out.plane_delta += weight * v.plane_delta;
        }
      }
    }
  }
  if (!t->gaps().empty() && gap > t->gaps().back() && t->gaps().back() > 0) {
    const double fade = t->gaps().back() / gap;
    out.coupling *= fade;
    out.total_delta *= fade;
    out.lateral_delta *= fade;
    out.plane_delta *= fade;
  }
  return out;
}

EndTable& Model::addEnd(const LateralKey& key, EndTable table)
{
  return ends_.insert_or_assign(key, std::move(table)).first->second;
}

const EndTable* Model::end(const LateralKey& key) const
{
  const auto it = ends_.find(key);
  return it == ends_.end() ? nullptr : &it->second;
}

Corners Model::corners(const LateralKey& key, const double width) const
{
  const EndTable* t = end(key);
  if (t == nullptr) {
    throw std::out_of_range("no end table for layer " + key.layer + " below "
                            + key.below + " above " + key.above);
  }
  if (!t->hasCorners()) {
    return {};
  }
  const Bracket w = bracketLinear(t->widths(), width);
  auto mix = [&](const CornerValue& a, const CornerValue& b) {
    return CornerValue{(1 - w.t) * a.total + w.t * b.total,
                       (1 - w.t) * a.below + w.t * b.below,
                       (1 - w.t) * a.above + w.t * b.above};
  };
  const Corners& lo = t->corners(w.lo);
  const Corners& hi = t->corners(w.hi);
  return {mix(lo.inner, hi.inner), mix(lo.outer, hi.outer)};
}

EndCap Model::end(const LateralKey& key,
                  const double width,
                  const double left,
                  const double right,
                  const double gap) const
{
  const EndTable* t = end(key);
  if (t == nullptr) {
    throw std::out_of_range("no end table for layer " + key.layer + " below "
                            + key.below + " above " + key.above);
  }
  const Bracket w = bracketLinear(t->widths(), width);
  const Bracket g = bracketSpacing(t->gaps(), gap);
  auto couple = [](const double a, const double b, const double u) {
    if (u == 0 || a <= 0 || b <= 0) {
      return (1 - u) * a + u * b;
    }
    return std::exp((1 - u) * std::log(a) + u * std::log(b));
  };
  auto mix = [&](const EndValue& a, const EndValue& b, const double u) {
    return EndValue{(1 - u) * a.total + u * b.total,
                    (1 - u) * a.below + u * b.below,
                    (1 - u) * a.above + u * b.above,
                    couple(a.lateral, b.lateral, u),
                    couple(a.facing, b.facing, u)};
  };
  // The end with a spacing on both sides: over the gap, the spacing, then
  // linearly over the width.
  auto both = [&](const double spacing, const Bracket& over) {
    const Bracket s = bracketSpacing(t->spacings(), spacing);
    auto overGap = [&](const int wi, const int si) {
      return mix(t->at(wi, si, over.lo), t->at(wi, si, over.hi), over.t);
    };
    auto overSpacing = [&](const int wi) {
      return mix(overGap(wi, s.lo), overGap(wi, s.hi), s.t);
    };
    const EndValue a = overSpacing(w.lo);
    const EndValue b = overSpacing(w.hi);
    return EndValue{(1 - w.t) * a.total + w.t * b.total,
                    (1 - w.t) * a.below + w.t * b.below,
                    (1 - w.t) * a.above + w.t * b.above,
                    (1 - w.t) * a.lateral + w.t * b.lateral,
                    (1 - w.t) * a.facing + w.t * b.facing};
  };
  // Each side takes half of the end with its spacing on both.
  auto halves = [&](const Bracket& over) {
    const EndValue l = both(left, over);
    const EndValue r = left == right ? l : both(right, over);
    return EndCap{(l.total + r.total) / 2,
                  (l.below + r.below) / 2,
                  (l.above + r.above) / 2,
                  l.lateral / 2,
                  r.lateral / 2,
                  (l.facing + r.facing) / 2};
  };
  const EndCap at_gap = halves(g);
  if (!t->hasSides()) {
    return at_gap;
  }
  // The different-sided ends over both spacings, then the width.
  const Bracket sl = bracketSpacing(t->spacings(), left);
  const Bracket sr = bracketSpacing(t->spacings(), right);
  auto mixSides = [&](const EndSides& a, const EndSides& b, const double u) {
    return EndSides{(1 - u) * a.total + u * b.total,
                    (1 - u) * a.below + u * b.below,
                    (1 - u) * a.above + u * b.above,
                    couple(a.left, b.left, u),
                    couple(a.right, b.right, u)};
  };
  auto overSpacings = [&](const int wi) {
    auto overRight = [&](const int li) {
      return mixSides(t->sides(wi, li, sr.lo), t->sides(wi, li, sr.hi), sr.t);
    };
    return mixSides(overRight(sl.lo), overRight(sl.hi), sl.t);
  };
  const EndSides a = overSpacings(w.lo);
  const EndSides b = overSpacings(w.hi);
  const EndSides sides{(1 - w.t) * a.total + w.t * b.total,
                       (1 - w.t) * a.below + w.t * b.below,
                       (1 - w.t) * a.above + w.t * b.above,
                       (1 - w.t) * a.left + w.t * b.left,
                       (1 - w.t) * a.right + w.t * b.right};
  // A facing wire adds what it adds to the like-sided ends.
  const int open_gap = t->gapCount() - 1;
  const EndCap none = g.lo == open_gap && g.hi == open_gap
                          ? at_gap
                          : halves(Bracket{open_gap, open_gap, 0});
  return {sides.total + at_gap.total - none.total,
          sides.below + at_gap.below - none.below,
          sides.above + at_gap.above - none.above,
          std::max(0.0, sides.left + at_gap.left - none.left),
          std::max(0.0, sides.right + at_gap.right - none.right),
          at_gap.facing};
}

}  // namespace pex
