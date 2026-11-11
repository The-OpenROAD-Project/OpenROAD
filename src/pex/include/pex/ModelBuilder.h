// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Builds model tables from a process stack by solving patterns with fsv.

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Stack.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {

struct BuildOptions
{
  // The technology's routing layers, as ITF conductor names: these alone
  // become pieces, covers and diagonal partners. Empty means every
  // conductor of the stack. At gate level only routing layers appear in a
  // window, so the device layers' tables would never be looked up.
  std::vector<std::string> layers;
  // The lateral grid, as multiples of the layer's minimum width and spacing.
  std::vector<double> width_factors{1, 1.5, 2, 3, 5, 10};
  std::vector<double> spacing_factors{1, 1.25, 1.5, 2, 3, 5, 7, 10};
  // Past the last spacing, more spacings out to a multiple of the distance
  // from the wire to the nearest plane (the substrate, or the nearest
  // cover below or above): `lateral_reach` with covers on both sides or
  // none, `lateral_reach_one_sided` with a cover on one side only, whose
  // open side lets the field spread further. Consecutive added spacings
  // differ by at most the ratio `lateral_reach_step`, which must be above
  // 1. A neighbor past the last spacing is looked up as none. 0 adds none;
  // the reaches must be finite and not negative.
  double lateral_reach = 6;
  double lateral_reach_one_sided = 10;
  double lateral_reach_step = 1.6;
  // Covering layers tabulated below the layer (plus the substrate alone)
  // and above it (plus none), nearest first. These and the far factors
  // below apply only when cover_tolerance is 0.
  int below_depth = 2;
  int above_depth = 2;
  // Past the depth, a few far layers get contexts too, at spaced
  // distances: for each factor, the usable layer whose distance from the
  // wire is nearest (in the logarithm) to the factor times the distance
  // of the first usable layer past the depth, and with any factor at all
  // the farthest usable layer, each layer once. A lookup for a cover
  // between two tabulated ones interpolates (CoverMap), never past the
  // farthest, so the table count grows with this list, not with the
  // layer count. Empty: no far layer gets a context.
  std::vector<double> far_cover_factors{1, 2, 4};
  // The covers are chosen per layer and side from 2D probes (planCovers):
  // the fewest layers, always the nearest and the farthest, such that
  // log-distance interpolation between them predicts every other layer's
  // total and plane coupling within this fraction of the total. 0 uses
  // the depths and far factors above instead.
  double cover_tolerance = 0.02;
  // The crossing patterns: crossing widths as multiples of the cover
  // layer's minimum width, coverage fractions, and the two wire lengths
  // whose difference cancels the ends.
  std::vector<double> crossing_width_factors{1, 2, 5};
  // Spacings to neighbors of the wire under the crossings, as multiples of
  // the layer's minimum spacing, with no neighbor besides.
  std::vector<double> crossing_spacing_factors{1, 2, 4};
  std::vector<double> coverages{0.1, 0.25, 0.5, 0.75};
  double crossing_length = 10;
  double crossing_length_long = 30;
  // The crossing reach: how far a single crossing of the cover's minimum
  // width runs past the wire, and the wire past it, as multiples of the
  // wider of the two layers' minimum widths; with a crossing running on
  // as far as the planes (at least twice the last) besides.
  std::vector<double> reach_factors{0, 2, 5, 12, 30};
  // The end patterns: a wire of this length, or five times its width if
  // longer.
  double end_length = 2;
  // The end tables' spacings to neighbors running past the end on both
  // sides, and gaps to a wire facing the end, as multiples of the layer's
  // minimum spacing; with no neighbor and no facing wire besides.
  std::vector<double> end_spacing_factors{1, 2, 4};
  std::vector<double> facing_factors{1, 2, 4};
  // The diagonal patterns: the parallel wire's widths as multiples of its
  // layer's minimum width, its edge-to-edge gaps as multiples of the
  // wire's minimum width, the spacings to the wire's own neighbor on the
  // parallel wire's side and on the far side as multiples of its minimum
  // spacing, and how many layers each way.
  std::vector<double> diagonal_width_factors{1, 3, 10};
  std::vector<double> diagonal_gap_factors{-0.5, 0, 0.25, 0.5, 1, 2, 4, 8, 16};
  // Past the last gap, more gaps out to this multiple of the vertical
  // distance between the two layers, consecutive ones at most twice apart;
  // a parallel wire farther out fades as one over its gap. 0 adds none; it
  // must be finite and not negative.
  double diagonal_reach = 4;
  std::vector<double> diagonal_spacing_factors{1, 2, 4, 8};
  std::vector<double> diagonal_far_spacing_factors{1, 4};
  int diagonal_depth = 2;
  // The solver's standard error, relative to the total, on the total and on
  // every coupling. Per-unit-length references are solved at a third of
  // it.
  double tol = 0.003;
  int threads = 1;
  uint64_t seed = 1;
  // Half-width of the covering planes, µm.
  double extent = 10;
  // Called after each solve with the solves done and the total.
  std::function<void(int done, int total)> progress;
};

// A fingerprint of everything the tables depend on: the stack's layers,
// dielectrics and vias with their numbers, and the options that shape the
// tables (the grids, depths, lengths, tolerance, seed and extent; not the
// threads). A saved model whose fingerprint differs was built from other
// data and must not be reused.
std::string buildFingerprint(const fsv::Stack& stack,
                             const BuildOptions& options);

// The covering contexts of a layer on each side: no layer first (the
// substrate below, open above), then the tabulated covers nearest first.
// A conductor sitting on the ground plane is the ground and does not
// count, nor does one outside the options' layers.
struct CoverPlan
{
  std::vector<Layer> below;
  std::vector<Layer> above;
};

// The covers to tabulate for a layer. Without a cover tolerance: the
// usable layers within the depth, then the far layers the factors pick
// and the farthest. With one: the fewest usable layers, always the
// nearest and the farthest, such that interpolating in the logarithm of
// the distance between consecutive ones (as CoverMap does) predicts every
// other usable layer within the tolerance. The prediction is checked on
// 2D probes of a wire with a plane on each candidate cover: at the
// narrowest and the widest width of the grid, alone and between
// neighbors at minimum spacing, with the other side open and with its
// nearest cover (when it has one); the total, the plane coupling, the
// ground and the neighbor coupling, each relative to the total.
// The probes are seeded by their position, so the plan does not depend on
// the thread count.
CoverPlan planCovers(const fsv::Stack& stack,
                     Layer layer,
                     const BuildOptions& options);
// One side of planCovers. Each call plans both sides, which with a cover
// tolerance costs probes; call planCovers once to get both.
std::vector<Layer> coversBelow(const fsv::Stack& stack,
                               Layer layer,
                               const BuildOptions& options);
std::vector<Layer> coversAbove(const fsv::Stack& stack,
                               Layer layer,
                               const BuildOptions& options);

// The probe curves of one side, for pickCovers: per quantity, a value and
// the scale its error is measured against, at each candidate.
struct ProbeCurve
{
  std::vector<double> value;
  std::vector<double> scale;
};

// The fewest of the candidates, given ascending by distance, that include
// the first and the last and predict every other candidate within `tol`
// (|interpolated - value| <= tol × scale) on every curve, interpolating
// linearly in the logarithm of the distance between consecutive picks.
// The indices of the picks, ascending; ties go to the earlier candidates.
std::vector<int> pickCovers(const std::vector<double>& distances,
                            const std::vector<ProbeCurve>& curves,
                            double tol);

// Builds the lateral tables of a conductor layer, one per covering context,
// into the model. Each table point is its own single-threaded solve, seeded
// by its position, so the tables do not depend on the thread count. A
// table the model already holds is left alone, as every builder here
// does, so a build resumes from a saved model; one on a grid other than
// the options' is an error, since it came from another build. Throws
// std::invalid_argument for that, or for a handle that is not a conductor
// of the stack.
void buildLateral(const fsv::Stack& stack,
                  Layer layer,
                  const BuildOptions& options,
                  Model& model);
// The same over a plan computed once for the layer. The overload above
// plans the covers itself, which with a cover tolerance costs probes on
// every call: a caller building several tables of a layer passes a plan.
void buildLateral(const fsv::Stack& stack,
                  Layer layer,
                  const CoverPlan& plan,
                  const BuildOptions& options,
                  Model& model);

// The crossing shares follow a kept amount over a gap g of
// L (1 - exp(-g / L)): a share of 1 at no gap, falling as L / g. One length
// L per spacing column, fitted to the solved shares, smooths their noise,
// which the plane's small effect beside close neighbors amplifies.
struct KeptFit
{
  double length = 0;    // L, µm
  double residual = 0;  // rms of the kept amounts' misfit, µm
};
// A fit is good when its residual is within this fraction of its length.
inline constexpr double kKeptFitTolerance = 0.15;
double keptShare(double length, double gap);
KeptFit fitKept(const std::vector<double>& gaps,
                const std::vector<double>& shares);

// Builds the crossing table of a layer and one covering layer: periodic
// arrays of crossing wires on the cover, over a wire of minimum width, at
// every crossing width and coverage of the options, and with neighbors
// at every crossing spacing; each array gives the share kept over the gap
// between its crossings, and the table holds them fitted (fitKept). Throws
// std::runtime_error when the coupling shares without neighbors fit no
// kept amount. The table without a plane opposite also holds the pair's
// reach (CrossingReach), from one crossing over a wire alone at every pair
// of the reach factors, against one running on and none; a held table
// without one gains it, and one with a reach on another grid is refused.
// Throws std::invalid_argument for reach factors that do not ascend from
// 0.
void buildCrossing(const fsv::Stack& stack,
                   Layer layer,
                   Layer cover,
                   const BuildOptions& options,
                   Model& model);
// The crossing tables of a layer and several covering layers, their solves
// in one pool; for a cover under the wire with a plane on `plane_above`
// too, and for one over it with a plane on `plane_below` (none when the
// handle names no layer). A table with a plane whose shares fit no kept
// amount holds the values of the table without the plane, and no reach.
void buildCrossings(const fsv::Stack& stack,
                    Layer layer,
                    const std::vector<Layer>& covers,
                    const BuildOptions& options,
                    Model& model,
                    Layer plane_below = Layer(),
                    Layer plane_above = Layer());

// Builds the diagonal table of a layer and a nearby layer: a wire of
// minimum width with a neighbor at a spacing on each side, and a parallel
// wire on the other layer beside it on the right, at every width, gap and
// pair of spacings of the options; with a plane on `opposite` when it
// names a layer, which must lie on the wire's other side. Each layer's
// build tabulates a parallel wire with no plane and with one on the
// nearest cover on the other side.
void buildDiagonal(const fsv::Stack& stack,
                   Layer layer,
                   Layer other,
                   const BuildOptions& options,
                   Model& model,
                   Layer opposite = Layer());

// The layers within the diagonal depth of a layer, below and above, that
// do not sit on the ground plane.
std::vector<Layer> diagonalLayers(const fsv::Stack& stack,
                                  Layer layer,
                                  const BuildOptions& options);

// Builds the end tables of a layer for the same contexts as its lateral
// tables: what one line end adds, over the lateral grid's widths, the end
// spacings and the facing gaps.
void buildEnds(const fsv::Stack& stack,
               Layer layer,
               const BuildOptions& options,
               Model& model);
void buildEnds(const fsv::Stack& stack,
               Layer layer,
               const CoverPlan& plan,
               const BuildOptions& options,
               Model& model);

// All of a layer's tables: lateral and end tables for every covering
// context of its plan (planCovers, computed once), a crossing table for
// every cover in the plan, and a diagonal table for every layer within the
// diagonal depth. Tables the model already holds are left alone, so a
// changed plan adds what it lacks.
void buildLayer(const fsv::Stack& stack,
                Layer layer,
                const BuildOptions& options,
                Model& model);
// The same over a given plan, such as heldCovers gives.
void buildLayer(const fsv::Stack& stack,
                Layer layer,
                const CoverPlan& plan,
                const BuildOptions& options,
                Model& model);

// The plan a model's own contexts for a layer make, or none when it holds
// no lateral table of the layer. A build resuming a saved model, which
// holds whole layers, takes it instead of probing again; it is the plan
// for the current options only when the model was built with them, which
// the caller checks (buildFingerprint).
std::optional<CoverPlan> heldCovers(const fsv::Stack& stack,
                                    const Model& model,
                                    Layer layer);

}  // namespace pex
