// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Sequential mapper: converts Dff instances to Target cells
// by matching flip-flop features to library cell capabilities.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "syn/synthesis.h"
#include "utl/Logger.h"

namespace syn {

struct FeatureSet
{
  bool clock_polarity = true;
  bool has_clear = false;
  bool clear_polarity = true;
  bool has_set = false;
  bool set_polarity = true;

  FeatureSet withoutClear() const
  {
    FeatureSet r = *this;
    r.has_clear = false;
    r.clear_polarity = true;
    return r;
  }

  FeatureSet withoutSet() const
  {
    FeatureSet r = *this;
    r.has_set = false;
    r.set_polarity = true;
    return r;
  }

  FeatureSet complementClear() const
  {
    FeatureSet r = *this;
    r.clear_polarity = !r.clear_polarity;
    return r;
  }

  FeatureSet complementSet() const
  {
    FeatureSet r = *this;
    r.set_polarity = !r.set_polarity;
    return r;
  }

  bool operator==(const FeatureSet& o) const
  {
    return clock_polarity == o.clock_polarity && has_clear == o.has_clear
           && clear_polarity == o.clear_polarity && has_set == o.has_set
           && set_polarity == o.set_polarity;
  }
};

struct FeatureSetHash
{
  size_t operator()(const FeatureSet& f) const
  {
    size_t h = 0;
    h = (h << 1) | f.clock_polarity;
    h = (h << 1) | f.has_clear;
    h = (h << 1) | f.clear_polarity;
    h = (h << 1) | f.has_set;
    h = (h << 1) | f.set_polarity;
    return h;
  }
};

struct PinPositions
{
  int clock_in = -1;
  int data_in = -1;
  int clear_in = -1;
  int set_in = -1;
  int data_out = -1;
  int data_negated_out = -1;
};

struct MapTarget
{
  sta::LibertyCell* cell;
  FeatureSet features;
  PinPositions pins;
  bool data_negated = false;
  float area;
  int pin_count = 0;

  // Compare for "is this target better": lower area wins,
  // tie-break on fewer pins, then lexicographic cell name.
  bool betterThan(const MapTarget& other) const
  {
    if (area != other.area) {
      return area < other.area;
    }
    if (pin_count != other.pin_count) {
      return pin_count < other.pin_count;
    }
    return cell->name() < other.cell->name();
  }
};

// Parse a FuncExpr that is either a port reference or NOT(port).
// Returns (polarity, port) where polarity=true means non-inverted.
static bool parsePin(sta::FuncExpr* expr,
                     bool& polarity,
                     sta::LibertyPort*& port)
{
  if (!expr) {
    return false;
  }
  if (expr->op() == sta::FuncExpr::Op::port) {
    polarity = true;
    port = expr->port();
    return true;
  }
  if (expr->op() == sta::FuncExpr::Op::not_ && expr->left()
      && expr->left()->op() == sta::FuncExpr::Op::port) {
    polarity = false;
    port = expr->left()->port();
    return true;
  }
  return false;
}

// Compute the position of a port in the input bundle (skipping
// output/power/ground ports, in iteration order).
static int inputPortPosition(sta::LibertyCell* cell, sta::LibertyPort* target)
{
  int pos = 0;
  sta::LibertyCellPortIterator it(cell);
  while (it.hasNext()) {
    sta::LibertyPort* p = it.next();
    if (p->direction()->isInput()) {
      if (p == target) {
        return pos;
      }
      int w = 1;
      if (p->isBus()) {
        w = std::abs(p->toIndex() - p->fromIndex()) + 1;
      }
      pos += w;
    }
  }
  return -1;
}

static bool detectCell(sta::LibertyCell* cell, MapTarget& result)
{
  if (!cell->hasSequentials()) {
    return false;
  }
  auto& seqs = cell->sequentials();
  if (seqs.size() != 1) {
    return false;
  }
  const sta::Sequential& seq = seqs[0];
  if (!seq.isRegister()) {
    return false;
  }

  FeatureSet feats;
  PinPositions pins;

  // Clock (required).
  bool clk_pol;
  sta::LibertyPort* clk_port;
  if (!parsePin(seq.clock(), clk_pol, clk_port)) {
    return false;
  }
  feats.clock_polarity = clk_pol;
  pins.clock_in = inputPortPosition(cell, clk_port);
  if (pins.clock_in < 0) {
    return false;
  }

  // Data (required).
  bool data_pol;
  sta::LibertyPort* data_port;
  if (!parsePin(seq.data(), data_pol, data_port)) {
    return false;
  }
  pins.data_in = inputPortPosition(cell, data_port);
  if (pins.data_in < 0) {
    return false;
  }

  // Clear (optional).
  if (seq.clear()) {
    bool pol;
    sta::LibertyPort* port;
    if (!parsePin(seq.clear(), pol, port)) {
      return false;
    }
    feats.has_clear = true;
    feats.clear_polarity = pol;
    pins.clear_in = inputPortPosition(cell, port);
    if (pins.clear_in < 0) {
      return false;
    }
  }

  // Preset/set (optional).
  if (seq.preset()) {
    bool pol;
    sta::LibertyPort* port;
    if (!parsePin(seq.preset(), pol, port)) {
      return false;
    }
    feats.has_set = true;
    feats.set_polarity = pol;
    pins.set_in = inputPortPosition(cell, port);
    if (pins.set_in < 0) {
      return false;
    }
  }

  // Output: find non-negated (Q) and negated (QN) output pins.
  {
    sta::LibertyPort* seq_out = seq.output();
    int pos = 0;
    sta::LibertyCellPortIterator pit(cell);
    while (pit.hasNext()) {
      sta::LibertyPort* p = pit.next();
      if (p->isPwrGnd()) {
        continue;
      }
      if (!p->direction()->isOutput()) {
        continue;
      }
      sta::FuncExpr* func = p->function();
      if (func && func->op() == sta::FuncExpr::Op::port
          && func->port() == seq_out) {
        pins.data_out = pos;
      } else if (func && func->op() == sta::FuncExpr::Op::not_ && func->left()
                 && func->left()->op() == sta::FuncExpr::Op::port
                 && func->left()->port() == seq_out) {
        pins.data_negated_out = pos;
      }
      int w = 1;
      if (p->isBus()) {
        w = std::abs(p->toIndex() - p->fromIndex()) + 1;
      }
      pos += w;
    }
  }

  // data_negated tracks whether the data input is inverted.
  bool data_negated = !data_pol;

  // Prefer the non-negated output.  If only the negated output exists,
  // use it and flip the effective negation and swap clear/set.
  if (pins.data_out < 0) {
    if (pins.data_negated_out < 0) {
      return false;
    }
    pins.data_out = pins.data_negated_out;
    pins.data_negated_out = -1;
    data_negated = !data_negated;
    std::swap(feats.has_clear, feats.has_set);
    std::swap(feats.clear_polarity, feats.set_polarity);
    std::swap(pins.clear_in, pins.set_in);
  }

  result.data_negated = data_negated;

  // Count described input pins vs total input pins.
  int described = (pins.clock_in >= 0) + (pins.data_in >= 0)
                  + (pins.clear_in >= 0) + (pins.set_in >= 0);
  int total_inputs = 0;
  sta::LibertyCellPortIterator it(cell);
  while (it.hasNext()) {
    sta::LibertyPort* p = it.next();
    if (p->isPwrGnd()) {
      continue;
    }
    if (p->direction()->isInput()) {
      total_inputs++;
    }
  }
  if (described != total_inputs) {
    return false;
  }

  result.cell = cell;
  result.features = feats;
  result.pins = pins;
  result.area = cell->area();
  result.pin_count = total_inputs;
  return true;
}

using FeatureMap = std::unordered_map<FeatureSet, MapTarget, FeatureSetHash>;

static void buildIndex(sta::Network* network,
                       FeatureMap& classes,
                       utl::Logger* logger,
                       const Synthesis& syn)
{
  sta::LibertyLibraryIterator* lib_iter = network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (syn.dontUse(cell)) {
        debugPrint(logger,
                   utl::SYN,
                   "sm",
                   1,
                   "ignoring {} because of dont use",
                   cell->name());
        continue;
      }
      MapTarget mt;
      if (!detectCell(cell, mt)) {
        continue;
      }
      debugPrint(
          logger,
          utl::SYN,
          "sm",
          1,
          "detected {} (clk={} clear={} set={} neg={})",
          cell->name(),
          mt.features.clock_polarity ? "pos" : "neg",
          mt.features.has_clear ? (mt.features.clear_polarity ? "pos" : "neg")
                                : "none",
          mt.features.has_set ? (mt.features.set_polarity ? "pos" : "neg")
                              : "none",
          mt.data_negated ? "yes" : "no");
      auto it = classes.find(mt.features);
      if (it != classes.end() && !mt.betterThan(it->second)) {
        continue;
      }
      classes[mt.features] = mt;
    }
  }
  delete lib_iter;

  // Feature demotion: propagate cells to simpler feature sets.
  bool changed = true;
  while (changed) {
    changed = false;
    FeatureMap snapshot = classes;
    for (auto& [feats, target] : snapshot) {
      FeatureSet demoted[] = {feats.withoutClear(), feats.withoutSet()};
      for (auto& df : demoted) {
        auto it = classes.find(df);
        if (it == classes.end() || target.betterThan(it->second)) {
          classes[df] = target;
          changed = true;
        }
      }
    }
  }
}

// Build a Target input bundle with all pins set to inactive defaults.
static Bundle buildDefaultInputs(const MapTarget& mt)
{
  // Count total input width.
  uint32_t input_width = 0;
  sta::LibertyCellPortIterator it(mt.cell);
  while (it.hasNext()) {
    sta::LibertyPort* p = it.next();
    if (p->direction()->isInput()) {
      if (p->isBus()) {
        input_width += std::abs(p->toIndex() - p->fromIndex()) + 1;
      } else {
        input_width += 1;
      }
    }
  }

  Bundle inputs = Bundle::undef(input_width);
  // Set control pins to their inactive values.
  if (mt.features.has_clear) {
    inputs.mutableNet(mt.pins.clear_in)
        = mt.features.clear_polarity ? Net::zero() : Net::one();
  }
  if (mt.features.has_set) {
    inputs.mutableNet(mt.pins.set_in)
        = mt.features.set_polarity ? Net::zero() : Net::one();
  }
  return inputs;
}

void mapSequentials(Graph& g,
                    sta::Network* network,
                    utl::Logger* logger,
                    const Synthesis& syn)
{
  g.normalize();

  FeatureMap classes;
  buildIndex(network, classes, logger, syn);

  std::vector<Instance*> to_remove;

  g.forEachInstance([&](const Instance* inst) {
    if (!inst->is<Dff>()) {
      return;
    }
    auto* dff = inst->as<Dff>();

    for (uint32_t i = 0; i < dff->data().width(); i++) {
      FeatureSet feats;
      feats.clock_polarity = dff->clock().isPositive();

      bool clear_is_set = false;
      bool complement_clear = false;
      if (!dff->clear().isAlways(false)) {
        Net cv = dff->clearValue()[i];
        if (cv == Net::zero()) {
          feats.has_clear = true;
          feats.clear_polarity = dff->clear().isPositive();
        } else if (cv == Net::one()) {
          clear_is_set = true;
          feats.has_set = true;
          feats.set_polarity = dff->clear().isPositive();
        }
      }

      auto it = classes.find(feats);
      if (it == classes.end()) {
        it = classes.find(clear_is_set ? feats.complementSet()
                                       : feats.complementClear());
        complement_clear = true;
      }

      if (it == classes.end()) {
        auto polString = [](bool has, bool polarity) {
          if (!has) {
            return "none";
          }
          return polarity ? "pos" : "neg";
        };
        logger->error(utl::SYN,
                      13,
                      "mapSequentials: no matching flip-flop cell for"
                      " Dff bit {} (clk={} clear={} set={})",
                      i,
                      feats.clock_polarity ? "pos" : "neg",
                      polString(feats.has_clear, feats.clear_polarity),
                      polString(feats.has_set, feats.set_polarity));
      }
      const MapTarget& mt = it->second;

      Bundle inputs = buildDefaultInputs(mt);
      inputs.mutableNet(mt.pins.clock_in) = dff->clock().net();
      inputs.mutableNet(mt.pins.data_in)
          = mt.data_negated ? g.add<Not>(BundleView(dff->data()[i]))[0]
                            : dff->data()[i];

      if (!dff->clear().isAlways(false)) {
        if (!clear_is_set) {
          inputs.mutableNet(mt.pins.clear_in)
              = complement_clear ? g.add<Not>(dff->clear().net()).asNet()
                                 : dff->clear().net();
        } else {
          inputs.mutableNet(mt.pins.set_in)
              = complement_clear ? g.add<Not>(dff->clear().net()).asNet()
                                 : dff->clear().net();
        }
      }

      Bundle target_out = g.add<Target>(mt.cell, std::move(inputs));
      Net old_out = g.output(dff)[i];
      Net new_out = target_out[mt.pins.data_out];
      g.forceReplace(BundleView(old_out), BundleView(new_out));
    }

    to_remove.push_back(const_cast<Instance*>(inst));
  });

  for (auto* inst : to_remove) {
    g.removeInstance(inst);
  }
}

}  // namespace syn
