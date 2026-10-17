// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "packing.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_checker.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/sat_parameters.pb.h"
#include "ortools/util/sorted_interval_list.h"

namespace mpl {

namespace sat = operations_research::sat;
using operations_research::Domain;

namespace {

// A problem the check cannot state is a caller's bug: refuse it loudly
// rather than answer a different question.
void requireValid(const PackingProblem& problem)
{
  if (problem.width <= 0 || problem.height <= 0) {
    throw std::invalid_argument("packing: the outline is "
                                + std::to_string(problem.width) + " x "
                                + std::to_string(problem.height));
  }
  for (const PackingBlock& block : problem.blocks) {
    if (block.fixed_x.has_value() != block.fixed_y.has_value()) {
      throw std::invalid_argument("packing: block " + block.name
                                  + " is fixed in one coordinate only");
    }
    if (block.isFixed()) {
      continue;
    }
    if (block.shapes.empty() && block.soft_shapes.empty()) {
      throw std::invalid_argument("packing: block " + block.name
                                  + " has no shape");
    }
    for (const SoftShape& shape : block.soft_shapes) {
      if (shape.width.min <= 0 || shape.width.min > shape.width.max
          || shape.area <= 0) {
        throw std::invalid_argument("packing: block " + block.name
                                    + " has an empty soft piece");
      }
    }
  }
}

// The height a soft block takes at `width`: mpl's SoftMacro rounds down.
int64_t softHeight(const SoftShape& shape, const int64_t width)
{
  return shape.area / width;
}

// The narrowest and the lowest the block can be, and its least area, inside
// the outline. Returns false when no shape fits the outline at all.
struct Extent
{
  int64_t min_width{std::numeric_limits<int64_t>::max()};
  int64_t min_height{std::numeric_limits<int64_t>::max()};
  int64_t min_area{std::numeric_limits<int64_t>::max()};
};

Extent extentOf(const PackingBlock& block, const int width, const int height)
{
  Extent extent;
  if (block.isFixed() || !block.isSoft()) {
    for (const Tiling& shape : block.shapes) {
      if (block.isFixed()
          || (shape.width() <= width && shape.height() <= height)) {
        extent.min_width = std::min<int64_t>(extent.min_width, shape.width());
        extent.min_height
            = std::min<int64_t>(extent.min_height, shape.height());
        extent.min_area = std::min(extent.min_area, shape.area());
      }
    }
    return extent;
  }
  for (const SoftShape& shape : block.soft_shapes) {
    const int64_t lo = std::max(shape.width.min, 1);
    const int64_t hi = std::min(shape.width.max, width);
    if (lo > hi) {
      continue;
    }
    // Height decreases with width: the widest width gives the lowest height.
    if (softHeight(shape, hi) > height) {
      continue;
    }
    extent.min_width = std::min(extent.min_width, lo);
    extent.min_height = std::min(extent.min_height, softHeight(shape, hi));
    // width * floor(area / width) > area - width.
    extent.min_area = std::min(extent.min_area, shape.area - hi + 1);
  }
  return extent;
}

bool sameRigidShape(const PackingBlock& a, const PackingBlock& b)
{
  return !a.isFixed() && !b.isFixed() && !a.isSoft() && !b.isSoft()
         && a.shapes.size() == 1 && b.shapes.size() == 1
         && a.shapes.front() == b.shapes.front();
}

int64_t floorDiv(const int64_t a, const int64_t b)
{
  return a >= 0 ? a / b : -((-a + b - 1) / b);
}

int64_t ceilDiv(const int64_t a, const int64_t b)
{
  return -floorDiv(-a, b);
}

// How a problem is put on a grid of `grid` DBU:
//  - kInner rounds blocks up: a placement found is legal at full
//    resolution, finding none proves nothing;
//  - kOuter rounds blocks down: finding none proves the problem does not
//    fit, a placement found proves nothing.
// At grid 1 both are the problem itself.
enum class Rounding
{
  kInner,
  kOuter
};

struct GridSolve
{
  sat::CpSolverStatus status{sat::CpSolverStatus::UNKNOWN};
  double deterministic_time{0.0};
  bool found{false};                   // kInner found a placement
  std::vector<PlacedBlock> placement;  // full resolution, when found
};

GridSolve solveOnGrid(const PackingProblem& problem,
                      const int grid,
                      const Rounding rounding,
                      const double deterministic_time,
                      const PackingLimits& limits)
{
  const bool inner = rounding == Rounding::kInner;
  const int64_t g = grid;
  const int64_t width = problem.width / g;
  const int64_t height = problem.height / g;
  auto size = [&](const int64_t value) {
    return inner ? ceilDiv(value, g) : floorDiv(value, g);
  };
  const int n = static_cast<int>(problem.blocks.size());

  sat::CpModelBuilder model;
  GridSolve result;
  std::vector<sat::IntVar> xs(n), ys(n), ws(n);
  std::vector<std::vector<sat::BoolVar>> choices(n);
  std::vector<sat::IntervalVar> x_intervals, y_intervals;
  std::vector<int> owner;
  std::vector<bool> is_fixed;
  for (int i = 0; i < n; i++) {
    const PackingBlock& block = problem.blocks[i];
    sat::IntVar x, y, w, h;
    if (block.isFixed()) {
      const Tiling& shape = block.shapes.front();
      // Inner: the fixed block grows to the grid; outer: it shrinks.
      const int64_t x0
          = inner ? floorDiv(*block.fixed_x, g) : ceilDiv(*block.fixed_x, g);
      const int64_t y0
          = inner ? floorDiv(*block.fixed_y, g) : ceilDiv(*block.fixed_y, g);
      const int64_t x1 = inner ? ceilDiv(*block.fixed_x + shape.width(), g)
                               : floorDiv(*block.fixed_x + shape.width(), g);
      const int64_t y1 = inner ? ceilDiv(*block.fixed_y + shape.height(), g)
                               : floorDiv(*block.fixed_y + shape.height(), g);
      if (x1 <= x0 || y1 <= y0 || shape.area() == 0) {
        continue;
      }
      x_intervals.push_back(model.NewFixedSizeIntervalVar(x0, x1 - x0));
      y_intervals.push_back(model.NewFixedSizeIntervalVar(y0, y1 - y0));
      owner.push_back(i);
      is_fixed.push_back(true);
      continue;
    }
    if (block.isSoft()) {
      w = model.NewIntVar(Domain(0, width));
      h = model.NewIntVar(Domain(0, height));
      for (const SoftShape& shape : block.soft_shapes) {
        const int64_t area = shape.area;
        // Widths on the grid that the piece allows.
        const int64_t lo = inner ? ceilDiv(std::max(shape.width.min, 1), g)
                                 : floorDiv(std::max(shape.width.min, 1), g);
        const int64_t hi = std::min(floorDiv(shape.width.max, g), width);
        if (lo > hi) {
          continue;
        }
        const sat::BoolVar choice = model.NewBoolVar();
        choices[i].push_back(choice);
        model.AddGreaterOrEqual(w, lo).OnlyEnforceIf(choice);
        model.AddLessOrEqual(w, hi).OnlyEnforceIf(choice);
        if (g == 1) {
          // height = floor(area / width):
          // width * height <= area < width * (height + 1).
          const sat::IntVar product
              = model.NewIntVar(Domain(0, width * height));
          model.AddMultiplicationEquality(product, {w, h});
          model.AddLessOrEqual(product, area).OnlyEnforceIf(choice);
          model.AddGreaterOrEqual(product + w, area + 1).OnlyEnforceIf(choice);
        } else if (inner) {
          // The real block is (g w, floor(area / (g w))), which fits in
          // g h iff area < g w (g h + 1) = g^2 w h + g w.
          const sat::IntVar product
              = model.NewIntVar(Domain(0, width * height));
          model.AddMultiplicationEquality(product, {w, h});
          model.AddGreaterOrEqual(g * g * product + g * w, area + 1)
              .OnlyEnforceIf(choice);
          model.AddGreaterOrEqual(w, 1).OnlyEnforceIf(choice);
        } else {
          // Any real (w, h) maps to (floor(w / g), floor(h / g)); since
          // area < w (h + 1) < g^2 (w' + 1)(h' + 1), require that.
          const sat::IntVar w1 = model.NewIntVar(Domain(1, width + 1));
          const sat::IntVar h1 = model.NewIntVar(Domain(1, height + 1));
          model.AddEquality(w1, w + 1);
          model.AddEquality(h1, h + 1);
          const sat::IntVar product
              = model.NewIntVar(Domain(0, (width + 1) * (height + 1)));
          model.AddMultiplicationEquality(product, {w1, h1});
          model.AddGreaterOrEqual(g * g * product, area + 1)
              .OnlyEnforceIf(choice);
        }
      }
    } else {
      sat::LinearExpr width_sum, height_sum;
      for (const Tiling& shape : block.shapes) {
        const int64_t sw = size(shape.width());
        const int64_t sh = size(shape.height());
        const sat::BoolVar choice = model.NewBoolVar();
        choices[i].push_back(choice);
        if (sw > width || sh > height) {
          model.AddEquality(choice, 0);
        }
        width_sum += choice * sw;
        height_sum += choice * sh;
      }
      w = model.NewIntVar(Domain(0, width));
      h = model.NewIntVar(Domain(0, height));
      model.AddEquality(w, width_sum);
      model.AddEquality(h, height_sum);
    }
    if (choices[i].empty()) {
      // No shape fits on this grid.
      result.status = sat::CpSolverStatus::INFEASIBLE;
      return result;
    }
    model.AddExactlyOne(choices[i]);
    x = model.NewIntVar(Domain(0, width));
    y = model.NewIntVar(Domain(0, height));
    const sat::IntVar x_end = model.NewIntVar(Domain(0, width));
    const sat::IntVar y_end = model.NewIntVar(Domain(0, height));
    x_intervals.push_back(model.NewIntervalVar(x, w, x_end));
    y_intervals.push_back(model.NewIntervalVar(y, h, y_end));
    owner.push_back(i);
    is_fixed.push_back(false);
    xs[i] = x;
    ys[i] = y;
    ws[i] = w;
  }

  // Fixed blocks may overlap each other (a blockage over a fixed macro);
  // only movable blocks must avoid them.
  std::vector<int> fixed, movable;
  for (int k = 0; k < static_cast<int>(owner.size()); k++) {
    (is_fixed[k] ? fixed : movable).push_back(k);
  }
  auto add_rectangles = [&](sat::NoOverlap2DConstraint& constraint,
                            const std::vector<int>& indices) {
    for (const int k : indices) {
      constraint.AddRectangle(x_intervals[k], y_intervals[k]);
    }
  };
  if (fixed.empty()) {
    sat::NoOverlap2DConstraint constraint = model.AddNoOverlap2D();
    add_rectangles(constraint, movable);
  }
  for (const int f : fixed) {
    sat::NoOverlap2DConstraint constraint = model.AddNoOverlap2D();
    add_rectangles(constraint, movable);
    add_rectangles(constraint, {f});
  }

  // Identical rigid blocks are interchangeable: order them, left to right
  // then bottom to top, so the solver never explores a permutation twice.
  int previous = -1;
  for (int i = 0; i < n; i++) {
    if (problem.blocks[i].isFixed()) {
      continue;
    }
    if (previous >= 0
        && sameRigidShape(problem.blocks[previous], problem.blocks[i])) {
      model.AddLessOrEqual(xs[previous] * (height + 1) + ys[previous],
                           xs[i] * (height + 1) + ys[i]);
    }
    previous = i;
  }

  std::string parameters
      = "num_workers: " + std::to_string(limits.workers)
        + " random_seed: " + std::to_string(limits.seed)
        + " max_deterministic_time: " + std::to_string(deterministic_time);
  if (limits.workers > 1) {
    parameters += " interleave_search: true";
  }
  if (const char* extra = std::getenv("MPL_PACKING_PARAMS")) {
    parameters += std::string(" ") + extra;
  }
  sat::Model solver;
  solver.Add(sat::NewSatParameters(parameters));
  const sat::CpModelProto proto = model.Build();
  const sat::CpSolverResponse response = sat::SolveCpModel(proto, &solver);
  result.status = response.status();
  result.deterministic_time = response.deterministic_time();
  if (result.status == sat::CpSolverStatus::MODEL_INVALID) {
    throw std::logic_error("checkPacking built an invalid model: "
                           + sat::ValidateCpModel(proto));
  }
  const bool found = result.status == sat::CpSolverStatus::OPTIMAL
                     || result.status == sat::CpSolverStatus::FEASIBLE;
  if (!found || !inner) {
    return result;
  }
  result.found = true;
  for (int i = 0; i < n; i++) {
    const PackingBlock& block = problem.blocks[i];
    if (block.isFixed()) {
      result.placement.push_back({*block.fixed_x,
                                  *block.fixed_y,
                                  block.shapes.front().width(),
                                  block.shapes.front().height()});
      continue;
    }
    PlacedBlock placed;
    placed.x = static_cast<int>(sat::SolutionIntegerValue(response, xs[i]) * g);
    placed.y = static_cast<int>(sat::SolutionIntegerValue(response, ys[i]) * g);
    int chosen = 0;
    while (!sat::SolutionBooleanValue(response, choices[i][chosen])) {
      chosen++;
    }
    if (block.isSoft()) {
      // Choices were made only for pieces with a grid width; map back.
      int piece = -1;
      for (const SoftShape& shape : block.soft_shapes) {
        piece++;
        const int64_t lo = g == 1 ? std::max(shape.width.min, 1)
                                  : ceilDiv(std::max(shape.width.min, 1), g);
        const int64_t hi = std::min(floorDiv(shape.width.max, g), width);
        if (lo > hi) {
          continue;
        }
        if (chosen-- == 0) {
          break;
        }
      }
      const SoftShape& shape = block.soft_shapes[piece];
      placed.width
          = static_cast<int>(sat::SolutionIntegerValue(response, ws[i]) * g);
      placed.height = static_cast<int>(shape.area / placed.width);
    } else {
      placed.width = block.shapes[chosen].width();
      placed.height = block.shapes[chosen].height();
    }
    result.placement.push_back(placed);
  }
  return result;
}

// Independent check of a placement at full resolution; a failure is a bug.
void assertLegal(const PackingProblem& problem,
                 const std::vector<PlacedBlock>& placement)
{
  const int n = static_cast<int>(problem.blocks.size());
  for (int i = 0; i < n; i++) {
    const PlacedBlock& p = placement[i];
    if (problem.blocks[i].isFixed()) {
      continue;
    }
    if (p.x < 0 || p.y < 0 || p.x + p.width > problem.width
        || p.y + p.height > problem.height) {
      throw std::logic_error("checkPacking placed " + problem.blocks[i].name
                             + " outside the outline");
    }
  }
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (problem.blocks[i].isFixed() && problem.blocks[j].isFixed()) {
        continue;
      }
      const PlacedBlock& a = placement[i];
      const PlacedBlock& b = placement[j];
      if (static_cast<int64_t>(a.width) * a.height == 0
          || static_cast<int64_t>(b.width) * b.height == 0) {
        continue;
      }
      if (a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height
          && b.y < a.y + a.height) {
        throw std::logic_error("checkPacking overlapped "
                               + problem.blocks[i].name + " and "
                               + problem.blocks[j].name);
      }
    }
  }
}

// Shelves, tallest block first, each block in the first shelf with room;
// with `columns` the same with width and height exchanged. Movable blocks
// only. Empty when it finds no placement, which proves nothing.
std::vector<PlacedBlock> shelfPack(const PackingProblem& problem,
                                   const bool columns)
{
  const int n = static_cast<int>(problem.blocks.size());
  const int64_t across = columns ? problem.height : problem.width;
  const int64_t along = columns ? problem.width : problem.height;
  // Per block, the shape that is lowest across the shelf direction.
  std::vector<std::pair<int64_t, int64_t>> size(n);  // (across, along)
  for (int i = 0; i < n; i++) {
    const PackingBlock& block = problem.blocks[i];
    if (block.isFixed()) {
      return {};
    }
    // A soft block offers a few widths of each piece, with the annealer's
    // height for each.
    std::vector<Tiling> shapes = block.shapes;
    for (const SoftShape& piece : block.soft_shapes) {
      const int64_t lo = std::max(piece.width.min, 1);
      const int64_t hi = piece.width.max;
      for (int k = 0; k <= 8; k++) {
        const int64_t w = lo + (hi - lo) * k / 8;
        if (w > 0 && piece.area / w > 0) {
          shapes.emplace_back(static_cast<int>(w),
                              static_cast<int>(piece.area / w));
        }
      }
    }
    bool found = false;
    for (const Tiling& shape : shapes) {
      const int64_t a = columns ? shape.height() : shape.width();
      const int64_t b = columns ? shape.width() : shape.height();
      if (a <= across && b <= along && (!found || b < size[i].second)) {
        size[i] = {a, b};
        found = true;
      }
    }
    if (!found) {
      return {};
    }
  }
  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
    return size[a].second > size[b].second;
  });
  struct Shelf
  {
    int64_t start;
    int64_t depth;
    int64_t used;
  };
  std::vector<Shelf> shelves;
  int64_t next_start = 0;
  std::vector<PlacedBlock> placement(n);
  for (const int i : order) {
    const auto [a, b] = size[i];
    Shelf* shelf = nullptr;
    for (Shelf& candidate : shelves) {
      if (candidate.used + a <= across && b <= candidate.depth) {
        shelf = &candidate;
        break;
      }
    }
    if (shelf == nullptr) {
      if (next_start + b > along) {
        return {};
      }
      shelves.push_back({next_start, b, 0});
      next_start += b;
      shelf = &shelves.back();
    }
    const int64_t pa = shelf->used;
    const int64_t pb = shelf->start;
    shelf->used += a;
    placement[i] = columns ? PlacedBlock{static_cast<int>(pb),
                                         static_cast<int>(pa),
                                         static_cast<int>(b),
                                         static_cast<int>(a)}
                           : PlacedBlock{static_cast<int>(pa),
                                         static_cast<int>(pb),
                                         static_cast<int>(a),
                                         static_cast<int>(b)};
  }
  return placement;
}

// Coarse grids first: each level tries the inner rounding (a fit is
// final) and the outer one (no fit is final); grid 1 is the problem itself.
PackingResult ladder(const PackingProblem& problem, const PackingLimits& limits)
{
  PackingResult result;
  const int width = problem.width;
  const int height = problem.height;
  std::vector<int> grids;
  for (int64_t grid = std::max<int64_t>(1, std::max(width, height) / 512);
       grid > 1;
       grid /= 4) {
    grids.push_back(static_cast<int>(grid));
  }
  grids.push_back(1);
  double remaining = limits.deterministic_time;
  for (size_t level = 0; level < grids.size(); level++) {
    if (remaining <= 0) {
      break;  // budget spent: undecided
    }
    const int grid = grids[level];
    const double share = remaining / (grids.size() - level);
    const GridSolve fit
        = solveOnGrid(problem, grid, Rounding::kInner, share / 2, limits);
    remaining -= fit.deterministic_time;
    if (fit.found) {
      assertLegal(problem, fit.placement);
      result.status = PackingStatus::kFits;
      result.placement = fit.placement;
      return result;
    }
    if (grid == 1) {
      if (fit.status == sat::CpSolverStatus::INFEASIBLE) {
        result.status = PackingStatus::kDoesNotFit;
        result.cause = PackingCause::kArrangement;
      } else {
        result.solver_status = sat::CpSolverStatus_Name(fit.status);
      }
      return result;
    }
    if (remaining <= 0) {
      break;
    }
    const GridSolve proof = solveOnGrid(problem,
                                        grid,
                                        Rounding::kOuter,
                                        remaining / (grids.size() - level) / 2,
                                        limits);
    remaining -= proof.deterministic_time;
    if (proof.status == sat::CpSolverStatus::INFEASIBLE) {
      result.status = PackingStatus::kDoesNotFit;
      result.cause = PackingCause::kArrangement;
      return result;
    }
  }
  return result;
}

}  // namespace

PackingResult checkPacking(const PackingProblem& problem,
                           const PackingLimits& limits)
{
  requireValid(problem);
  PackingResult result;
  const int width = problem.width;
  const int height = problem.height;
  const int n = static_cast<int>(problem.blocks.size());

  // Bounds that need no search, cheapest first.
  int64_t total_area = 0;
  for (int i = 0; i < n; i++) {
    const PackingBlock& block = problem.blocks[i];
    const Extent extent = extentOf(block, width, height);
    if (extent.min_area == std::numeric_limits<int64_t>::max()) {
      // No shape fits on its own: say which side is too short.
      bool fits_wide = false;
      if (block.isSoft()) {
        for (const SoftShape& shape : block.soft_shapes) {
          fits_wide |= shape.width.min <= width;
        }
      } else {
        for (const Tiling& shape : block.shapes) {
          fits_wide |= shape.width() <= width;
        }
      }
      result.status = PackingStatus::kDoesNotFit;
      result.cause = fits_wide ? PackingCause::kBlockTooTall
                               : PackingCause::kBlockTooWide;
      result.block = i;
      return result;
    }
    if (!block.isFixed()) {
      total_area += extent.min_area;
    }
  }
  if (total_area > static_cast<int64_t>(width) * height) {
    result.status = PackingStatus::kDoesNotFit;
    result.cause = PackingCause::kArea;
    return result;
  }
  // No two blocks wider than half the outline share a row, so their
  // heights add up; likewise for blocks taller than half, by column.
  int64_t wide_height = 0;
  int64_t tall_width = 0;
  for (const PackingBlock& block : problem.blocks) {
    if (block.isFixed() || block.isSoft()) {
      continue;
    }
    int64_t least_height = std::numeric_limits<int64_t>::max();
    int64_t least_width = std::numeric_limits<int64_t>::max();
    bool always_wide = true;
    bool always_tall = true;
    for (const Tiling& shape : block.shapes) {
      if (shape.width() > width || shape.height() > height) {
        continue;
      }
      always_wide &= 2 * static_cast<int64_t>(shape.width()) > width;
      always_tall &= 2 * static_cast<int64_t>(shape.height()) > height;
      least_height = std::min<int64_t>(least_height, shape.height());
      least_width = std::min<int64_t>(least_width, shape.width());
    }
    wide_height += always_wide ? least_height : 0;
    tall_width += always_tall ? least_width : 0;
  }
  if (wide_height > height || tall_width > width) {
    result.status = PackingStatus::kDoesNotFit;
    result.cause = PackingCause::kArrangement;
    return result;
  }
  // Each group of identical rigid blocks must fit on its own, and a grid
  // is the most of them an outline holds.
  std::map<std::pair<int, int>, int64_t> identical_count;
  for (const PackingBlock& block : problem.blocks) {
    if (!block.isFixed() && !block.isSoft() && block.shapes.size() == 1) {
      identical_count[{block.shapes.front().width(),
                       block.shapes.front().height()}]++;
    }
  }
  for (const auto& [shape, count] : identical_count) {
    if (static_cast<int64_t>(width / std::max(shape.first, 1))
            * (height / std::max(shape.second, 1))
        < count) {
      result.status = PackingStatus::kDoesNotFit;
      result.cause = PackingCause::kArrangement;
      return result;
    }
  }

  // Identical blocks that cannot rotate (a macro array) fit exactly when a
  // grid of them does: floor(W / w) * floor(H / h) >= n.
  auto rigid_single = [](const PackingBlock& block) {
    return !block.isFixed() && !block.isSoft() && block.shapes.size() == 1;
  };
  bool identical = n > 0 && rigid_single(problem.blocks.front());
  for (const PackingBlock& block : problem.blocks) {
    identical
        = identical && rigid_single(block)
          && block.shapes.front() == problem.blocks.front().shapes.front();
  }
  if (identical) {
    const Tiling& shape = problem.blocks.front().shapes.front();
    const int64_t columns = width / std::max(shape.width(), 1);
    const int64_t rows = height / std::max(shape.height(), 1);
    if (columns * rows < n) {
      result.status = PackingStatus::kDoesNotFit;
      result.cause = PackingCause::kArrangement;
      return result;
    }
    result.status = PackingStatus::kFits;
    for (int i = 0; i < n; i++) {
      result.placement.push_back(
          {static_cast<int>(i % columns) * shape.width(),
           static_cast<int>(i / columns) * shape.height(),
           shape.width(),
           shape.height()});
    }
    return result;
  }

  // Easy problems fit on shelves.
  for (const bool columns : {false, true}) {
    std::vector<PlacedBlock> placement = shelfPack(problem, columns);
    if (!placement.empty()) {
      assertLegal(problem, placement);
      result.status = PackingStatus::kFits;
      result.placement = std::move(placement);
      return result;
    }
  }

  // One worker decides most problems in milliseconds; a dozen or more
  // movable blocks need CP-SAT's portfolio (interleaved, so still
  // deterministic).
  int movable = 0;
  for (const PackingBlock& block : problem.blocks) {
    movable += block.isFixed() ? 0 : 1;
  }
  PackingLimits used = limits;
  used.workers = movable >= 12 ? limits.workers : 1;
  return ladder(problem, used);
}

namespace {

int64_t roundUp(const int64_t value, const int step)
{
  return (value + step - 1) / step * step;
}

// Bisect over multiples of `step` for the smallest value at which
// `resize(value)` fits. `fits(value)` must be monotone.
template <typename Resize>
DimensionFix bisect(const PackingProblem& problem,
                    const int step,
                    const int64_t limit,
                    const PackingLimits& limits,
                    Resize resize)
{
  if (step <= 0) {
    throw std::invalid_argument("packing: the search step is "
                                + std::to_string(step));
  }
  // A bisection asks some 30 questions; they share one budget, and an
  // undecided one widens the range rather than stalling the error.
  PackingLimits each = limits;
  each.deterministic_time = limits.deterministic_time / 30;
  auto check = [&](const int64_t value) {
    return checkPacking(resize(problem, static_cast<int>(value)), each).status;
  };
  // proven_lo: largest value proven not to fit; lo: largest value not
  // known to fit (proven or undecided); hi: smallest value proven to fit.
  DimensionFix fix;
  int64_t proven_lo = 0;
  int64_t lo = 0;
  int64_t hi = step;
  const int64_t top = roundUp(limit, step);
  for (;; hi = std::min(roundUp(hi * 2, step), top)) {
    if (hi <= lo) {
      return fix;  // nothing up to the search limit fits
    }
    const PackingStatus status = check(hi);
    if (status == PackingStatus::kFits) {
      break;
    }
    if (status == PackingStatus::kDoesNotFit) {
      proven_lo = hi;
    }
    lo = hi;
  }
  while (hi - lo > step) {
    const int64_t probe
        = std::min(roundUp(lo + (hi - lo) / 2, step), hi - step);
    switch (check(probe)) {
      case PackingStatus::kFits:
        hi = probe;
        break;
      case PackingStatus::kDoesNotFit:
        proven_lo = std::max(proven_lo, probe);
        lo = probe;
        break;
      case PackingStatus::kUndecided:
        lo = probe;
        break;
    }
  }
  fix.found = true;
  fix.upper = static_cast<int>(hi);
  fix.lower = static_cast<int>(std::min(proven_lo + step, hi));
  return fix;
}

int64_t searchLimit(const PackingProblem& problem)
{
  int64_t limit = std::max(problem.width, problem.height);
  for (const PackingBlock& block : problem.blocks) {
    int64_t largest = 0;
    for (const Tiling& shape : block.shapes) {
      largest = std::max<int64_t>(largest, shape.width() + shape.height());
    }
    for (const SoftShape& shape : block.soft_shapes) {
      largest = std::max<int64_t>(largest, shape.area);
    }
    if (block.isFixed()) {
      largest += std::max(*block.fixed_x, *block.fixed_y);
    }
    limit += largest;
  }
  return std::min<int64_t>(limit, std::numeric_limits<int>::max() / 2);
}

}  // namespace

DimensionFix smallestHeight(const PackingProblem& problem,
                            const int step,
                            const PackingLimits& limits)
{
  return bisect(problem,
                step,
                searchLimit(problem),
                limits,
                [](PackingProblem copy, const int value) {
                  copy.height = value;
                  return copy;
                });
}

DimensionFix smallestWidth(const PackingProblem& problem,
                           const int step,
                           const PackingLimits& limits)
{
  return bisect(problem,
                step,
                searchLimit(problem),
                limits,
                [](PackingProblem copy, const int value) {
                  copy.width = value;
                  return copy;
                });
}

int sameAspectHeight(const PackingProblem& problem,
                     const int width,
                     const int height_step)
{
  requireValid(problem);
  if (height_step <= 0) {
    throw std::invalid_argument("packing: the height step is "
                                + std::to_string(height_step));
  }
  const int64_t w = problem.width;
  return static_cast<int>(roundUp(
      (width * static_cast<int64_t>(problem.height) + w - 1) / w, height_step));
}

DimensionFix smallestSameAspect(const PackingProblem& problem,
                                const int width_step,
                                const int height_step,
                                const PackingLimits& limits)
{
  return bisect(problem,
                width_step,
                searchLimit(problem),
                limits,
                [&problem, height_step](PackingProblem copy, const int value) {
                  copy.width = value;
                  copy.height = sameAspectHeight(problem, value, height_step);
                  return copy;
                });
}

PackingFixes findFixes(const PackingProblem& problem,
                       const int width_step,
                       const int height_step,
                       const PackingLimits& limits)
{
  PackingFixes fixes;
  fixes.height = smallestHeight(problem, height_step, limits);
  fixes.width = smallestWidth(problem, width_step, limits);
  fixes.same_aspect
      = smallestSameAspect(problem, width_step, height_step, limits);
  return fixes;
}

namespace {

std::string microns(const int64_t dbu, const int dbu_per_micron)
{
  char text[32];
  std::snprintf(
      text, sizeof(text), "%.3f", static_cast<double>(dbu) / dbu_per_micron);
  return text;
}

// "123.456" or "between 123.456 and 130.000" when a check was undecided.
std::string fixText(const DimensionFix& fix, const int dbu_per_micron)
{
  if (fix.lower == fix.upper) {
    return microns(fix.upper, dbu_per_micron);
  }
  return "between " + microns(fix.lower, dbu_per_micron) + " and "
         + microns(fix.upper, dbu_per_micron);
}

}  // namespace

std::string explainDoesNotFit(const PackingProblem& problem,
                              const PackingResult& result,
                              const PackingFixes& fixes,
                              const int dbu_per_micron)
{
  requireValid(problem);
  if (result.status != PackingStatus::kDoesNotFit || dbu_per_micron <= 0) {
    throw std::invalid_argument(
        "packing: explainDoesNotFit needs a kDoesNotFit result and a "
        "positive dbu_per_micron");
  }
  auto um = [dbu_per_micron](const int64_t dbu) {
    return microns(dbu, dbu_per_micron);
  };
  std::string text = "The blocks, halos included, cannot fit in the "
                     + um(problem.width) + " x " + um(problem.height)
                     + " um outline (proven).\n  Cause: ";
  int64_t area = 0;
  for (const PackingBlock& block : problem.blocks) {
    if (block.isFixed()) {
      continue;
    }
    int64_t least = std::numeric_limits<int64_t>::max();
    for (const Tiling& shape : block.shapes) {
      least = std::min(least, shape.area());
    }
    for (const SoftShape& shape : block.soft_shapes) {
      least = std::min(least, shape.area - shape.width.max + 1);
    }
    area += least;
  }
  switch (result.cause) {
    case PackingCause::kBlockTooWide:
    case PackingCause::kBlockTooTall: {
      const PackingBlock& block = problem.blocks[result.block];
      const bool wide = result.cause == PackingCause::kBlockTooWide;
      text += block.name + " is " + (wide ? "wider" : "taller")
              + " than the outline in every shape it can take.";
      break;
    }
    case PackingCause::kArea: {
      const double square
          = static_cast<double>(dbu_per_micron) * dbu_per_micron;
      char numbers[96];
      std::snprintf(
          numbers,
          sizeof(numbers),
          "%.1f um^2, the outline has %.1f um^2",
          area / square,
          static_cast<double>(problem.width) * problem.height / square);
      text += std::string("the blocks need at least ") + numbers
              + "; no arrangement helps.";
      break;
    }
    case PackingCause::kArrangement:
    case PackingCause::kNone:
      text += "the area suffices ("
              + std::to_string(
                  100 * area
                  / (static_cast<int64_t>(problem.width) * problem.height))
              + "% of the outline), but no arrangement of the blocks does.";
      break;
  }
  text += "\n  It fits with any one of:";
  if (fixes.height.found) {
    text += "\n    same width " + um(problem.width)
            + " um: height >= " + fixText(fixes.height, dbu_per_micron) + " um";
  } else {
    text += "\n    same width " + um(problem.width) + " um: no height suffices";
  }
  if (fixes.width.found) {
    text += "\n    same height " + um(problem.height)
            + " um: width >= " + fixText(fixes.width, dbu_per_micron) + " um";
  } else {
    text
        += "\n    same height " + um(problem.height) + " um: no width suffices";
  }
  if (fixes.same_aspect.found) {
    text += "\n    same aspect ratio: width >= "
            + fixText(fixes.same_aspect, dbu_per_micron) + " um";
  }
  return text;
}

}  // namespace mpl
