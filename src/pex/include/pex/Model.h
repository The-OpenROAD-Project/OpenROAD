// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The capacitance model: tables keyed by a wire piece's context, looked up
// with interpolation. Every table checks its grid when made and its values
// when set, so a model never holds one it cannot look up.

#pragma once

#include <compare>
#include <map>
#include <string>
#include <vector>

namespace pex {

// Per-unit-length capacitances of a wire piece, in fF/µm: to the ground
// plane, to each lateral neighbor, and to the covering plane below and
// above.
struct LateralCap
{
  double ground = 0;
  double left = 0;
  double right = 0;
  double below = 0;
  double above = 0;

  double total() const { return ground + left + right + below + above; }
  // Everything but the two neighbors.
  double rest() const { return ground + below + above; }
};

// The context a lateral table describes: the piece's layer, the nearest
// covering layer below it ("" for the substrate alone) and above it ("" for
// none).
struct LateralKey
{
  std::string layer;
  std::string below;
  std::string above;

  auto operator<=>(const LateralKey&) const = default;
};

// Ground and lateral coupling per unit length over a grid of the wire's
// width and the spacing to the neighbor on each side. A spacing index of
// spacings().size() means no neighbor on that side. The pattern is its
// own mirror image, so the table is too: the point (width, left, right)
// and (width, right, left) with the two couplings swapped always agree.
class LateralTable
{
 public:
  // Throws std::invalid_argument for an empty grid, one that is not
  // ascending, or a width or spacing that is not positive. The values
  // start at zero.
  LateralTable(std::vector<double> widths, std::vector<double> spacings);

  const std::vector<double>& widths() const { return widths_; }
  const std::vector<double>& spacings() const { return spacings_; }
  int widthCount() const { return static_cast<int>(widths_.size()); }
  // The spacings plus the open column.
  int spacingCount() const { return static_cast<int>(spacings_.size()) + 1; }
  // In [width][left][right] order.
  const std::vector<LateralCap>& values() const { return values_; }
  const LateralCap& at(int width, int left, int right) const
  {
    return values_[index(width, left, right)];
  }
  // Stores the value at the point and, with left and right swapped, at
  // its mirror; a point with equal spacings is its own mirror and keeps
  // the value as given. Throws std::invalid_argument for an index off the
  // grid or a value that is not finite.
  void set(int width, int left, int right, const LateralCap& cap);

 private:
  int index(int width, int left, int right) const;

  std::vector<double> widths_;    // µm, ascending
  std::vector<double> spacings_;  // µm, ascending
  std::vector<LateralCap> values_;
};

// The share of a covering plane's effect that crossing wires on the cover
// layer keep over an uncovered gap of the wire between two of them: for the
// total and for the coupling to the crossings. A gap of 0 is the plane
// itself.
struct CrossingShare
{
  double total = 0;
  double cross = 0;
};

struct CrossingKey
{
  std::string layer;
  std::string cover;

  auto operator<=>(const CrossingKey&) const = default;
};

class CrossingTable
{
 public:
  // Throws std::invalid_argument for an empty grid, gaps not ascending or
  // not positive, a value count that differs, or a share that is not
  // finite.
  CrossingTable(std::vector<double> gaps, std::vector<CrossingShare> values);

  const std::vector<double>& gaps() const { return gaps_; }
  const std::vector<CrossingShare>& values() const { return values_; }

 private:
  std::vector<double> gaps_;  // µm, ascending, all positive
  std::vector<CrossingShare> values_;
};

// The coupling per unit length to a parallel wire on a nearby layer lying
// beside the wire rather than over it, in fF/µm, with what it adds to the
// wire's total and takes from the coupling to the wire's own neighbor on
// that side.
struct DiagonalCap
{
  double coupling = 0;
  double total_delta = 0;
  double lateral_delta = 0;
};

struct DiagonalKey
{
  std::string layer;
  std::string other;

  auto operator<=>(const DiagonalKey&) const = default;
};

// Over the parallel wire's width, its edge-to-edge gap from the wire
// (negative when it overlaps, by at most half the width), the spacing to
// the wire's own neighbor on that side, and the spacing to the neighbor on
// the far side; a spacing index of spacings().size() (or
// farSpacings().size()) means no neighbor.
class DiagonalTable
{
 public:
  // Throws std::invalid_argument for an empty grid or one that is not
  // ascending, or a width or spacing that is not positive; gaps may be
  // negative. The values start at zero.
  DiagonalTable(std::vector<double> widths,
                std::vector<double> gaps,
                std::vector<double> spacings,
                std::vector<double> far_spacings);

  const std::vector<double>& widths() const { return widths_; }
  const std::vector<double>& gaps() const { return gaps_; }
  const std::vector<double>& spacings() const { return spacings_; }
  const std::vector<double>& farSpacings() const { return far_spacings_; }
  int widthCount() const { return static_cast<int>(widths_.size()); }
  int gapCount() const { return static_cast<int>(gaps_.size()); }
  int spacingCount() const { return static_cast<int>(spacings_.size()) + 1; }
  int farCount() const { return static_cast<int>(far_spacings_.size()) + 1; }
  // In [width][gap][spacing][far] order.
  const std::vector<DiagonalCap>& values() const { return values_; }
  const DiagonalCap& at(int width, int gap, int spacing, int far) const
  {
    return values_[index(width, gap, spacing, far)];
  }
  // Throws std::invalid_argument for an index off the grid or a value that
  // is not finite.
  void set(int width, int gap, int spacing, int far, const DiagonalCap& cap);

 private:
  int index(int width, int gap, int spacing, int far) const;

  std::vector<double> widths_;        // µm, ascending
  std::vector<double> gaps_;          // µm, ascending
  std::vector<double> spacings_;      // µm, ascending
  std::vector<double> far_spacings_;  // µm, ascending
  std::vector<DiagonalCap> values_;
};

// What one line end adds to a wire's capacitance, in fF, over its width.
class EndTable
{
 public:
  // Throws std::invalid_argument for an empty grid, widths not ascending
  // or not positive, a value count that differs, or an excess that is not
  // finite.
  EndTable(std::vector<double> widths, std::vector<double> excess);

  const std::vector<double>& widths() const { return widths_; }
  const std::vector<double>& excess() const { return excess_; }

 private:
  std::vector<double> widths_;  // µm, ascending
  std::vector<double> excess_;
};

class Model
{
 public:
  // Stores a table under its key, replacing any there, and returns it.
  LateralTable& addLateral(const LateralKey& key, LateralTable table);
  const LateralTable* lateral(const LateralKey& key) const;
  const std::map<LateralKey, LateralTable>& laterals() const
  {
    return laterals_;
  }

  // Interpolates the context's table: linear in the width, and in the
  // logarithms of the spacings, where the couplings interpolate in their
  // own logarithms (they decay about exponentially between grid points)
  // and the rest linearly. A negative spacing, or one beyond the last
  // tabulated, means no neighbor. Below the narrowest width the values
  // are its own; past the widest, the ground and plane couplings follow
  // the last segment's slope while the lateral couplings stay. Throws
  // std::out_of_range when the context has no table.
  LateralCap lateral(const LateralKey& key,
                     double width,
                     double left,
                     double right) const;

  CrossingTable& addCrossing(const CrossingKey& key, CrossingTable table);
  const CrossingTable* crossing(const CrossingKey& key) const;
  const std::map<CrossingKey, CrossingTable>& crossings() const
  {
    return crossings_;
  }
  // The amount of the plane's effect kept over a distance d from one
  // crossing's edge with nothing on the other side, in µm of fully
  // covered wire: the integral of the one-sided share. It is linear in d
  // between the tabulated gaps, where it equals gap × share, and constant
  // past the last. Throws std::out_of_range when the pair has no table.
  CrossingShare crossingKept(const CrossingKey& key, double d) const;
  // The share over a gap between two crossings: 1 at a gap of 0, else the
  // kept amount over the gap, so it falls as 1 / gap past the last
  // tabulated gap.
  CrossingShare crossing(const CrossingKey& key, double gap) const;
  // The share one crossing keeps at a distance d from its edge with
  // nothing on the other side: 1 under the crossing (d <= 0), the slope
  // of the kept amount on the segment holding d otherwise, within [0, 1].
  // The two-sided share over a gap is exactly the mean of the one-sided
  // shares along it.
  CrossingShare crossingOneSided(const CrossingKey& key, double d) const;

  DiagonalTable& addDiagonal(const DiagonalKey& key, DiagonalTable table);
  const DiagonalTable* diagonal(const DiagonalKey& key) const;
  const std::map<DiagonalKey, DiagonalTable>& diagonals() const
  {
    return diagonals_;
  }
  // Interpolates the pair's table: in the logarithm of the width, linearly
  // in the gap (past the last gap, falling as 1 / gap), and in the
  // logarithms of the two spacings, where a negative spacing, or one past
  // the last tabulated, means no neighbor on that side. Throws
  // std::out_of_range when the pair has no table.
  DiagonalCap diagonal(const DiagonalKey& key,
                       double width,
                       double gap,
                       double spacing,
                       double far_spacing) const;

  EndTable& addEnd(const LateralKey& key, EndTable table);
  const EndTable* end(const LateralKey& key) const;
  const std::map<LateralKey, EndTable>& ends() const { return ends_; }
  // What one line end adds, linear in the width and clamped to the
  // table's range. Throws std::out_of_range when the context has no table.
  double endExcess(const LateralKey& key, double width) const;

 private:
  std::map<LateralKey, LateralTable> laterals_;
  std::map<CrossingKey, CrossingTable> crossings_;
  std::map<DiagonalKey, DiagonalTable> diagonals_;
  std::map<LateralKey, EndTable> ends_;
};

}  // namespace pex
