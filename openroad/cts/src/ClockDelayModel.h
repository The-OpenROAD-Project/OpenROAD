// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "CtsOptions.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/ArcDelayCalc.hh"

namespace sta {
class ArcDelayCalc;
class Pin;
class dbSta;
class dbNetwork;
class LibertyCell;
class LibertyPort;
class Scene;
class TimingArc;
class RiseFall;
}  // namespace sta

namespace est {
class EstimateParasitics;
}  // namespace est

namespace rsz {
class Resizer;
}  // namespace rsz

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

// A pin of a clock net: its location in DBU and its capacitance.
struct NetPin
{
  int x = 0;
  int y = 0;
  double cap = 0.0;
  // The pin's lowest routing layer, from which vias reach the wire.
  odb::dbTechLayer* layer = nullptr;
};

// The wire of a clock net as the timer estimates it from the placement: a
// Steiner tree over its pins, reduced to a pi model at the driver and an
// Elmore delay to each load. Each value is held for both scenes of the model.
struct NetWire
{
  // Wire and pin capacitance.
  std::array<double, 2> cap{};
  // Pi model: capacitance at the driver, resistance, capacitance behind it.
  std::array<double, 2> c_near{};
  std::array<double, 2> r_pi{};
  std::array<double, 2> c_far{};
  // Elmore delay from the driver to each load, in the order given.
  std::array<std::vector<double>, 2> elmore;
  // Wire length in DBU.
  double length = 0.0;
};

// The timing of a clock pin with clock logic behind it, such as a clock
// gate's: the cell from the pin to its clock output, the load on that output,
// the delay from the output to the registers, and their number. A pin with a
// delay behind it and no cell has a negative `cell`.
struct ClockPinModel
{
  int cell = -1;
  // Delay of the cell at the model's nominal input slew, and the load that
  // gives its change with the input slew.
  double delay = 0.0;
  double load = 0.0;
  double after = 0.0;
  double registers = 1.0;
};

struct StageTiming
{
  double delay = 0.0;
  double slew = 0.0;
  // Effective capacitance the driver sees.
  double load = 0.0;
  // Delay from the driver to each load and the slew there.
  std::vector<double> wire_delay;
  std::vector<double> load_slew;
};

// Delay and slew of the clock cells and wires, taken from the Liberty tables
// and the wire RC the timer uses for placement parasitics. Delays are those of
// `kBalance`, the scene the trees are balanced at; `kSlow`, the scene with the
// slowest cells, is where slews are held within the limit. SI units.
class ClockDelayModel
{
 public:
  enum Corner
  {
    kBalance = 0,
    kSlow = 1
  };

  ClockDelayModel(CtsOptions* options,
                  odb::dbDatabase* db,
                  sta::dbSta* sta,
                  sta::dbNetwork* network,
                  est::EstimateParasitics* estimate_parasitics,
                  rsz::Resizer* resizer,
                  utl::Logger* logger);

  // The clock buffers are cells 0 to numBuffers() - 1, in rising order of
  // drive strength.
  int numBuffers() const { return num_buffers_; }
  int strongestBuffer() const { return num_buffers_ - 1; }
  int findBuffer(const std::string& master) const;
  // Makes a cell of the timing arc from `from` to `to`, such as a clock
  // gate's; returns -1 when the cell has no such arc.
  int addCell(sta::LibertyCell* cell,
              const sta::LibertyPort* from,
              const sta::LibertyPort* to);

  const std::string& cellName(int cell) const { return cells_[cell].name; }
  odb::dbMaster* master(int cell) const { return cells_[cell].master; }
  double inputCap(int cell) const { return cells_[cell].input_cap; }
  // Largest load the cell drives with its output slew within the share of
  // the slew limit a driver may take.
  double maxLoad(int cell) const { return cells_[cell].max_load; }
  // Pin locations from the center of the cell.
  odb::Point inputPin(int cell) const { return cells_[cell].in; }
  odb::Point outputPin(int cell) const { return cells_[cell].out; }
  odb::dbTechLayer* inputLayer(int cell) const { return cells_[cell].in_layer; }
  odb::dbTechLayer* outputLayer(int cell) const
  {
    return cells_[cell].out_layer;
  }
  static odb::dbTechLayer* pinLayer(odb::dbMTerm* pin);
  static odb::dbTechLayer* pinLayer(odb::dbBTerm* pin);
  // Whether the timer takes a net with this load for a clock net's last.
  bool isRegisterClock(odb::dbITerm* pin) const;

  void gate(Corner corner,
            int cell,
            double in_slew,
            double load,
            double& delay,
            double& slew) const;
  double delay(int cell, double in_slew, double load) const;
  ~ClockDelayModel();

  // The cell driving `wire`, as the timer's delay calculator has it.
  StageTiming drive(Corner corner,
                    int cell,
                    double in_slew,
                    const NetWire& wire) const;
  // A driver without a cell, such as a port, with `slew` at its output.
  StageTiming port(Corner corner, double slew, const NetWire& wire) const;
  // Delay of the cell at `in_slew` when it has `delay` at the nominal slew.
  double delayAt(int cell, double delay, double in_slew, double load) const;
  // Capacitance of a pin as the timer counts it on its net.
  double pinCap(odb::dbITerm* pin) const;
  // Capacitance set on a port of the design with set_load.
  double portCap(odb::dbBTerm* port) const;
  // Delay and slew at a load `elmore` behind a driver with `drvr_slew`.
  void wireEnd(double drvr_slew,
               double elmore,
               double& delay,
               double& slew) const;

  // `leaf`: the net has a register clock pin, which the timer estimates with
  // the signal wire RC in place of the clock wire RC.
  NetWire wire(const NetPin& driver,
               const std::vector<NetPin>& loads,
               bool leaf) const;
  // A driver with one wire to each load and nothing shared between them.
  NetWire starWire(const NetPin& driver,
                   const std::vector<NetPin>& loads,
                   bool leaf) const;
  // Capacitance of a wire of `length` DBU.
  double wireCap(double length, bool leaf) const;

  // Largest slew a clock pin may have, at kSlow.
  double slewLimit() const { return slew_limit_; }
  // Whether `cell` drives `wire` with every load within the slew limit.
  bool slewOk(int cell, const NetWire& wire) const;
  // Whether a driver without a cell, with `slew` at its output at kSlow,
  // keeps every load of `wire` within the slew limit.
  bool portSlewOk(double slew, const NetWire& wire) const;
  // Largest slew on the net at kSlow, on either edge, with the largest slew
  // a clock pin may have at the input of `cell`.
  double worstSlew(int cell, const NetWire& wire) const;
  // The input slew delays are quoted at, at kBalance.
  double nominalSlew() const { return nominal_slew_; }
  // Longest wire, in DBU, the strongest buffer drives into one of its own
  // inputs within the slew limit.
  double maxHop() const { return max_hop_; }
  double dbuToMeters(double dbu) const { return dbu * meters_per_dbu_; }
  double dbuPerMicron() const { return dbu_per_micron_; }
  const sta::Scene* balanceScene() const { return scenes_[kBalance]; }
  const sta::Scene* slowScene() const { return scenes_[kSlow]; }

 private:
  struct Cell
  {
    std::string name;
    odb::dbMaster* master = nullptr;
    const sta::LibertyCell* liberty = nullptr;
    // From the rising edge of the input, and from the falling edge.
    const sta::TimingArc* arc = nullptr;
    const sta::TimingArc* fall_arc = nullptr;
    double input_cap = 0.0;
    double max_load = 0.0;
    // Pin locations from the center of the master, and the pins' layers.
    odb::Point in;
    odb::Point out;
    odb::dbTechLayer* in_layer = nullptr;
    odb::dbTechLayer* out_layer = nullptr;
  };

  // Wire resistance and capacitance per meter, horizontal and vertical.
  struct WireRc
  {
    double r_h = 0.0;
    double r_v = 0.0;
    double c_h = 0.0;
    double c_v = 0.0;
  };

  struct TreeNode
  {
    int parent = -1;
    int dx = 0;
    int dy = 0;
    double cap = 0.0;
    // The node is a pin, joined to its parent by the vias from this layer.
    bool pin = false;
    odb::dbTechLayer* layer = nullptr;
  };

  // Finds the clock buffers that have a delay table, weakest first.
  void findBuffers();
  // Takes the slew thresholds of the buffers' library, and input pins of the
  // design to stand in for the loads of the nets the delay calculator is
  // asked about; the first keys the scratch parasitic.
  void initDelayCalc();
  void reportCells() const;
  void findScenes();
  void findSlewLimit();
  void findMaxLoad(Cell& cell, const sta::LibertyPort* out);
  void findPins(Cell& cell,
                const sta::LibertyPort* in,
                const sta::LibertyPort* out) const;
  void findMaxHop();
  const sta::TimingArc* findArc(const sta::LibertyCell* cell,
                                const sta::LibertyPort* from,
                                const sta::LibertyPort* to,
                                const sta::RiseFall* edge) const;
  void gate(Corner corner,
            const sta::TimingArc* arc,
            double in_slew,
            double load,
            double& delay,
            double& slew) const;
  StageTiming drive(Corner corner,
                    const sta::TimingArc* arc,
                    double in_slew,
                    const NetWire& wire) const;
  // Fills `wire` from a tree whose node 0 is the driver and whose nodes
  // come after their parents; `load_nodes` gives each load's node.
  void reduce(const std::vector<TreeNode>& tree,
              const std::vector<int>& load_nodes,
              bool leaf,
              NetWire& wire) const;
  // Adds the pins to a tree over their locations, whose node 0 is at the
  // driver, and reduces it. `load_nodes` gives the node at each load.
  NetWire finish(std::vector<TreeNode> tree,
                 const std::vector<int>& load_nodes,
                 const NetPin& driver,
                 const std::vector<NetPin>& loads,
                 bool leaf) const;
  // Makes the tree of the Steiner tree over the pins at distinct `points`,
  // of which point 0 is the driver, and gives each load's node; false when
  // the Steiner tree does not hold every pin.
  bool steinerTree(const NetPin& driver,
                   const std::vector<NetPin>& loads,
                   const std::map<std::pair<int, int>, int>& points,
                   const std::vector<int>& load_points,
                   std::vector<TreeNode>& tree,
                   std::vector<int>& load_nodes) const;
  double viaResistance(odb::dbTechLayer* layer, int corner, bool leaf) const;
  // Time at which a ramp of `ramp` seconds through a pole at `tau` crosses
  // the fraction `v` of its swing.
  static double crossing(double ramp, double tau, double v);

  CtsOptions* options_;
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  est::EstimateParasitics* estimate_parasitics_;
  rsz::Resizer* resizer_;
  utl::Logger* logger_;

  std::vector<Cell> cells_;
  int num_buffers_ = 0;
  // The timer's delay calculator works on pins of the design: a scratch
  // parasitic kept under `key_pin_`, with `stand_ins_` for the load pins.
  sta::ArcDelayCalc* arc_delay_calc_ = nullptr;
  const sta::Pin* key_pin_ = nullptr;
  std::vector<const sta::Pin*> stand_ins_;
  mutable std::vector<std::unique_ptr<sta::LoadPinIndexMap>> load_maps_;
  mutable std::map<std::tuple<odb::dbTechLayer*, int, bool>, double> vias_;
  std::array<const sta::Scene*, 2> scenes_{};
  // [corner][leaf]
  std::array<std::array<WireRc, 2>, 2> rc_{};
  double slew_limit_ = 0.0;
  double nominal_slew_ = 0.0;
  double max_hop_ = 0.0;
  double meters_per_dbu_ = 0.0;
  double dbu_per_micron_ = 0.0;
  // Slew measurement thresholds of the buffers' library.
  double slew_lower_ = 0.2;
  double slew_upper_ = 0.8;
  double slew_derate_ = 1.0;
  double delay_threshold_ = 0.5;
};

}  // namespace cts
