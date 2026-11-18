// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

// Builders for small physical designs in C++ unit tests: technology layers,
// vias, antenna rules, masters with pin geometry, routed wires and global
// route guides.  They depend only on odb, so they work with tst::DbFixture
// and need no STA.
//
// Like the DbFixture netlist helpers, these favor convenience over
// performance.  A test should build only the handful of objects that the
// behavior under test needs, so the construction reads as the test case.
//
// Coordinates and sizes are in database units unless a field says
// otherwise.  Antenna areas are in square microns, as in LEF.

namespace tst {

// Creates a technology with the given database units per micron and sets
// the database's units to match.
odb::dbTech* makeTech(odb::dbDatabase* db,
                      const char* name = "tech",
                      int dbu_per_micron = 1000);

// Creates a library for masters built with makeMaster.
odb::dbLib* makeLib(odb::dbDatabase* db,
                    odb::dbTech* tech,
                    const char* name = "lib");

// Creates a chip and its top block with the given die area.
odb::dbBlock* makeBlock(odb::dbDatabase* db,
                        odb::dbTech* tech,
                        const odb::Rect& die_area = {0, 0, 100000, 100000},
                        const char* name = "top");

struct LayerSpec
{
  odb::dbTechLayerDir direction{odb::dbTechLayerDir::HORIZONTAL};
  int width{100};
  int pitch{200};
  std::optional<int> thickness{};  // unset leaves the layer without one
};

odb::dbTechLayer* addRoutingLayer(odb::dbTech* tech,
                                  const char* name,
                                  const LayerSpec& spec = {});

odb::dbTechLayer* addCutLayer(odb::dbTech* tech, const char* name);

// A via with a square cut of cut_size, and square enclosures that extend
// enclosure past the cut on the bottom and top layers.  All shapes are
// centered on the via origin.
odb::dbTechVia* addVia(odb::dbTech* tech,
                       const char* name,
                       odb::dbTechLayer* bottom,
                       odb::dbTechLayer* cut,
                       odb::dbTechLayer* top,
                       int cut_size = 100,
                       int enclosure = 50);

// A uniform stack of routing layers M1..Mn with cut layers V1..V(n-1) and a
// via VIAij between each adjacent pair.  Directions alternate, starting
// with spec.direction on M1.
struct LayerStack
{
  std::vector<odb::dbTechLayer*> metals;
  std::vector<odb::dbTechLayer*> cuts;
  std::vector<odb::dbTechVia*> vias;

  // 1-based to match the layer names: metal(1) is M1, cut(1) is V1 and
  // via(1) connects M1 to M2.
  odb::dbTechLayer* metal(int level) const { return metals.at(level - 1); }
  odb::dbTechLayer* cut(int level) const { return cuts.at(level - 1); }
  odb::dbTechVia* via(int level) const { return vias.at(level - 1); }
};

LayerStack makeLayerStack(odb::dbTech* tech,
                          int num_metals,
                          const LayerSpec& spec = {});

// A piecewise-linear table of (index, ratio) points.  A single point is a
// constant, which odb stores the same way as LEF's single-value form.
struct Pwl
{
  std::vector<double> indices{};
  std::vector<double> ratios{};
};

inline Pwl constantPwl(double ratio)
{
  return {{0.0}, {ratio}};
}

// The default (OXIDE1) antenna rule of a layer.  Unset fields keep the
// rule's current value, so a spec can be applied on top of a LEF rule.
// The Pwl fields are the exception: odb cannot replace a table once it is
// set, so applyAntennaRule throws std::invalid_argument if one of them is
// already set on the layer.
struct AntennaRuleSpec
{
  std::optional<double> par{};  // ANTENNAAREARATIO
  std::optional<double> car{};  // ANTENNACUMAREARATIO
  std::optional<double> psr{};  // ANTENNASIDEAREARATIO
  std::optional<double> csr{};  // ANTENNACUMSIDEAREARATIO

  std::optional<Pwl> diff_par{};  // ANTENNADIFFAREARATIO
  std::optional<Pwl> diff_car{};  // ANTENNACUMDIFFAREARATIO
  std::optional<Pwl> diff_psr{};  // ANTENNADIFFSIDEAREARATIO
  std::optional<Pwl> diff_csr{};  // ANTENNACUMDIFFSIDEAREARATIO

  std::optional<double> area_factor{};  // ANTENNAAREAFACTOR
  bool area_factor_diffuse_only{false};
  std::optional<double> side_area_factor{};  // ANTENNASIDEAREAFACTOR
  bool side_area_factor_diffuse_only{false};

  std::optional<double> area_minus_diff{};  // ANTENNAAREAMINUSDIFF
  std::optional<double> gate_plus_diff{};   // ANTENNAGATEPLUSDIFF
  std::optional<Pwl> gate_plus_diff_pwl{};  // ANTENNAGATEPLUSDIFF PWL
  std::optional<Pwl> area_diff_reduce{};    // ANTENNAAREADIFFREDUCEPWL
  bool cum_routing_plus_cut{false};         // ANTENNACUMROUTINGPLUSCUT
};

// Creates the layer's default antenna rule if needed and applies spec.
odb::dbTechLayerAntennaRule* applyAntennaRule(odb::dbTechLayer* layer,
                                              const AntennaRuleSpec& spec);

struct PinSpec
{
  const char* name{nullptr};
  odb::dbIoType io_type{odb::dbIoType::INPUT};
  odb::dbTechLayer* layer{nullptr};   // nullptr creates a pin with no shape
  odb::Rect rect{};                   // in master coordinates
  std::optional<double> gate_area{};  // ANTENNAGATEAREA
  std::optional<double> diff_area{};  // ANTENNADIFFAREA
  odb::dbSigType sig_type{odb::dbSigType::SIGNAL};
};

// Creates a frozen master with one shape per pin.
odb::dbMaster* makeMaster(odb::dbLib* lib,
                          const char* name,
                          int width,
                          int height,
                          const std::vector<PinSpec>& pins,
                          odb::dbMasterType type = odb::dbMasterType::CORE);

// Builds a net's routed wire one path at a time:
//
//   RoutePath(net)
//       .on(m1, {0, 0}).to({5000, 0})
//       .via(via12).to({5000, 8000})  // continues on M2
//       .on(m1, {9000, 0}).to({9000, 3000})  // a separate path
//       .build();
//
// Each path runs along one layer until a via moves it to the via's other
// layer.  Consecutive points on a layer must share an x or y coordinate,
// and a via must connect to the current layer; either mistake throws
// std::invalid_argument rather than encoding a wire odb would mangle.
// build() finishes the wire and marks the net ROUTED; the destructor calls
// it if the test did not.  Any wire already on the net is replaced.
class RoutePath
{
 public:
  explicit RoutePath(odb::dbNet* net);
  ~RoutePath();

  RoutePath(const RoutePath&) = delete;
  RoutePath& operator=(const RoutePath&) = delete;

  // Starts a new path on layer at start.
  RoutePath& on(odb::dbTechLayer* layer, const odb::Point& start);
  // Extends the current path to point on the current layer.
  RoutePath& to(const odb::Point& point);
  // Places via at the current point and continues on its other layer.
  RoutePath& via(odb::dbTechVia* via);

  odb::dbWire* build();

 private:
  odb::dbNet* net_;
  odb::dbWire* wire_;
  odb::dbWireEncoder encoder_;
  odb::dbTechLayer* layer_{nullptr};
  odb::Point point_;
  bool built_{false};
};

// Adds a global route guide for net.  via_layer is the routing layer the
// guide connects to through a via; nullptr means none, which odb (like the
// global router) records as the guide's own layer.
odb::dbGuide* addGuide(odb::dbNet* net,
                       odb::dbTechLayer* layer,
                       const odb::Rect& rect,
                       odb::dbTechLayer* via_layer = nullptr);

}  // namespace tst
