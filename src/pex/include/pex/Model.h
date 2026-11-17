// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The capacitance model: tables keyed by a wire piece's context, looked up
// with interpolation. Every table checks its grid when made and its values
// when set, so a model never holds one it cannot look up.

#pragma once

#include <compare>
#include <map>
#include <optional>
#include <string>
#include <string_view>
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

// Crossings on `cover` over or under a wire on `layer`, with a plane on
// `opposite`, a layer on the wire's other side (none when empty).
struct CrossingKey
{
  std::string layer;
  std::string cover;
  std::string opposite;

  auto operator<=>(const CrossingKey&) const = default;
};

// How much of a crossing's effect a single crossing of a given width
// keeps when it runs only `extent` past the wire's sides, for the wire
// running `past` beyond the crossing's sides, relative to a crossing that
// runs on: for the total and for the coupling, alone with no neighbor.
// The past and extent grids start at 0; the last extent is one running
// on. Values in [past][extent] order.
struct CrossingReach
{
  double width = 0;  // the crossing's, µm
  std::vector<double> past;
  std::vector<double> extents;
  std::vector<CrossingShare> values;

  const CrossingShare& at(int p, int e) const
  {
    return values[(p * extents.size()) + e];
  }
};

// The shares over the gaps, for each spacing to neighbors of minimum width
// on both sides of the wire under the same crossings, and with no
// neighbor last.
class CrossingTable
{
 public:
  // Values in [spacing][gap] order. Throws std::invalid_argument for an
  // empty gap grid, a grid not ascending or not positive, a value count
  // that is not gaps x (spacings + 1), or a share that is not finite.
  CrossingTable(std::vector<double> gaps,
                std::vector<CrossingShare> values,
                std::vector<double> spacings = {});

  const std::vector<double>& gaps() const { return gaps_; }
  const std::vector<double>& spacings() const { return spacings_; }
  // With the column of no neighbor.
  int spacingCount() const { return static_cast<int>(spacings_.size()) + 1; }
  const std::vector<CrossingShare>& values() const { return values_; }
  const CrossingShare& at(int spacing, int gap) const
  {
    return values_[(spacing * gaps_.size()) + gap];
  }
  // The reach is optional. Throws std::invalid_argument for a width that
  // is not positive, a grid not ascending from 0, a value count that is
  // not past x extents, or a value that is not finite.
  bool hasReach() const { return !reach_.values.empty(); }
  const CrossingReach& reach() const { return reach_; }
  void setReach(CrossingReach reach);

 private:
  std::vector<double> gaps_;      // µm, ascending, all positive
  std::vector<double> spacings_;  // µm, ascending
  std::vector<CrossingShare> values_;
  CrossingReach reach_;  // empty if not held
};

// The coupling per unit length to a parallel wire on a nearby layer lying
// beside the wire rather than over it, in fF/µm, with what it adds to the
// wire's total and takes from the coupling to the wire's own neighbor on
// that side; with a plane on the wire's other side, also what it changes in
// the coupling to that plane.
struct DiagonalCap
{
  double coupling = 0;
  double total_delta = 0;
  double lateral_delta = 0;
  double plane_delta = 0;
};

// A parallel wire on `other` beside a wire on `layer`, with a plane on
// `opposite`, a layer on the wire's other side (none when empty).
struct DiagonalKey
{
  std::string layer;
  std::string other;
  std::string opposite;

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

// What one line end adds, in fF, at a tabulated point: the total, and
// the parts of it to the planes below and above, to the neighbors on both
// sides together, and to a wire facing the end.
struct EndValue
{
  double total = 0;
  double below = 0;
  double above = 0;
  double lateral = 0;
  double facing = 0;
};

// What one line end adds, in fF, at a tabulated point with a different
// spacing on each side and no facing wire: the total, and its parts to the
// planes and to the left and right neighbors.
struct EndSides
{
  double total = 0;
  double below = 0;
  double above = 0;
  double left = 0;
  double right = 0;
};

// What one corner where two wires of a net meet adds, in fF: the total
// and its parts to the planes below and above. An inner corner, between
// the two wires, takes away what each one's side loses to the other; an
// outer corner, where a bend turns, adds to the bend's two wires without
// their ends.
struct CornerValue
{
  double total = 0;
  double below = 0;
  double above = 0;
};

struct Corners
{
  CornerValue inner;
  CornerValue outer;
};

// What one line end adds to a wire's capacitance over its width, the
// spacing to a neighbor of minimum width on each side running past the end
// (the same on both), and the gap to a wire as wide facing the end, end to
// end. The column after the last spacing is no neighbor, and after the
// last gap no facing wire. Optionally, the ends with a different spacing on
// each side and no facing wire, and the corners of wires of the width
// meeting, with no neighbors.
class EndTable
{
 public:
  // Throws std::invalid_argument for an empty width grid or a grid not
  // ascending or not positive; spacings and gaps may be empty.
  EndTable(std::vector<double> widths,
           std::vector<double> spacings = {},
           std::vector<double> gaps = {});

  const std::vector<double>& widths() const { return widths_; }
  const std::vector<double>& spacings() const { return spacings_; }
  const std::vector<double>& gaps() const { return gaps_; }
  // With the open spacing column and the column of no facing wire.
  int spacingCount() const { return static_cast<int>(spacings_.size()) + 1; }
  int gapCount() const { return static_cast<int>(gaps_.size()) + 1; }
  const std::vector<EndValue>& values() const { return values_; }
  const EndValue& at(int width, int spacing, int gap) const
  {
    return values_[index(width, spacing, gap)];
  }
  // Throws std::invalid_argument for an index off the grid or a value
  // that is not finite.
  void set(int width, int spacing, int gap, const EndValue& value);

  // Whether the ends with a different spacing on each side are held.
  bool hasSides() const { return !sides_.empty(); }
  // The end with spacing indices `left` and `right` and no facing wire:
  // the held value when they differ, the end with that spacing on both
  // sides, halved between them, when they are the same.
  EndSides sides(int width, int left, int right) const;
  // Sets the end for left < right and its mirror. Throws
  // std::invalid_argument for an index off the grid, left >= right, or a
  // value that is not finite.
  void setSides(int width, int left, int right, const EndSides& value);

  // Whether the corners are held.
  bool hasCorners() const { return !corners_.empty(); }
  const Corners& corners(int width) const { return corners_.at(width); }
  // Throws std::invalid_argument for a width off the grid or a value that
  // is not finite.
  void setCorners(int width, const Corners& value);

 private:
  int index(int width, int spacing, int gap) const;
  int sideIndex(int width, int left, int right) const;

  std::vector<double> widths_;    // µm, ascending
  std::vector<double> spacings_;  // µm, ascending
  std::vector<double> gaps_;      // µm, ascending
  std::vector<EndValue> values_;
  std::vector<EndSides> sides_;   // [width][left][right]; empty if not held
  std::vector<Corners> corners_;  // by width; empty if not held
};

// What one line end adds, in fF, looked up: the total, and its parts to
// the planes, to the left and right neighbors, and to the facing wire.
struct EndCap
{
  double total = 0;
  double below = 0;
  double above = 0;
  double left = 0;
  double right = 0;
  double facing = 0;
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
  // The wire's neighbors on the left and right (negative or past the last
  // tabulated spacing: none) each take half, from the spacing's column,
  // interpolated in its logarithm.
  CrossingShare crossingKept(const CrossingKey& key,
                             double d,
                             double left = -1,
                             double right = -1) const;
  // The share over a gap between two crossings: 1 at a gap of 0, else the
  // kept amount over the gap, so it falls as 1 / gap past the last
  // tabulated gap.
  CrossingShare crossing(const CrossingKey& key,
                         double gap,
                         double left = -1,
                         double right = -1) const;
  // The share one crossing keeps at a distance d from its edge with
  // nothing on the other side: 1 under the crossing (d <= 0), the slope
  // of the kept amount on the segment holding d otherwise, within [0, 1].
  // The two-sided share over a gap is exactly the mean of the one-sided
  // shares along it.
  CrossingShare crossingOneSided(const CrossingKey& key,
                                 double d,
                                 double left = -1,
                                 double right = -1) const;
  // The part of a single crossing's effect on the wire over [from, to]
  // past one of the crossing's sides (to <= 0: under the crossing) that
  // the crossing keeps running `extent_a` and `extent_b` past the wire's
  // two sides, each side of the crossing taking half: 1 without a reach.
  // Through the reach, the effect kept up to a distance d is the
  // crossing's half width plus half the kept amount, times the ratio at
  // d, linear in both distances, held past the last past and 1 past the
  // last extent; the part over the span is its difference across it. A
  // table with a plane opposite and no reach of its own takes the reach of
  // the pair's table without the plane. Throws std::out_of_range when the
  // pair has no table.
  CrossingShare crossingReach(const CrossingKey& key,
                              double from,
                              double to,
                              double extent_a,
                              double extent_b,
                              double left = -1,
                              double right = -1) const;

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
  // The context's corners, linear in the width and clamped to the table's
  // range; none when its end table holds no corners. Throws
  // std::out_of_range when the context has no end table.
  Corners corners(const LateralKey& key, double width) const;
  const EndTable* end(const LateralKey& key) const;
  const std::map<LateralKey, EndTable>& ends() const { return ends_; }
  // What one line end adds: linear in the width, and in the logarithms
  // of the spacings and the gap, where the couplings interpolate in their
  // own logarithms; clamped to the table's ranges below. A spacing or gap
  // that is negative, or beyond the last tabulated, means no neighbor or
  // no facing wire. With the different-sided ends held, those are
  // interpolated over both spacings at no facing wire, and a facing wire
  // adds what it adds to the like-sided ends; otherwise each side takes
  // half of the end with that side's spacing on both. Throws
  // std::out_of_range when the context has no table.
  EndCap end(const LateralKey& key,
             double width,
             double left = -1,
             double right = -1,
             double gap = -1) const;
  double endExcess(const LateralKey& key, double width) const
  {
    return end(key, width).total;
  }

 private:
  std::map<LateralKey, LateralTable> laterals_;
  std::map<CrossingKey, CrossingTable> crossings_;
  std::map<DiagonalKey, DiagonalTable> diagonals_;
  std::map<LateralKey, EndTable> ends_;
};

}  // namespace pex
