// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "shapes.h"

namespace mpl {

// Exact feasibility of a fixed-outline packing: can these rectangles be
// placed inside a W x H outline without overlap? Coordinates and sizes are
// integers (DBU), relative to the outline's lower-left corner, halos
// included. No odb dependency, so the questions can be asked from unit tests
// and from the annealer's callers alike.

// One piece of a soft block's shape curve: any width in `width`, with
// height = floor(area / width), as mpl's SoftMacro resizes.
struct SoftShape
{
  Interval width;
  int64_t area{0};
};

struct PackingBlock
{
  std::string name;
  // Allowed (width, height) pairs; one entry for a rigid block.
  std::vector<Tiling> shapes;
  // A soft block's shape curve; used instead of `shapes` when not empty.
  std::vector<SoftShape> soft_shapes;
  // A block that may not move; it uses shapes.front().
  std::optional<int> fixed_x;
  std::optional<int> fixed_y;

  bool isFixed() const { return fixed_x.has_value(); }
  bool isSoft() const { return !soft_shapes.empty(); }
};

struct PackingProblem
{
  int width{0};
  int height{0};
  std::vector<PackingBlock> blocks;
};

enum class PackingStatus
{
  kFits,
  kDoesNotFit,
  kUndecided  // the solver's deterministic limit ran out
};

// Why a problem does not fit, in the order they are checked.
enum class PackingCause
{
  kNone,
  kBlockTooWide,  // one block is wider than the outline in every shape
  kBlockTooTall,  // one block is taller than the outline in every shape
  kArea,          // the blocks' minimum areas exceed the outline's area
  kArrangement    // the area suffices, no arrangement does
};

struct PlacedBlock
{
  int x{0};
  int y{0};
  int width{0};
  int height{0};
};

struct PackingResult
{
  PackingStatus status{PackingStatus::kUndecided};
  PackingCause cause{PackingCause::kNone};
  // Index of the block named by kBlockTooWide/kBlockTooTall.
  int block{-1};
  // One entry per block when status is kFits.
  std::vector<PlacedBlock> placement;
  // The solver's status when it decided nothing, for diagnostics.
  std::string solver_status;
};

struct PackingLimits
{
  // CP-SAT deterministic time, independent of machine load.
  double deterministic_time{10.0};
  int seed{0};
  // More than one worker runs CP-SAT's portfolio interleaved, which keeps
  // the answer independent of thread scheduling. Real problems with a
  // dozen or more soft blocks need the portfolio.
  int workers{8};
};

// Throws std::invalid_argument for a problem it cannot state: an outline
// without area, a block fixed in one coordinate only, a movable block
// without a shape, an empty soft piece.
PackingResult checkPacking(const PackingProblem& problem,
                           const PackingLimits& limits = {});

// The smallest outline dimension at which the problem fits, searched on
// multiples of `step` (a row height, a site width). `lower` is proven not to
// fit below it, `upper` proven to fit; they are equal unless a check was
// undecided. A bigger outline never makes a packing impossible, so the
// search is a bisection over exact checks.
// Steps must be positive; like checkPacking(), the searches throw
// std::invalid_argument otherwise.
struct DimensionFix
{
  bool found{false};  // no outline up to the search limit fits
  int lower{0};
  int upper{0};
};

// Keep the width, grow or shrink the height.
DimensionFix smallestHeight(const PackingProblem& problem,
                            int step,
                            const PackingLimits& limits = {});
// Keep the height, grow or shrink the width.
DimensionFix smallestWidth(const PackingProblem& problem,
                           int step,
                           const PackingLimits& limits = {});
// Keep the aspect ratio; the result is the width, on multiples of
// `width_step`; the height follows as ceil(width * height / problem.width)
// rounded up to `height_step`.
DimensionFix smallestSameAspect(const PackingProblem& problem,
                                int width_step,
                                int height_step,
                                const PackingLimits& limits = {});

// The three single changes to the outline that make a problem fit.
struct PackingFixes
{
  DimensionFix height;       // at the same width
  DimensionFix width;        // at the same height
  DimensionFix same_aspect;  // the width; the height keeps the ratio
};

PackingFixes findFixes(const PackingProblem& problem,
                       int width_step,
                       int height_step,
                       const PackingLimits& limits = {});

// The height that goes with `width` under smallestSameAspect().
int sameAspectHeight(const PackingProblem& problem, int width, int height_step);

}  // namespace mpl
