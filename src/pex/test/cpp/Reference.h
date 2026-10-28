// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The model applied to a field-solver geometry by brute force: every box of
// a target net is cut into pieces wherever its nearest neighbor or cover
// changes, and each piece looks up the tables. It is the reference the
// extractor's index and segmenter are checked against, and the way the
// experiments measure the model on real nets.

#pragma once

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

struct NetEstimate
{
  double ground = 0;                     // to the ground plane, fF
  std::map<NetIndex, double> couplings;  // by net; the target excluded
  int wires = 0;                         // boxes on conductor layers
  int pieces = 0;
  int ends = 0;  // line ends
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
