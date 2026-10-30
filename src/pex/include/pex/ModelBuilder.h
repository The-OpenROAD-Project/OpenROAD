// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Builds model tables from a process stack by solving patterns with fsv.

#pragma once

#include <cstdint>
#include <functional>
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
  // Covering layers tabulated below the layer (plus the substrate alone)
  // and above it (plus none), nearest first.
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
  // The crossing patterns: crossing widths as multiples of the cover
  // layer's minimum width, coverage fractions, and the two wire lengths
  // whose difference cancels the ends.
  std::vector<double> crossing_width_factors{1, 2, 5};
  std::vector<double> coverages{0.1, 0.25, 0.5, 0.75};
  double crossing_length = 10;
  double crossing_length_long = 30;
  // The end patterns: a wire of this length, or five times its width if
  // longer.
  double end_length = 2;
  // The diagonal patterns: the parallel wire's widths as multiples of its
  // layer's minimum width, its edge-to-edge gaps as multiples of the
  // wire's minimum width, the spacings to the wire's own neighbor on the
  // parallel wire's side and on the far side as multiples of its minimum
  // spacing, and how many layers each way.
  std::vector<double> diagonal_width_factors{1, 3, 10};
  std::vector<double> diagonal_gap_factors{-0.5, 0, 0.25, 0.5, 1, 2, 4, 8, 16};
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

// The covering contexts of a layer: no layer first (the substrate below,
// open above), then the usable layers within the depth nearest first,
// then the far layers the options' factors pick and the farthest, nearest
// first. A conductor sitting on the ground plane is the ground and does
// not count, nor does one outside the options' layers.
std::vector<Layer> coversBelow(const fsv::Stack& stack,
                               Layer layer,
                               const BuildOptions& options);
std::vector<Layer> coversAbove(const fsv::Stack& stack,
                               Layer layer,
                               const BuildOptions& options);

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

// Builds the crossing table of a layer and one covering layer: periodic
// arrays of crossing wires on the cover, over a wire of minimum width, at
// every crossing width and coverage of the options; each array gives the
// share kept over the gap between its crossings.
void buildCrossing(const fsv::Stack& stack,
                   Layer layer,
                   Layer cover,
                   const BuildOptions& options,
                   Model& model);

// Builds the diagonal table of a layer and a nearby layer: a wire of
// minimum width with a neighbor at a spacing on each side, and a parallel
// wire on the other layer beside it on the right, at every width, gap and
// pair of spacings of the options.
void buildDiagonal(const fsv::Stack& stack,
                   Layer layer,
                   Layer other,
                   const BuildOptions& options,
                   Model& model);

// The layers within the diagonal depth of a layer, below and above, that
// do not sit on the ground plane.
std::vector<Layer> diagonalLayers(const fsv::Stack& stack,
                                  Layer layer,
                                  const BuildOptions& options);

// Builds the end tables of a layer for the same contexts as its lateral
// tables: what one line end adds, over the lateral grid's widths.
void buildEnds(const fsv::Stack& stack,
               Layer layer,
               const BuildOptions& options,
               Model& model);

// All of a layer's tables: lateral and end tables, a crossing table for
// every cover within the depths, and a diagonal table for every layer
// within the diagonal depth.
void buildLayer(const fsv::Stack& stack,
                Layer layer,
                const BuildOptions& options,
                Model& model);

}  // namespace pex
