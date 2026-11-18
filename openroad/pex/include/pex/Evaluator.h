// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The model applied to conductor boxes: every box of a target net is cut
// into pieces wherever its nearest neighbor or cover changes, and each
// piece looks up the tables. Each part of the result names the target box
// it belongs to and the box it couples to, so the extractor can place it
// in the net's RC network. The boxes around a target box come from the
// caller: an index in extraction, or every box of a solver geometry in the
// brute-force estimate below, which the experiments use and against which
// extraction is checked.

#pragma once

#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "odb/geom.h"
#include "pex/CoverMap.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {

struct EvaluatorOptions
{
  // A box on another layer covers a piece when it overlaps at least this
  // fraction of the wire's width.
  double cover_overlap = 0.5;
  // Layers each way whose parallel wires beside a piece count.
  int diagonal_depth = 2;
};

// Where a coupling comes from: a piece's lateral neighbor, its cover (a
// crossing's share included), a farther cover behind crossings, a parallel
// wire on a nearby layer, and at a line end the neighbors, the facing wire,
// the cover and the farther cover.
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

// Where a ground capacitance comes from: the pieces, the parallel wires,
// the line ends and corners.
enum class Ground
{
  kPiece,
  kDiagonal,
  kEnd,
  kCount
};

// A conductor box: its net, its layer (an index into Evaluator's layers,
// which ascend by height; -1 for a via) and its x-y extent in database
// units.
struct EvalBox
{
  NetIndex net;
  int layer;
  odb::Rect rect;
};

// One part of a target box's capacitance, in fF: to ground when `other` is
// negative, else coupling to box `other`. `at` is where along the target
// box's long axis it lies (a piece's middle, an end's face), in database
// units; a middle may lie halfway between two.
struct Contribution
{
  int box;
  int other;
  Source source;  // a coupling's
  Ground ground;  // a ground part's
  double value;
  double at;
};

// Counts of what a net's boxes held.
struct EvalCounts
{
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
};

// Positions are compared exactly in database units, `dbu` per µm, and
// converted to µm for the model's lookups.
class Evaluator
{
 public:
  // Throws std::invalid_argument for a model layer the stack lacks or a
  // `dbu` below 1.
  Evaluator(const fsv::Stack& stack,
            const Model& model,
            int dbu,
            const EvaluatorOptions& options = {});

  // The stack's conductors ascending by bottom.
  int layerCount() const { return static_cast<int>(layers_.size()); }
  const std::string& layerName(int layer) const { return names_[layer]; }
  // The layer whose z extent `box` has, -1 for none.
  int layerOf(const fsv::Box& box) const;
  int dbu() const { return dbu_; }
  // How far around a box of `layer` the boxes that can change its lookups
  // reach, in x-y and database units: the farthest last spacing, gap or
  // reach of the layer's tables, to the nearest unit. Boxes beyond it are
  // ignored.
  int radius(int layer) const { return radius_[layer]; }

  // Gives, for box `box`, ascending indices of the boxes within
  // radius(layer of box) of it in x-y on every layer; it may give more.
  using Candidates = std::function<void(int box, std::vector<int>& out)>;

  // Adds the contributions of the net whose boxes are `target` (indices
  // into `boxes`, ascending; its vias included) to `out`, in a fixed order.
  // Throws std::out_of_range for a context the model has no table for,
  // after falling back to the contexts with fewer covers.
  void evaluate(const std::vector<EvalBox>& boxes,
                const std::vector<int>& target,
                const Candidates& candidates,
                std::vector<Contribution>& out,
                EvalCounts* counts = nullptr) const;

 private:
  const fsv::Stack& stack_;
  const Model& model_;
  const EvaluatorOptions options_;
  const int dbu_;
  std::vector<fsv::Stack::Layer> layers_;
  std::vector<std::string> names_;
  std::vector<CoverMap> maps_;  // per layer
  // In database units.
  std::vector<int> radius_;
  // Per layer, the farthest lateral spacing, crossing gap and facing gap.
  std::vector<int> lateral_reach_;
  std::vector<int> crossing_reach_;
  std::vector<int> facing_reach_;
  // Per (layer, other layer), the diagonal table's last gap; 0 for none.
  std::vector<std::vector<int>> diagonal_reach_;
};

// A net's capacitance by brute force: the contributions summed.
struct NetEstimate : EvalCounts
{
  double ground = 0;  // to the ground plane, fF
  // The ground by source.
  std::array<double, static_cast<size_t>(Ground::kCount)> ground_sources{};
  std::map<NetIndex, double> couplings;  // by net; the target excluded
  // The couplings by source.
  std::map<NetIndex, std::array<double, static_cast<size_t>(Source::kCount)>>
      sources;
  int vias = 0;  // boxes on via layers, which add nothing yet

  double total() const;
};

// The database units per µm estimate() rounds a geometry's x-y to: exact
// for layout read from a database at 1000, 2000 or 4000 per µm, fine enough
// (0.01 nm) that patterns off any grid lose nothing measurable, and ±21 mm
// of range.
constexpr int kEstimateDbu = 100000;

// Estimates a target net of a geometry whose boxes lie on the stack's
// layers, with every box of the geometry as candidates, its x-y rounded to
// `dbu` units per µm. Boxes between layers are vias. Throws
// std::invalid_argument for a box on no layer or out of range, and
// std::out_of_range as Evaluator::evaluate does.
NetEstimate estimate(const fsv::Geometry& geometry,
                     const fsv::Stack& stack,
                     const Model& model,
                     NetIndex target,
                     const EvaluatorOptions& options = {},
                     int dbu = kEstimateDbu);

}  // namespace pex
