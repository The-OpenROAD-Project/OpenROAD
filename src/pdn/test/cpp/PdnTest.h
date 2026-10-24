// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "domain.h"
#include "grid.h"
#include "grid_component.h"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "shape.h"
#include "tst/fixture.h"
#include "tst/physical_builder.h"

namespace pdn {

// Minimal concrete Grid subclass used by Shape/GridComponent tests; we
// only need an instantiable `Grid` so a `GridComponent` can be built.
class TestGrid : public Grid
{
 public:
  using Grid::Grid;
  Type type() const override { return kCore; }
};

// Minimal concrete GridComponent subclass with no-op virtual methods so
// it can be used as a Shape's `grid_component_`.
class TestGridComponent : public GridComponent
{
 public:
  using GridComponent::GridComponent;
  void makeShapes(const Shape::ShapeTreeMap& /*other_shapes*/) override {}
  void report() const override {}
  Type type() const override { return kStrap; }
  void checkLayerSpecifications() const override {}
};

// Fixture for PDN unit tests. Provides a minimal in-memory tech (no LEF
// needed) plus helpers that build synthetic via stacks for the
// classes under test.
//
// Why no LEF? Building exactly the bits we need keeps tests fast, makes
// failures easier to debug (every dimension is explicit), and lets us
// generate problematic rule combinations that no real tech ships.
class PdnTest : public tst::Fixture
{
 protected:
  void SetUp() override
  {
    tst::makeTech(db_.get());
    tech()->setManufacturingGrid(kManufacturingGrid);
    tst::makeLib(db_.get(), tech());
    tst::makeBlock(db_.get(), tech());
  }

  odb::dbTech* tech() { return db_->getTech(); }
  odb::dbBlock* block() { return db_->getChip()->getBlock(); }

  // A minimal via stack: bottom metal -> cut -> top metal, with a
  // single-cut dbTechVia and a matching VIARULE GENERATE rule (with
  // enclosures on both ends).
  struct ViaStack
  {
    odb::dbTechLayer* bottom = nullptr;          // routing, HORIZONTAL
    odb::dbTechLayer* cut = nullptr;             // cut layer
    odb::dbTechLayer* top = nullptr;             // routing, VERTICAL
    odb::dbTechVia* via = nullptr;               // single-cut tech via
    odb::dbTechViaGenerateRule* rule = nullptr;  // VIARULE GENERATE
  };

  // Build a metal-cut-metal stack named "{prefix}_m1 / {prefix}_v1 /
  // {prefix}_m2". Geometry is fully controlled by the parameters; defaults
  // produce a single-cut via with a 50-unit-wide cut and 10-unit enclosure
  // on each side. `bottom_dir` lets tests pick whether the lower metal is
  // horizontal or vertical (the top metal is set to the opposite).
  ViaStack makeBasicViaStack(const std::string& prefix = "",
                             int cut_size = 50,
                             int enclosure = 10,
                             odb::dbTechLayerDir bottom_dir
                             = odb::dbTechLayerDir::HORIZONTAL)
  {
    ViaStack s;
    s.bottom = tst::addRoutingLayer(
        tech(),
        (prefix + "m1").c_str(),
        {.direction = bottom_dir, .width = cut_size + 2 * enclosure});

    s.cut = tst::addCutLayer(tech(), (prefix + "v1").c_str());
    s.cut->setWidth(cut_size);

    s.top = tst::addRoutingLayer(
        tech(),
        (prefix + "m2").c_str(),
        {.direction = bottom_dir == odb::dbTechLayerDir::HORIZONTAL
                          ? odb::dbTechLayerDir::VERTICAL
                          : odb::dbTechLayerDir::HORIZONTAL,
         .width = cut_size + 2 * enclosure});

    s.via = tst::addVia(tech(),
                        (prefix + "TV1").c_str(),
                        s.bottom,
                        s.cut,
                        s.top,
                        cut_size,
                        enclosure);

    // Matching VIARULE GENERATE rule with the same enclosures, so that
    // a GenerateViaGenerator built from this rule will succeed.
    s.rule = odb::dbTechViaGenerateRule::create(tech(),
                                                (prefix + "Rule1").c_str(),
                                                /*is_default=*/false);
    odb::dbTechViaLayerRule* bot_rule
        = odb::dbTechViaLayerRule::create(tech(), s.rule, s.bottom);
    bot_rule->setEnclosure(enclosure, enclosure);
    odb::dbTechViaLayerRule* cut_rule
        = odb::dbTechViaLayerRule::create(tech(), s.rule, s.cut);
    cut_rule->setRect(
        odb::Rect(-cut_size / 2, -cut_size / 2, cut_size / 2, cut_size / 2));
    // a centre-to-centre pitch of one cut width, so the cuts abut unless the
    // cut layer sets a spacing
    cut_rule->setSpacing(cut_size, cut_size);
    odb::dbTechViaLayerRule* top_rule
        = odb::dbTechViaLayerRule::create(tech(), s.rule, s.top);
    top_rule->setEnclosure(enclosure, enclosure);

    return s;
  }

  // Build a VIARULE GENERATE stack with independent, per-axis enclosures on
  // each metal layer (unlike makeBasicViaStack, which uses one symmetric
  // overhang). This lets a test reproduce real techs where the bottom and top
  // enclosures are asymmetric and oriented differently -- e.g. a vertical
  // bottom strap enclosing only along its length while a horizontal top strap
  // encloses only across its width. `bottom_dir` sets the lower metal's
  // routing direction; the upper metal is set to the opposite.
  ViaStack makeGenerateViaStack(int cut_size,
                                int bottom_enc_x,
                                int bottom_enc_y,
                                int top_enc_x,
                                int top_enc_y,
                                int cut_spacing,
                                odb::dbTechLayerDir bottom_dir
                                = odb::dbTechLayerDir::VERTICAL)
  {
    ViaStack s;
    s.bottom = tst::addRoutingLayer(
        tech(), "m1", {.direction = bottom_dir, .width = cut_size});

    s.cut = tst::addCutLayer(tech(), "v1");
    s.cut->setWidth(cut_size);

    s.top = tst::addRoutingLayer(
        tech(),
        "m2",
        {.direction = bottom_dir == odb::dbTechLayerDir::VERTICAL
                          ? odb::dbTechLayerDir::HORIZONTAL
                          : odb::dbTechLayerDir::VERTICAL,
         .width = cut_size});

    s.rule = odb::dbTechViaGenerateRule::create(tech(),
                                                "GenRule",
                                                /*is_default=*/false);
    auto* br = odb::dbTechViaLayerRule::create(tech(), s.rule, s.bottom);
    br->setEnclosure(bottom_enc_x, bottom_enc_y);
    auto* cr = odb::dbTechViaLayerRule::create(tech(), s.rule, s.cut);
    cr->setRect(
        odb::Rect(-cut_size / 2, -cut_size / 2, cut_size / 2, cut_size / 2));
    cr->setSpacing(cut_spacing, cut_spacing);
    auto* tr = odb::dbTechViaLayerRule::create(tech(), s.rule, s.top);
    tr->setEnclosure(top_enc_x, top_enc_y);

    return s;
  }

  // Build a multi-cut tech via on an existing stack: a 2x2 grid of cuts at
  // (+/-cut_pitch/2, +/-cut_pitch/2) plus a single merged metal box per metal
  // that encloses the outer cut edges by `enclosure`. Used to exercise
  // DbTechVia's multicut-simplification path (it requires the tech via itself
  // to contain >1 cut boxes).
  odb::dbTechVia* addMultiCutViaToStack(const ViaStack& s,
                                        const char* name,
                                        int cut_size,
                                        int cut_pitch,
                                        int enclosure)
  {
    odb::dbTechVia* mv = odb::dbTechVia::create(tech(), name);
    const int half = cut_pitch / 2;
    const int cut_half = cut_size / 2;
    const int metal_half = half + cut_half + enclosure;
    // 2x2 cuts laid out so adjacent centers are exactly cut_pitch apart.
    for (int dx : {-half, half}) {
      for (int dy : {-half, half}) {
        odb::dbBox::create(mv,
                           s.cut,
                           dx - cut_half,
                           dy - cut_half,
                           dx + cut_half,
                           dy + cut_half);
      }
    }
    // Single combined enclosure rect on each metal covering the cut grid
    // plus its enclosure.
    odb::dbBox::create(
        mv, s.bottom, -metal_half, -metal_half, metal_half, metal_half);
    odb::dbBox::create(
        mv, s.top, -metal_half, -metal_half, metal_half, metal_half);
    return mv;
  }

  // Build a second via stack stacked on top of `lower` (shares
  // lower.top as the new stack's bottom layer).
  ViaStack addStackedLevel(const ViaStack& lower,
                           int cut_size = 50,
                           int enclosure = 10)
  {
    ViaStack s;
    s.bottom = lower.top;  // share the middle metal

    s.cut = tst::addCutLayer(tech(), "v2");
    s.cut->setWidth(cut_size);

    s.top = tst::addRoutingLayer(tech(),
                                 "m3",
                                 {.direction = odb::dbTechLayerDir::HORIZONTAL,
                                  .width = cut_size + 2 * enclosure});

    s.via = tst::addVia(
        tech(), "TV2", s.bottom, s.cut, s.top, cut_size, enclosure);

    s.rule = odb::dbTechViaGenerateRule::create(tech(),
                                                "Rule2",
                                                /*is_default=*/false);
    auto* br = odb::dbTechViaLayerRule::create(tech(), s.rule, s.bottom);
    br->setEnclosure(enclosure, enclosure);
    auto* cr = odb::dbTechViaLayerRule::create(tech(), s.rule, s.cut);
    cr->setRect(
        odb::Rect(-cut_size / 2, -cut_size / 2, cut_size / 2, cut_size / 2));
    cr->setSpacing(cut_size, cut_size);
    auto* tr = odb::dbTechViaLayerRule::create(tech(), s.rule, s.top);
    tr->setEnclosure(enclosure, enclosure);

    return s;
  }

  // A metal/cut/metal/.../metal stack of alternating routing directions,
  // as `Connect` sees a real tech: routing layers named "M1".."M<levels>"
  // with cut layers "V1".."V<levels-1>" interleaved. `first_dir` sets M1's
  // routing direction; every layer above alternates.
  //
  // `Connect` walks `dbTech::findLayer(number)` between its two endpoints, so
  // the interleaved creation order (and therefore layer numbering) is what
  // makes the intermediate-layer discovery work. Not tst::makeLayerStack:
  // that also creates a tech via per level, which `Connect` would pick up.
  struct RoutingStack
  {
    std::vector<odb::dbTechLayer*> routing;  // M1, M2, ...
    std::vector<odb::dbTechLayer*> cuts;     // V1, V2, ...
  };

  RoutingStack makeRoutingStack(int levels,
                                int min_width = 100,
                                odb::dbTechLayerDir first_dir
                                = odb::dbTechLayerDir::HORIZONTAL)
  {
    RoutingStack stack;
    odb::dbTechLayerDir dir = first_dir;
    for (int i = 1; i <= levels; i++) {
      if (i > 1) {
        odb::dbTechLayer* cut
            = tst::addCutLayer(tech(), ("V" + std::to_string(i - 1)).c_str());
        cut->setWidth(min_width / 2);
        stack.cuts.push_back(cut);
      }

      // getWidth() is the default routing width, which is what
      // Connect::generateMinEnclosureViaRects uses as the min-width target;
      // getMinWidth() feeds the complex-stack path.
      odb::dbTechLayer* metal
          = tst::addRoutingLayer(tech(),
                                 ("M" + std::to_string(i)).c_str(),
                                 {.direction = dir, .width = min_width});
      metal->setMinWidth(min_width);
      stack.routing.push_back(metal);

      dir = dir == odb::dbTechLayerDir::HORIZONTAL
                ? odb::dbTechLayerDir::VERTICAL
                : odb::dbTechLayerDir::HORIZONTAL;
    }
    return stack;
  }

  // ---------- Rule-augmentation helpers ----------
  // Each helper layers one optional tech feature on top of a basic stack so
  // tests can drive the ViaGenerator branches they care about.

  // Set the cut layer's default spacing. This drives the first branch of
  // ViaGenerator::determineCutSpacing.
  void setCutLayerSpacing(const ViaStack& s, int spacing)
  {
    s.cut->setSpacing(spacing);
  }

  // Attach a LEF58 cut-class rule on the cut layer whose (width, length)
  // match the stack's cut box. After this, ViaGenerator::determineCutClass
  // (called inside the generator's ctor) will pick it up.
  odb::dbTechLayerCutClassRule* addCutClassMatching(const ViaStack& s,
                                                    const char* name,
                                                    int cut_dim)
  {
    odb::dbTechLayerCutClassRule* rule
        = odb::dbTechLayerCutClassRule::create(s.cut, name);
    rule->setWidth(cut_dim);
    rule->setLength(cut_dim);
    rule->setLengthValid(true);
    return rule;
  }

  // LEF v5.4 MINIMUMCUT rule: require `num_cuts` cuts on layer connections
  // to wires at least `width` wide. `above_only`/`below_only` constrain it
  // to the via direction (matches LEF FROMABOVE/FROMBELOW).
  odb::dbTechMinCutRule* addMinCutRuleV54(odb::dbTechLayer* on_layer,
                                          uint32_t num_cuts,
                                          uint32_t width,
                                          bool above_only = false,
                                          bool below_only = false)
  {
    odb::dbTechMinCutRule* rule = odb::dbTechMinCutRule::create(on_layer);
    rule->setMinimumCuts(num_cuts, width, above_only, below_only);
    return rule;
  }

  // LEF58 cut enclosure rule attached to the cut layer; drives
  // ViaGenerator::getCutMinimumEnclosureRules and getMinimumEnclosures.
  odb::dbTechLayerCutEnclosureRule* addCutEnclosureRule(
      odb::dbTechLayer* cut_layer,
      int first_overhang,
      int second_overhang,
      odb::dbTechLayerCutEnclosureRule::ENC_TYPE type
      = odb::dbTechLayerCutEnclosureRule::DEFAULT)
  {
    odb::dbTechLayerCutEnclosureRule* rule
        = odb::dbTechLayerCutEnclosureRule::create(cut_layer);
    rule->setType(type);
    rule->setFirstOverhang(first_overhang);
    rule->setSecondOverhang(second_overhang);
    return rule;
  }

  // Constrain the width range on both the bottom and top via-layer-rule
  // entries (the LEF VIARULE WIDTH/MAXWIDTH constraint). Drives
  // GenerateViaGenerator::isLayerValidForWidth.
  void setRoutingWidthRange(const ViaStack& s, int min_w, int max_w)
  {
    for (uint32_t i = 0; i < s.rule->getViaLayerRuleCount(); ++i) {
      auto* lr = s.rule->getViaLayerRule(i);
      if (lr->getLayer()->getType() == odb::dbTechLayerType::ROUTING) {
        lr->setWidth(min_w, max_w);
      }
    }
  }

  // LEF58 cut-spacing rule with ADJACENTCUTS type; drives the
  // updateCutSpacing rule loop.
  odb::dbTechLayerCutSpacingRule* addAdjacentCutsSpacingRule(
      odb::dbTechLayer* cut_layer,
      int cut_spacing,
      uint32_t adjacent_cuts)
  {
    odb::dbTechLayerCutSpacingRule* rule
        = odb::dbTechLayerCutSpacingRule::create(cut_layer);
    rule->setType(odb::dbTechLayerCutSpacingRule::ADJACENTCUTS);
    rule->setCutSpacing(cut_spacing);
    rule->setAdjacentCuts(adjacent_cuts);
    rule->setCutClassToAll(true);
    return rule;
  }

  // LEF v5.4 SPACING ADJACENTCUTS rule: drives the V54SpacingRules fallback
  // in updateCutSpacing.
  odb::dbTechLayerSpacingRule* addV54AdjacentCutsRule(
      odb::dbTechLayer* cut_layer,
      uint32_t numcuts,
      uint32_t within,
      uint32_t spacing)
  {
    odb::dbTechLayerSpacingRule* rule
        = odb::dbTechLayerSpacingRule::create(cut_layer);
    rule->setAdjacentCuts(numcuts,
                          within,
                          spacing,
                          /*except_same_pgnet=*/false);
    return rule;
  }

  // LEF58 ARRAYSPACING rule on the cut layer. The cuts-array-spacing map
  // is keyed by required cut count -> spacing, mirroring LEF syntax
  // ("CUTS n SPACING s"). Drives the array-spacing branch inside
  // ViaGenerator::determineRowsAndColumns.
  odb::dbTechLayerArraySpacingRule* addArraySpacingRule(
      odb::dbTechLayer* cut_layer,
      int num_cuts,
      int array_spacing,
      int cut_spacing = 0,
      bool long_array = false)
  {
    odb::dbTechLayerArraySpacingRule* rule
        = odb::dbTechLayerArraySpacingRule::create(cut_layer);
    rule->setCutsArraySpacing(num_cuts, array_spacing);
    if (cut_spacing > 0) {
      rule->setCutSpacing(cut_spacing);
    }
    if (long_array) {
      rule->setLongArray(true);
    }
    return rule;
  }

  // LEF58 CUTSPACING table rule keyed by cut-class. Fills a 2x2 spacing
  // table indexed by "{class}/SIDE" and "{class}/END" so
  // ViaGenerator::determineCutSpacing can resolve a non-zero spacing via
  // the rule's class-pair lookup.
  odb::dbTechLayerCutSpacingTableDefRule* addCutSpacingTableRule(
      odb::dbTechLayer* cut_layer,
      const std::string& cut_class_name,
      int spacing,
      bool same_net = false)
  {
    odb::dbTechLayerCutSpacingTableDefRule* rule
        = odb::dbTechLayerCutSpacingTableDefRule::create(cut_layer);
    rule->setDefault(0);
    if (same_net) {
      rule->setSameNet(true);
    }
    // 2x2 table indexed by SIDE / END for the same class on rows and cols.
    std::map<std::string, uint32_t> row_map;
    std::map<std::string, uint32_t> col_map;
    row_map[cut_class_name + "/SIDE"] = 0;
    row_map[cut_class_name + "/END"] = 1;
    col_map[cut_class_name + "/SIDE"] = 0;
    col_map[cut_class_name + "/END"] = 1;
    std::vector<std::vector<std::pair<int, int>>> table = {
        {{spacing, spacing}, {spacing, spacing}},
        {{spacing, spacing}, {spacing, spacing}},
    };
    rule->setSpacingTable(table, row_map, col_map);
    return rule;
  }

  // LEF58 MINIMUMCUT rule attached to a routing layer, keyed by cut-class
  // name. Drives the LEF58 path in TechLayer::getMinCutRules and the
  // cut-class-matched path in ViaGenerator::checkMinCuts.
  odb::dbTechLayerMinCutRule* addV58MinCutPerCutClass(
      odb::dbTechLayer* routing_layer,
      const char* cut_class_name,
      int num_cuts,
      int width)
  {
    odb::dbTechLayerMinCutRule* rule
        = odb::dbTechLayerMinCutRule::create(routing_layer);
    rule->setWidth(width);
    rule->setCutsPerCutClass(cut_class_name, num_cuts);
    return rule;
  }

  // Attach an ABOVE-only or BELOW-only cut enclosure rule on the cut layer.
  // Exercises the getCutMinimumEnclosureRules above/below branches.
  odb::dbTechLayerCutEnclosureRule* addAboveBelowCutEnclosureRule(
      odb::dbTechLayer* cut_layer,
      int first,
      int second,
      bool above,
      bool below)
  {
    odb::dbTechLayerCutEnclosureRule* rule = addCutEnclosureRule(
        cut_layer, first, second, odb::dbTechLayerCutEnclosureRule::DEFAULT);
    if (above) {
      rule->setAbove(true);
    }
    if (below) {
      rule->setBelow(true);
    }
    return rule;
  }

  static constexpr int kManufacturingGrid = 5;
};

}  // namespace pdn
