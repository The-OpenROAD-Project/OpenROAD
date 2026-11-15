// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Extractor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "fsv/BlockShapes.h"
#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "odb/db.h"
#include "odb/dbExtControl.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/normalizeWireConnectivity.h"
#include "pex/Evaluator.h"
#include "pex/Index.h"
#include "pex/Model.h"
#include "pex/Network.h"
#include "pex/Patterns.h"
#include "pex/Resistance.h"
#include "utl/ThreadPool.h"

namespace pex {

namespace {

// The point of a box at `at` along its long axis, on its center line.
odb::Point pointAt(const EvalBox& box, const double at)
{
  const int along = static_cast<int>(std::lround(at));
  if (box.rect.dx() >= box.rect.dy()) {
    return {along, box.rect.yCenter()};
  }
  return {box.rect.xCenter(), along};
}

// The node of a place nearer to it: a resistor's source or target.
int nearer(const NetPlace& place, const Network& network)
{
  if (place.resistor < 0) {
    return place.node;
  }
  const NetResistor& r = network.resistors[place.resistor];
  return place.t < 0.5 ? r.source : r.target;
}

// Splits a ground capacitance between a resistor's ends by where it lies.
void addGround(const NetPlace& place,
               const double value,
               const Network& network,
               std::vector<double>& ground)
{
  if (place.resistor < 0) {
    ground[place.node] += value;
    return;
  }
  const NetResistor& r = network.resistors[place.resistor];
  ground[r.source] += (1 - place.t) * value;
  ground[r.target] += place.t * value;
}

// The id a BoxOrigin keeps for a net shape: see BoxOrigin.
int originId(const fsv::BlockShape& shape)
{
  switch (shape.source) {
    case fsv::BlockShape::Source::kWire:
      return shape.shape_id;
    case fsv::BlockShape::Source::kITermPin:
      return static_cast<int>(shape.iterm->getId());
    case fsv::BlockShape::Source::kBTermPin:
      return static_cast<int>(shape.bterm->getId());
    default:
      return -1;
  }
}

}  // namespace

BlockBoxes readBlockBoxes(odb::dbBlock* block,
                          const fsv::Stack& stack,
                          const Evaluator& evaluator)
{
  BlockBoxes result;
  const double dbu = block->getDbUnitsPerMicron();
  std::map<std::string, int> layers;
  std::vector<fsv::Stack::ZExtent> z;
  for (int l = 0; l < evaluator.layerCount(); l++) {
    layers[evaluator.layerName(l)] = l;
    z.push_back(stack.zExtent(stack.layer(evaluator.layerName(l))));
  }
  // The layer of a shape; -1 for a cut, -2 for neither.
  std::set<std::string> unknown;
  auto layerOf = [&](odb::dbTechLayer* layer) {
    const auto it = layers.find(layer->getName());
    if (it != layers.end()) {
      return it->second;
    }
    if (layer->getType() == odb::dbTechLayerType::ROUTING) {
      unknown.insert(layer->getName());
    }
    return layer->getType() == odb::dbTechLayerType::CUT ? -1 : -2;
  };
  auto toBox = [&](const odb::Rect& r, const int layer) {
    return fsv::Box{
        .lo = {r.xMin() / dbu, r.yMin() / dbu, z[layer].bottom},
        .hi = {r.xMax() / dbu, r.yMax() / dbu, z[layer].top},
    };
  };
  // The net boxes on conductor layers, which trim the obstructions.
  std::vector<fsv::Box> net_boxes;
  for (odb::dbNet* net : block->getNets()) {
    const NetIndex index = static_cast<NetIndex>(result.nets.size());
    result.nets.push_back(net);
    result.net_boxes.emplace_back();
    for (const fsv::BlockShape& shape : fsv::netShapes(net)) {
      const int layer = layerOf(shape.layer);
      if (layer < -1) {
        result.skipped++;
        continue;
      }
      if (shape.rect.area() == 0) {
        continue;
      }
      result.net_boxes.back().push_back(static_cast<int>(result.boxes.size()));
      result.boxes.push_back(
          {.net = index, .layer = layer, .rect = shape.rect});
      result.origins.push_back({.source = shape.source, .id = originId(shape)});
      if (layer >= 0) {
        net_boxes.push_back(toBox(shape.rect, layer));
      }
    }
  }
  result.obstructions = static_cast<NetIndex>(result.nets.size());
  std::vector<fsv::Obstruction> obstructions;
  for (const fsv::BlockShape& shape : fsv::unconnectedShapes(block)) {
    const int layer = layerOf(shape.layer);
    if (layer >= 0 && shape.rect.area() > 0) {
      obstructions.push_back(
          fsv::obstruction(shape, toBox(shape.rect, layer), dbu));
    }
  }
  for (const fsv::Box& piece :
       fsv::trimObstructions(net_boxes, obstructions, result.trimmed)) {
    const odb::Rect rect(static_cast<int>(std::lround(piece.lo[0] * dbu)),
                         static_cast<int>(std::lround(piece.lo[1] * dbu)),
                         static_cast<int>(std::lround(piece.hi[0] * dbu)),
                         static_cast<int>(std::lround(piece.hi[1] * dbu)));
    if (rect.area() == 0) {
      continue;
    }
    result.boxes.push_back({
        .net = result.obstructions,
        .layer = evaluator.layerOf(piece),
        .rect = rect,
    });
    result.origins.push_back({.source = fsv::BlockShape::Source::kObstruction});
  }
  result.unknown_layers.assign(unknown.begin(), unknown.end());
  return result;
}

namespace {

std::vector<IndexBox> conductorBoxes(const BlockBoxes& boxes,
                                     std::vector<int>& indexed)
{
  std::vector<IndexBox> result;
  for (int i = 0; std::cmp_less(i, boxes.boxes.size()); i++) {
    const EvalBox& b = boxes.boxes[i];
    if (b.layer >= 0) {
      result.push_back({.layer = b.layer, .rect = b.rect});
      indexed.push_back(i);
    }
  }
  return result;
}

}  // namespace

// Vias add nothing to other nets and stay out of the index.
BlockIndex::BlockIndex(const BlockBoxes& boxes,
                       const Evaluator& evaluator,
                       const int threads)
    : boxes_(boxes),
      evaluator_(evaluator),
      index_(conductorBoxes(boxes, indexed_), evaluator.layerCount(), threads)
{
}

void BlockIndex::candidates(const int box,
                            std::vector<odb::Rect>& rects,
                            std::vector<int>& ids,
                            std::vector<int>& out) const
{
  evaluator_.reach(boxes_.boxes[box], rects);
  ids.clear();
  for (int l = 0; std::cmp_less(l, rects.size()); l++) {
    index_.query(l, rects[l], ids);
  }
  std::ranges::sort(ids);
  for (const int i : ids) {
    out.push_back(indexed_[i]);
  }
}

Extractor::Extractor(odb::dbBlock* block,
                     const fsv::Stack& stack,
                     const Model& model,
                     utl::Logger* logger,
                     const ExtractOptions& options)
    : block_(block),
      stack_(stack),
      model_(model),
      logger_(logger),
      options_(options),
      evaluator_(stack, model, block->getDbUnitsPerMicron(), options.evaluator)
{
}

ExtractStats Extractor::extract()
{
  ExtractStats stats;
  const Resistance resistance(stack_, block_->getTech());
  // Terminals on the wire paths; this renumbers the wires' shapes, so it
  // comes before they are read.
  odb::normalizeWireConnectivity(logger_, block_);
  boxes_ = readBlockBoxes(block_, stack_, evaluator_);
  stats.boxes = static_cast<int>(boxes_.boxes.size());
  stats.trimmed = boxes_.trimmed;
  stats.skipped = boxes_.skipped;
  stats.unknown_layers = boxes_.unknown_layers;

  const BlockIndex index(boxes_, evaluator_, options_.threads);

  const size_t net_count = boxes_.nets.size();
  extracted_.assign(net_count + 1, false);
  std::vector<NetIndex> work;
  for (size_t n = 0; n < net_count; n++) {
    odb::dbNet* net = boxes_.nets[n];
    if (!net->getSigType().isSupply() && net->getWire() != nullptr) {
      extracted_[n] = true;
      work.push_back(static_cast<NetIndex>(n));
    }
  }
  stats.nets = static_cast<int>(work.size());
  checkModel(work);

  utl::ThreadPool pool(options_.threads);
  std::vector<Network> networks(net_count);
  pool.parallelFor(work, [&](const NetIndex n) {
    networks[n] = buildNetwork(boxes_.nets[n], resistance, options_.max_res);
  });
  // A wire with no path makes no network; the net is a neighbor only.
  std::erase_if(work, [&](const NetIndex n) {
    if (networks[n].nodes.empty()) {
      extracted_[n] = false;
      return true;
    }
    return false;
  });
  stats.nets = static_cast<int>(work.size());
  for (const NetIndex n : work) {
    stats.unreached += unreachedPins(boxes_.nets[n], networks[n]);
  }
  parasitics_.assign(net_count, {});
  std::vector<int> unplaced(net_count, 0);
  pool.parallelFor(work, [&](const NetIndex n) {
    parasitics_[n] = evaluateNet(n, networks, index, unplaced[n]);
  });
  for (const NetIndex n : work) {
    stats.loops += networks[n].loops;
    stats.unplaced += unplaced[n];
    parasitics_[n].network = std::move(networks[n]);
  }

  const Pairs pairs = combine(stats);
  commit(pairs);
  return stats;
}

void Extractor::checkModel(const std::vector<NetIndex>& work) const
{
  std::set<int> used;
  for (const NetIndex n : work) {
    for (const int b : boxes_.net_boxes[n]) {
      used.insert(boxes_.boxes[b].layer);
    }
  }
  std::string missing;
  for (const int l : used) {
    if (l >= 0
        && model_.lateral(LateralKey{
               .layer = evaluator_.layerName(l),
               .below = "",
               .above = "",
           }) == nullptr) {
      missing += (missing.empty() ? "" : ", ") + evaluator_.layerName(l);
    }
  }
  if (!missing.empty()) {
    throw std::invalid_argument("the model has no tables for " + missing
                                + ", which signal nets are routed on");
  }
}

int Extractor::unreachedPins(odb::dbNet* net, const Network& network)
{
  int count = 0;
  for (odb::dbITerm* iterm : net->getITerms()) {
    if (!network.iterms.contains(static_cast<int>(iterm->getId()))) {
      count++;
    }
  }
  for (odb::dbBTerm* bterm : net->getBTerms()) {
    if (!network.bterms.contains(static_cast<int>(bterm->getId()))) {
      count++;
    }
  }
  return count;
}

NetParasitics Extractor::evaluateNet(const NetIndex net,
                                     const std::vector<Network>& networks,
                                     const BlockIndex& index,
                                     int& unplaced) const
{
  const Network& network = networks[net];
  NetParasitics result;
  result.ground.assign(network.nodes.size(), 0);
  if (network.nodes.empty()) {
    return result;
  }
  // Where a point of a box lies in its net's network: on its wire shape's
  // resistor, or on its terminal's node; else on the first node.
  auto placeOf = [&](const int box,
                     const odb::Point point,
                     const Network& on,
                     int* missed) {
    const BoxOrigin& o = boxes_.origins[box];
    switch (o.source) {
      case fsv::BlockShape::Source::kWire:
        if (on.shapes.contains(o.id)) {
          return on.place(o.id, point);
        }
        break;
      case fsv::BlockShape::Source::kITermPin:
      case fsv::BlockShape::Source::kBTermPin: {
        const auto& terms = o.source == fsv::BlockShape::Source::kITermPin
                                ? on.iterms
                                : on.bterms;
        const auto it = terms.find(o.id);
        if (it != terms.end()) {
          return NetPlace{.resistor = -1, .node = it->second, .t = 0};
        }
        break;
      }
      default:
        break;
    }
    if (missed != nullptr) {
      (*missed)++;
    }
    return NetPlace{.resistor = -1, .node = 0, .t = 0};
  };

  std::vector<Contribution> parts;
  std::vector<odb::Rect> rects;
  std::vector<int> ids;
  evaluator_.evaluate(
      boxes_.boxes,
      boxes_.net_boxes[net],
      [&](const int box, std::vector<int>& out) {
        index.candidates(box, rects, ids, out);
      },
      parts);

  // Pins and special wires are placed once per box, not per part.
  std::map<int, bool> missed_boxes;
  for (const Contribution& c : parts) {
    const odb::Point point = pointAt(boxes_.boxes[c.box], c.at);
    int missed = 0;
    const NetPlace own = placeOf(c.box, point, network, &missed);
    if (missed > 0) {
      missed_boxes[c.box] = true;
    }
    const NetIndex other
        = c.other < 0 ? kNullNetIndex : boxes_.boxes[c.other].net;
    if (other == kNullNetIndex || !extracted_[other]) {
      addGround(own, c.value, network, result.ground);
      continue;
    }
    const Network& theirs = networks[other];
    const NetPlace far
        = placeOf(c.other,
                  boxes_.boxes[c.other].rect.closestPtInside(point),
                  theirs,
                  nullptr);
    result.couplings[{nearer(own, network), other, nearer(far, theirs)}]
        += c.value;
  }
  unplaced = static_cast<int>(missed_boxes.size());
  return result;
}

Extractor::Pairs Extractor::combine(ExtractStats& stats)
{
  Pairs pairs;
  for (size_t n = 0; n < parasitics_.size(); n++) {
    const NetIndex own = static_cast<NetIndex>(n);
    for (const auto& [key, value] : parasitics_[n].couplings) {
      const auto [own_node, other, other_node] = key;
      if (own < other) {
        pairs[{own, other}][{own_node, other_node}] += value / 2;
      } else {
        pairs[{other, own}][{other_node, own_node}] += value / 2;
      }
    }
  }
  std::vector<double> totals(parasitics_.size(), 0);
  for (size_t n = 0; n < parasitics_.size(); n++) {
    for (const double g : parasitics_[n].ground) {
      totals[n] += g;
    }
  }
  for (const auto& [nets, nodes] : pairs) {
    for (const auto& [ends, value] : nodes) {
      totals[nets.first] += value;
      totals[nets.second] += value;
    }
  }
  Pairs kept;
  for (auto& [nets, nodes] : pairs) {
    double sum = 0;
    for (const auto& [ends, value] : nodes) {
      sum += value;
    }
    const auto [low, high] = nets;
    if (!options_.coupling_to_ground
        && (sum >= options_.coupling_threshold
            || sum >= options_.coupling_ratio * totals[low]
            || sum >= options_.coupling_ratio * totals[high])) {
      stats.coupled_pairs++;
      kept.emplace(nets, std::move(nodes));
      continue;
    }
    stats.grounded_pairs++;
    for (const auto& [ends, value] : nodes) {
      parasitics_[low].ground[ends.first] += value;
      parasitics_[high].ground[ends.second] += value;
    }
  }
  return kept;
}

void Extractor::commit(const Pairs& pairs) const
{
  // Clears every parasitic of the block, but not the nets' flags.
  block_->setCornerCount(1);
  // The ground capacitance is on the cap nodes ("foreign"), not on the
  // resistors, which ODB's net totals must know.
  odb::dbExtControl* control = block_->getExtControl();
  control->_foreign = true;
  control->_extracted = true;
  for (odb::dbNet* net : boxes_.nets) {
    net->setRCgraph(false);
  }
  std::vector<std::vector<odb::dbCapNode*>> caps(parasitics_.size());
  for (size_t n = 0; n < parasitics_.size(); n++) {
    if (!extracted_[n]) {
      continue;
    }
    odb::dbNet* net = boxes_.nets[n];
    const NetParasitics& p = parasitics_[n];
    for (size_t i = 0; i < p.network.nodes.size(); i++) {
      const NetNode& node = p.network.nodes[i];
      odb::dbCapNode* cap = odb::dbCapNode::create(net, 0, /*foreign=*/true);
      switch (node.kind) {
        case NetNode::Kind::kJunction:
          cap->setInternalFlag();
          break;
        case NetNode::Kind::kITerm:
          cap->setITermFlag();
          break;
        case NetNode::Kind::kBTerm:
          cap->setBTermFlag();
          break;
      }
      cap->setNode(node.id);
      cap->setCapacitance(p.ground[i]);
      caps[n].push_back(cap);
    }
    // dbRSeg::create puts each segment at the head of the net's list, and
    // iteration skips the head: the segments go in reverse path order and a
    // placeholder last, so they iterate from the path's start.
    const std::vector<NetResistor>& resistors = p.network.resistors;
    for (const NetResistor& r : std::views::reverse(resistors)) {
      const odb::Point at = p.network.nodes[r.target].point;
      odb::dbRSeg* seg
          = odb::dbRSeg::create(net, at.x(), at.y(), 0, /*allocate_cap=*/false);
      seg->setSourceNode(caps[n][r.source]->getId());
      seg->setTargetNode(caps[n][r.target]->getId());
      seg->setResistance(r.resistance);
    }
    if (!caps[n].empty()) {
      const int first = resistors.empty() ? 0 : resistors.front().source;
      const odb::Point at = p.network.nodes[first].point;
      odb::dbRSeg* head
          = odb::dbRSeg::create(net, at.x(), at.y(), 0, /*allocate_cap=*/false);
      head->setTargetNode(caps[n][first]->getId());
    }
    net->setRCgraph(true);
  }
  for (const auto& [nets, nodes] : pairs) {
    for (const auto& [ends, value] : nodes) {
      odb::dbCCSeg* cc = odb::dbCCSeg::create(caps[nets.first][ends.first],
                                              caps[nets.second][ends.second],
                                              /*mergeParallel=*/false);
      cc->setCapacitance(value);
    }
  }
}

}  // namespace pex
