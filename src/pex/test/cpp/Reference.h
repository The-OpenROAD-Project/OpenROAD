// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The model applied to a field-solver geometry by brute force: every box of
// a target net is cut into pieces wherever its nearest neighbor or cover
// changes, and each piece looks up the tables. It is the reference the
// extractor's index and segmenter are checked against, and the way the
// experiments measure the model on real nets.

#pragma once

#include <array>
#include <map>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {

struct ReferenceOptions
{
  // A box on another layer covers a piece when it overlaps at least this
  // fraction of the wire's width.
  double cover_overlap = 0.5;
  // Layers each way whose parallel wires beside a piece count.
  int diagonal_depth = 2;
};

// Where an estimate's coupling comes from: a piece's lateral neighbor, its
// cover (a crossing's share included), a farther cover behind crossings, a
// parallel wire on a nearby layer, and at a line end the neighbors, the
// facing wire, the cover and the farther cover.
enum class Source
{
  kLateral,
  kCover,
  kRest,
  kDiagonal,
  kEndLateral,
  kEndFacing,
  kEndCover,
  kEndRest,
  kCount
};

struct NetEstimate
{
  double ground = 0;  // to the ground plane, fF
  // The ground by source: the pieces, the parallel wires, the line ends.
  std::array<double, 3> ground_sources{};
  std::map<NetIndex, double> couplings;  // by net; the target excluded
  // The couplings by source.
  std::map<NetIndex, std::array<double, static_cast<size_t>(Source::kCount)>>
      sources;
  int wires = 0;  // boxes on conductor layers
  int pieces = 0;
  int ends = 0;         // line ends
  int via_ends = 0;     // of them, those the net's via or other layers reach
  int corners = 0;      // wire ends at a bend, which add nothing
  int facing_ends = 0;  // line ends facing a shape across a tabulated gap
  // Perpendicular wires of the net on one layer overlapping: at both
  // wires' ends, at one's end, and at neither.
  int bends = 0;
  int tees = 0;
  int crosses = 0;
  int vias = 0;  // boxes on via layers, which add nothing yet

  double total() const;
};

// Estimates a target net of a geometry whose boxes lie on the stack's
// layers. Boxes between layers are vias. Throws std::invalid_argument for a
// box on no layer, and std::out_of_range for a context the model has no
// table for, after falling back to the contexts with fewer covers.
NetEstimate estimate(const fsv::Geometry& geometry,
                     const fsv::Stack& stack,
                     const Model& model,
                     NetIndex target,
                     const ReferenceOptions& options = {});

}  // namespace pex
