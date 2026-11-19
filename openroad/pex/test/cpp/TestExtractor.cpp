// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "pex/Evaluator.h"
#include "pex/Extractor.h"
#include "pex/Model.h"
#include "pex/Network.h"
#include "tst/db_fixture.h"

namespace pex {
namespace {

constexpr std::array<std::string_view, 6> kLayers
    = {"li1", "met1", "met2", "met3", "met4", "met5"};

// Tables for every context of the routing layers, with values that differ
// by layer and cover so that each lookup is distinguishable; extraction is
// checked against itself and the brute force here, not against a solver.
Model everyContext()
{
  Model model;
  const int n = static_cast<int>(kLayers.size());
  auto addLateral = [&](const int l, const int below, const int above) {
    const LateralKey key{
        .layer = std::string(kLayers[l]),
        .below = below < 0 ? "" : std::string(kLayers[below]),
        .above = above < 0 ? "" : std::string(kLayers[above]),
    };
    const double base
        = 0.01 * (l + 1) + 0.001 * (below + 2) + 0.0001 * (above + 2);
    LateralTable& t
        = model.addLateral(key, LateralTable({0.14, 1.6}, {0.14, 1.6}));
    for (int w = 0; w < 2; w++) {
      for (int sl = 0; sl < 3; sl++) {
        for (int sr = 0; sr < 3; sr++) {
          t.set(w,
                sl,
                sr,
                {
                    .ground = base * (w + 1),
                    .left = sl == 2 ? 0.0 : 0.02 / (sl + 1),
                    .right = sr == 2 ? 0.0 : 0.02 / (sr + 1),
                    .below = below < 0 ? 0.0 : 0.005,
                    .above = above < 0 ? 0.0 : 0.004,
                });
        }
      }
    }
    EndTable& e = model.addEnd(key, EndTable({0.14, 1.6}, {0.3}, {0.3}));
    for (int w = 0; w < 2; w++) {
      for (int s = 0; s < 2; s++) {
        for (int g = 0; g < 2; g++) {
          const double lateral = s == 0 ? 0.002 : 0.0;
          const double facing = g == 0 ? 0.003 : 0.0;
          e.set(w,
                s,
                g,
                {
                    .total = base + lateral + facing,
                    .below = below < 0 ? 0.0 : base / 50,
                    .above = above < 0 ? 0.0 : base / 40,
                    .lateral = lateral,
                    .facing = facing,
                });
        }
      }
    }
  };
  for (int l = 0; l < n; l++) {
    for (int below = -1; below < l; below++) {
      for (int above = l + 1; above <= n; above++) {
        addLateral(l, below, above == n ? -1 : above);
      }
    }
    for (int c = 0; c < n; c++) {
      if (c == l) {
        continue;
      }
      // With no plane on the other side, and with each layer there.
      std::vector<std::string> opposite{""};
      for (int o = 0; o < n; o++) {
        if ((c < l && o > l) || (c > l && o < l)) {
          opposite.emplace_back(kLayers[o]);
        }
      }
      for (const std::string& o : opposite) {
        model.addCrossing(
            {
                .layer = std::string(kLayers[l]),
                .cover = std::string(kLayers[c]),
                .opposite = o,
            },
            CrossingTable({0.5, 4},
                          {
                              {.total = 0.7, .cross = 0.6},
                              {.total = 0.3, .cross = 0.2},
                          }));
      }
      if (std::abs(c - l) <= 2) {
        DiagonalTable& d = model.addDiagonal(
            {
                .layer = std::string(kLayers[l]),
                .other = std::string(kLayers[c]),
                .opposite = "",
            },
            DiagonalTable({0.14, 1.6}, {0, 1}, {0.14, 1.6}, {0.14, 1.6}));
        for (int w = 0; w < 2; w++) {
          for (int g = 0; g < 2; g++) {
            for (int s = 0; s < 3; s++) {
              for (int f = 0; f < 3; f++) {
                d.set(w,
                      g,
                      s,
                      f,
                      {
                          .coupling = 0.003 / (g + 1),
                          .total_delta = 0.004 / (g + 1),
                          .lateral_delta = -0.001,
                      });
              }
            }
          }
        }
      }
    }
  }
  return model;
}

class ExtractorTest : public tst::DbFixture
{
 protected:
  void SetUp() override
  {
    odb::dbTech* tech
        = loadTechLef("sky130", "_main/openroad/pex/test/sky130hd/sky130hd.tlef");
    loadLibLef(tech,
               "cells",
               "_main/openroad/pex/test/sky130hd/sky130_fd_sc_hd_merged.lef");
    block_ = readDef("_main/openroad/fsv/test/field_solver_gcd.def");
    stack_ = fsv::readItfFile(getFilePath("_main/openroad/fsv/test/sky130.itf"));
    model_ = everyContext();
  }

  // The block's parasitics as text, objects in creation order.
  std::string dump() const
  {
    std::ostringstream out;
    out.precision(17);
    for (odb::dbNet* net : block_->getNets()) {
      if (!net->isRCgraph()) {
        continue;
      }
      out << net->getName() << '\n';
      for (odb::dbCapNode* node : net->getCapNodes()) {
        out << " n " << node->getId() << ' ' << node->getNode() << ' '
            << node->isITerm() << node->isBTerm() << node->isInternal() << ' '
            << node->getCapacitance(0) << '\n';
        for (odb::dbCCSeg* cc : node->getCCSegs()) {
          out << "  c " << cc->getSourceCapNode()->getId() << ' '
              << cc->getTargetCapNode()->getId() << ' ' << cc->getCapacitance(0)
              << '\n';
        }
      }
      for (odb::dbRSeg* r : net->getRSegs()) {
        out << " r " << r->getSourceNode() << ' ' << r->getTargetNode() << ' '
            << r->getResistance(0) << '\n';
      }
    }
    return out.str();
  }

  odb::dbBlock* block_ = nullptr;
  fsv::Stack stack_;
  Model model_;
};

// The index gives each box every box that can change its lookups: a net's
// contributions equal those with every box of the block as candidates.
TEST_F(ExtractorTest, IndexEqualsEveryBox)
{
  const Evaluator evaluator(stack_, model_, block_->getDbUnitsPerMicron());
  const BlockBoxes boxes = readBlockBoxes(block_, stack_, evaluator);
  const BlockIndex index(boxes, evaluator, 2);
  const int count = static_cast<int>(boxes.boxes.size());
  std::vector<odb::Rect> rects;
  std::vector<int> ids;
  int compared = 0;
  for (size_t n = 0; n < boxes.nets.size(); n++) {
    if (boxes.nets[n]->getSigType().isSupply()
        || boxes.net_first[n] == boxes.net_first[n + 1]) {
      continue;
    }
    std::vector<Contribution> indexed;
    std::vector<Contribution> brute;
    evaluator.evaluate(
        boxes.boxes,
        boxes.netBoxes(n),
        [&](const int box, std::vector<int>& out) {
          index.candidates(box, rects, ids, out);
        },
        indexed);
    evaluator.evaluate(
        boxes.boxes,
        boxes.netBoxes(n),
        [&](int, std::vector<int>& out) {
          out.resize(count);
          std::iota(out.begin(), out.end(), 0);
        },
        brute);
    ASSERT_EQ(indexed.size(), brute.size()) << boxes.nets[n]->getName();
    for (size_t i = 0; i < brute.size(); i++) {
      EXPECT_EQ(indexed[i].box, brute[i].box);
      EXPECT_EQ(indexed[i].other, brute[i].other);
      EXPECT_EQ(indexed[i].value, brute[i].value);
      EXPECT_EQ(indexed[i].at, brute[i].at);
    }
    compared++;
  }
  EXPECT_GT(compared, 100);
}

// The parasitics are the same for any thread count, and a second
// extraction replaces the first. (Some pins of this excerpt of gcd are not
// on their net's wire; their capacitance goes to the net's first node.)
TEST_F(ExtractorTest, Threads)
{
  std::string first;
  int unplaced = -1;
  for (const int threads : {0, 1, 4}) {
    ExtractOptions options;
    options.threads = threads;
    Extractor extractor(block_, stack_, model_, getLogger(), options);
    const ExtractStats stats = extractor.extract();
    EXPECT_GT(stats.nets, 100);
    EXPECT_EQ(stats.loops, 0);
    if (unplaced < 0) {
      unplaced = stats.unplaced;
    }
    EXPECT_EQ(stats.unplaced, unplaced);
    EXPECT_TRUE(stats.unknown_layers.empty());
    const std::string text = dump();
    if (first.empty()) {
      first = text;
    } else {
      EXPECT_EQ(text, first) << threads << " threads";
    }
  }
}

// What the evaluator gives every extracted net is in ODB: ground on the
// nodes, and each coupling once as the average of what its two nets saw.
TEST_F(ExtractorTest, Commit)
{
  // Ground plus both sides of every coupling, as stored (in float).
  auto stored = [&] {
    double total = 0;
    for (odb::dbNet* net : block_->getNets()) {
      for (odb::dbCapNode* node : net->getCapNodes()) {
        total += node->getCapacitance(0);
        for (odb::dbCCSeg* cc : node->getCCSegs()) {
          total += cc->getCapacitance(0);
        }
      }
    }
    return total;
  };
  // A net flagged before, as by another extraction, is not flagged after.
  odb::dbNet* power = block_->findNet("VDD");
  ASSERT_NE(power, nullptr);
  power->setRCgraph(true);
  // With no thresholds, no pair is grounded.
  ExtractOptions none;
  none.coupling_threshold = 0;
  none.coupling_ratio = 0;
  Extractor all(block_, stack_, model_, getLogger(), none);
  EXPECT_EQ(all.extract().grounded_pairs, 0);
  EXPECT_FALSE(power->isRCgraph());
  const double total = stored();
  double seen = 0;
  int extracted = 0;
  const std::vector<NetParasitics>& parasitics = all.parasitics();
  for (size_t n = 0; n < parasitics.size(); n++) {
    const NetParasitics& p = parasitics[n];
    if (p.network.nodes.empty()) {
      continue;
    }
    extracted++;
    odb::dbNet* net = all.boxes().nets[n];
    EXPECT_TRUE(net->isRCgraph());
    // ODB's own totals read the ground capacitance where it is stored.
    double ground = 0;
    for (odb::dbCapNode* node : net->getCapNodes()) {
      ground += node->getCapacitance(0);
    }
    EXPECT_GT(ground, 0) << net->getName();
    EXPECT_DOUBLE_EQ(net->getTotalCapacitance(0, false), ground);
    // The resistors iterate in path order, the head placeholder skipped.
    size_t r = 0;
    for (odb::dbRSeg* seg : net->getRSegs()) {
      ASSERT_LT(r, p.network.resistors.size());
      const NetResistor& expected = p.network.resistors[r];
      EXPECT_EQ(seg->getSourceCapNode()->getNode(),
                p.network.nodes[expected.source].id);
      EXPECT_EQ(seg->getTargetCapNode()->getNode(),
                p.network.nodes[expected.target].id);
      EXPECT_NEAR(seg->getResistance(0),
                  expected.resistance,
                  1e-6 * expected.resistance);
      r++;
    }
    EXPECT_EQ(r, p.network.resistors.size()) << net->getName();
    for (const double g : p.ground) {
      seen += g;
    }
    for (const auto& [key, value] : p.couplings) {
      seen += value;
    }
  }
  EXPECT_GT(extracted, 100);
  EXPECT_NEAR(total, seen, 1e-6 * seen);

  // The default thresholds ground some pairs, which keeps every total.
  Extractor some(block_, stack_, model_, getLogger());
  const ExtractStats stats = some.extract();
  EXPECT_GT(stats.grounded_pairs, 0);
  EXPECT_GT(stats.coupled_pairs, 0);
  EXPECT_NEAR(stored(), total, 1e-6 * total);

  // Coupling to ground grounds every pair whatever the thresholds, with
  // the same totals and no coupling capacitors.
  ExtractOptions grounded = none;
  grounded.coupling_to_ground = true;
  Extractor decoupled(block_, stack_, model_, getLogger(), grounded);
  const ExtractStats ground_stats = decoupled.extract();
  EXPECT_EQ(ground_stats.coupled_pairs, 0);
  EXPECT_EQ(ground_stats.grounded_pairs,
            stats.coupled_pairs + stats.grounded_pairs);
  EXPECT_NEAR(stored(), total, 1e-6 * total);
  for (odb::dbNet* net : block_->getNets()) {
    for (odb::dbCapNode* node : net->getCapNodes()) {
      EXPECT_EQ(node->getCCSegs().size(), 0) << net->getName();
    }
  }
}

// A model without tables for a layer that signal nets are routed on is an
// error before the block's parasitics are touched.
TEST_F(ExtractorTest, ModelLacksLayer)
{
  const Model empty;
  Extractor extractor(block_, stack_, empty, getLogger());
  try {
    extractor.extract();
    FAIL() << "no error";
  } catch (const std::invalid_argument& e) {
    EXPECT_NE(std::string(e.what()).find("met1"), std::string::npos)
        << e.what();
  }
}

// A box at the coordinate limits reaches up to them, not past them.
TEST_F(ExtractorTest, ReachAtLimits)
{
  const Evaluator evaluator(stack_, model_, 1000);
  constexpr int kMax = std::numeric_limits<int>::max();
  constexpr int kMin = std::numeric_limits<int>::min();
  int met1 = 0;
  while (evaluator.layerName(met1) != "met1") {
    met1++;
  }
  std::vector<odb::Rect> rects;
  evaluator.reach(
      {.net = 0, .layer = met1, .rect = {kMax - 1000, 0, kMax, 140}}, rects);
  ASSERT_EQ(rects.size(), evaluator.layerCount());
  for (const odb::Rect& r : rects) {
    EXPECT_EQ(r.xMax(), kMax);
    EXPECT_LT(r.xMin(), kMax - 1000);
  }
  evaluator.reach(
      {.net = 0, .layer = met1, .rect = {kMin, 0, kMin + 1000, 140}}, rects);
  for (const odb::Rect& r : rects) {
    EXPECT_EQ(r.xMin(), kMin);
    EXPECT_GT(r.xMax(), kMin + 1000);
  }
}

}  // namespace
}  // namespace pex
