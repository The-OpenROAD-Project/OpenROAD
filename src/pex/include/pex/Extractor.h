// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Extraction of a routed block into ODB parasitics: the block's boxes read
// once, each signal net's resistor network and capacitance found in
// parallel into plain structs, and ODB written alone, in net order.

#pragma once

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "fsv/BlockShapes.h"
#include "fsv/Stack.h"
#include "pex/Evaluator.h"
#include "pex/Index.h"
#include "pex/Model.h"
#include "pex/Network.h"

namespace odb {
class dbBlock;
class dbNet;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace pex {

struct ExtractOptions
{
  // A run of wire shapes joins into one resistor up to this, ohms; 0 makes
  // every shape its own.
  double max_res = 50;
  // A net pair's coupling goes to ground on both nets when its total is
  // below coupling_threshold fF and below coupling_ratio of each net's
  // total capacitance.
  double coupling_threshold = 0.1;
  double coupling_ratio = 0.03;
  // Worker threads; 0 runs everything on the calling thread.
  int threads = 0;
  EvaluatorOptions evaluator;
};

// Where a box came from: the shape's source, and for a wire its dbWire
// shape id, for a pin its terminal's id.
struct BoxOrigin
{
  fsv::BlockShape::Source source;
  int shape_id = -1;
  int iterm = -1;
  int bterm = -1;
};

// The conductor boxes of a block as extraction sees them: every net's
// shapes, then the obstructions trimmed clear of them as one grounded net.
struct BlockBoxes
{
  std::vector<odb::dbNet*> nets;  // by NetIndex
  NetIndex obstructions;          // the grounded net, after the last
  std::vector<EvalBox> boxes;
  std::vector<BoxOrigin> origins;           // per box
  std::vector<std::vector<int>> net_boxes;  // per net, ascending
  int trimmed = 0;                          // obstructions a net cut
  int skipped = 0;  // shapes on layers neither conductor nor cut
  // The routing layers with shapes that the stack has no conductor for.
  std::vector<std::string> unknown_layers;
};

// Reads every shape of `block` onto the evaluator's layers by name; a cut
// layer's boxes are vias (layer -1). Boxes without area are dropped.
BlockBoxes readBlockBoxes(odb::dbBlock* block,
                          const fsv::Stack& stack,
                          const Evaluator& evaluator);

// The candidates of each box from an index of the block's boxes on
// conductor layers, through Evaluator::reach.
class BlockIndex
{
 public:
  // Builds the index on `threads` threads.
  BlockIndex(const BlockBoxes& boxes, const Evaluator& evaluator, int threads);

  // Evaluator::Candidates for `box`; `rects` and `ids` are scratch.
  void candidates(int box,
                  std::vector<odb::Rect>& rects,
                  std::vector<int>& ids,
                  std::vector<int>& out) const;

 private:
  const BlockBoxes& boxes_;
  const Evaluator& evaluator_;
  std::vector<int> indexed_;  // box by index id
  Index index_;
};

// A net's parasitics before they are written: ground capacitance on each
// node of its network and coupling by (own node, other net, other node),
// in fF.
struct NetParasitics
{
  Network network;
  std::vector<double> ground;
  std::map<std::tuple<int, NetIndex, int>, double> couplings;
};

struct ExtractStats
{
  int nets = 0;  // extracted
  int boxes = 0;
  int trimmed = 0;
  int skipped = 0;
  std::vector<std::string> unknown_layers;  // as in BlockBoxes
  int loops = 0;           // resistors that would join a node to itself
  int unplaced = 0;        // pin and special-wire boxes on no node of their own
  int unreached = 0;       // pins of extracted nets on none of their wire
  int coupled_pairs = 0;   // net pairs kept as coupling capacitors
  int grounded_pairs = 0;  // net pairs below both thresholds
};

class Extractor
{
 public:
  Extractor(odb::dbBlock* block,
            const fsv::Stack& stack,
            const Model& model,
            utl::Logger* logger,
            const ExtractOptions& options = {});

  // Replaces the block's parasitics with one corner extracted from its
  // signal nets that have wires; other nets are neighbors only, and
  // coupling to them goes to ground. Results are identical for any number
  // of threads.
  ExtractStats extract();

  // After extract(): the boxes read and each extracted net's parasitics,
  // empty for a net not extracted.
  const BlockBoxes& boxes() const { return boxes_; }
  const std::vector<NetParasitics>& parasitics() const { return parasitics_; }

 private:
  // Node pairs by net pair (low, high), each value a coupling in fF.
  using Pairs = std::map<std::pair<NetIndex, NetIndex>,
                         std::map<std::pair<int, int>, double>>;

  // Throws std::invalid_argument naming the layers the nets in `work` have
  // boxes on that the model has no tables for.
  void checkModel(const std::vector<NetIndex>& work) const;
  // The pins of `net` that are not nodes of its network.
  static int unreachedPins(odb::dbNet* net, const Network& network);
  NetParasitics evaluateNet(NetIndex net,
                            const std::vector<Network>& networks,
                            const BlockIndex& index,
                            int& unplaced) const;
  // Averages each coupling seen from both nets, and grounds the net pairs
  // below the thresholds; returns the pairs kept.
  Pairs combine(ExtractStats& stats);
  void commit(const Pairs& pairs) const;

  odb::dbBlock* block_;
  const fsv::Stack& stack_;
  const Model& model_;
  utl::Logger* logger_;
  const ExtractOptions options_;
  Evaluator evaluator_;
  BlockBoxes boxes_;
  std::vector<bool> extracted_;
  std::vector<NetParasitics> parasitics_;
};

}  // namespace pex
