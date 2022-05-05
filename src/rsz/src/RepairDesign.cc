/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "rsz/Resizer.hh"
#include "BufferedNet.hh"

#include "db_sta/dbNetwork.hh"

#include "sta/Units.hh"
#include "sta/Liberty.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Sdc.hh"
#include "sta/Corner.hh"
#include "sta/PathVertex.hh"
#include "sta/SearchPred.hh"
#include "sta/Search.hh"
#include "sta/PathRef.hh"
#include "sta/PathExpanded.hh"
#include "sta/Fuzzy.hh"

namespace rsz {

using std::abs;
using std::min;
using std::max;

using utl::RSZ;

using sta::NetPinIterator;
using sta::InstancePinIterator;
using sta::NetIterator;
using sta::PinConnectedPinIterator;
using sta::Clock;
using sta::INF;

// Repair long wires, max slew, max capacitance, max fanout violations
// The whole enchilada.
// max_wire_length zero for none (meters)
void
Resizer::repairDesign(double max_wire_length,
                      double slew_margin,
                      double max_cap_margin)
{
  int repaired_net_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repairDesign(max_wire_length, slew_margin, max_cap_margin,
               repaired_net_count, slew_violations, cap_violations,
               fanout_violations, length_violations);
  repair_design_buffer_count_ = inserted_buffer_count_;

  if (slew_violations > 0)
    logger_->info(RSZ, 34, "Found {} slew violations.", slew_violations);
  if (fanout_violations > 0)
    logger_->info(RSZ, 35, "Found {} fanout violations.", fanout_violations);
  if (cap_violations > 0)
    logger_->info(RSZ, 36, "Found {} capacitance violations.", cap_violations);
  if (length_violations > 0)
    logger_->info(RSZ, 37, "Found {} long wires.", length_violations);
  if (repair_design_buffer_count_ > 0)
    logger_->info(RSZ, 38, "Inserted {} buffers in {} nets.",
                  repair_design_buffer_count_,
                  repaired_net_count);
  if (resize_count_ > 0)
    logger_->info(RSZ, 39, "Resized {} instances.", resize_count_);
}

void
Resizer::repairDesign(double max_wire_length, // zero for none (meters)
                      double slew_margin,
                      double max_cap_margin,
                      int &repaired_net_count,
                      int &slew_violations,
                      int &cap_violations,
                      int &fanout_violations,
                      int &length_violations)
{
  repaired_net_count = 0;
  slew_violations = 0;
  cap_violations = 0;
  fanout_violations = 0;
  length_violations = 0;
  inserted_buffer_count_ = 0;
  resize_count_ = 0;

  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  incrementalParasiticsBegin();
  int max_length = metersToDbu(max_wire_length);
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex *drvr = level_drvr_vertices_[i];
    Pin *drvr_pin = drvr->pin();
    Net *net = network_->isTopLevelPort(drvr_pin)
      ? network_->net(network_->term(drvr_pin))
      : network_->net(drvr_pin);
    const char *dbg_net_name = nullptr;
    bool debug = net && dbg_net_name
      && sta::stringEq(sdc_network_->pathName(net), dbg_net_name);
    if (debug)
      logger_->setDebugLevel(RSZ, "repair_net", 3);
    if (net
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells and supply nets.
        && !drvr->isConstant())
      repairNet(net, drvr_pin, drvr, slew_margin, max_cap_margin,
                true, true, true, max_length, true,
                repaired_net_count, slew_violations, cap_violations,
                fanout_violations, length_violations);
    if (debug)
      logger_->setDebugLevel(RSZ, "repair_net", 0);
  }
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0)
    level_drvr_vertices_valid_ = false;
}

// repairDesign but restricted to clock network and
// no max_fanout/max_cap checks.
// Use max_wire_length zero for none (meters)
void
Resizer::repairClkNets(double max_wire_length)
{
  init();
  // Need slews to resize inserted buffers.
  sta_->findDelays();

  inserted_buffer_count_ = 0;
  resize_count_ = 0;

  int repaired_net_count = 0;
  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int max_length = metersToDbu(max_wire_length);
  incrementalParasiticsBegin();
  for (Clock *clk : sdc_->clks()) {
    const PinSet *clk_pins = sta_->pins(clk);
    if (clk_pins) {
      for (const Pin *clk_pin : *clk_pins) {
        Net *net = network_->isTopLevelPort(clk_pin)
          ? network_->net(network_->term(clk_pin))
          : network_->net(clk_pin);
        if (network_->isDriver(clk_pin)) {
          Vertex *drvr = graph_->pinDrvrVertex(clk_pin);
          // Do not resize clock tree gates.
          repairNet(net, clk_pin, drvr, 0.0, 0.0,
                    false, false, false, max_length, false,
                    repaired_net_count, slew_violations, cap_violations,
                    fanout_violations, length_violations);
        }
      }
    }
  }
  updateParasitics();
  incrementalParasiticsEnd();

  if (length_violations > 0)
    logger_->info(RSZ, 47, "Found {} long wires.", length_violations);
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 48, "Inserted {} buffers in {} nets.",
                  inserted_buffer_count_,
                  repaired_net_count);
    level_drvr_vertices_valid_ = false;
  }
}

// Repair one net (for debugging)
void
Resizer::repairNet(Net *net,
                   double max_wire_length, // meters
                   double slew_margin,
                   double max_cap_margin)
{
  init();

  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  resized_multi_output_insts_.clear();
  int repaired_net_count = 0;
  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int max_length = metersToDbu(max_wire_length);
  PinSet *drivers = network_->drivers(net);
  if (drivers && !drivers->empty()) {
    PinSet::Iterator drvr_iter(drivers);
    Pin *drvr_pin = drvr_iter.next();
    Vertex *drvr = graph_->pinDrvrVertex(drvr_pin);
    repairNet(net, drvr_pin, drvr, slew_margin, max_cap_margin,
              true, true, true, max_length, true,
              repaired_net_count, slew_violations, cap_violations,
              fanout_violations, length_violations);
  }

  if (slew_violations > 0)
    logger_->info(RSZ, 51, "Found {} slew violations.", slew_violations);
  if (fanout_violations > 0)
    logger_->info(RSZ, 52, "Found {} fanout violations.", fanout_violations);
  if (cap_violations > 0)
    logger_->info(RSZ, 53, "Found {} capacitance violations.", cap_violations);
  if (length_violations > 0)
    logger_->info(RSZ, 54, "Found {} long wires.", length_violations);
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 55, "Inserted {} buffers in {} nets.",
                  inserted_buffer_count_,
                  repaired_net_count);
    level_drvr_vertices_valid_ = false;
  }
  if (resize_count_ > 0)
    logger_->info(RSZ, 56, "Resized {} instances.", resize_count_);
  if (resize_count_ > 0)
    logger_->info(RSZ, 57, "Resized {} instances.", resize_count_);
}

void
Resizer::repairNet(Net *net,
                   const Pin *drvr_pin,
                   Vertex *drvr,
                   double slew_margin,
                   double max_cap_margin,
                   bool check_slew,
                   bool check_cap,
                   bool check_fanout,
                   int max_length, // dbu
                   bool resize_drvr,
                   int &repaired_net_count,
                   int &slew_violations,
                   int &cap_violations,
                   int &fanout_violations,
                   int &length_violations)
{
  // Hands off special nets.
  if (!db_network_->isSpecial(net)) {
    BufferedNetPtr bnet = makeBufferedNetSteiner(drvr_pin);
    if (bnet) {
      debugPrint(logger_, RSZ, "repair_net", 1, "repair net {}",
                 sdc_network_->pathName(drvr_pin));
      if (logger_->debugCheck(RSZ, "repair_net", 3))
        bnet->reportTree(this);
      // Resize the driver to normalize slews before repairing limit violations.
      if (resize_drvr)
        resizeToTargetSlew(drvr_pin, true);
      // For tristate nets all we can do is resize the driver.
      if (!isTristateDriver(drvr_pin)) {
        ensureWireParasitic(drvr_pin, net);
        graph_delay_calc_->findDelays(drvr);

        float max_load_slew = INF;
        float max_cap = INF;
        float max_fanout = INF;
        bool repair_slew = false;
        bool repair_cap = false;
        bool repair_fanout = false;
        bool repair_wire = false;
        const Corner *corner = sta_->cmdCorner();
        if (check_cap) {
          float cap1, max_cap1, cap_slack1;
          const Corner *corner1;
          const RiseFall *tr1;
          sta_->checkCapacitance(drvr_pin, nullptr, max_,
                                 corner1, tr1, cap1, max_cap1, cap_slack1);
          if (max_cap1 > 0.0 && corner1) {
            max_cap1 *= (1.0 - max_cap_margin / 100.0);
            max_cap = max_cap1;
            if (cap1 > max_cap1) {
              corner = corner1;
              cap_violations++;
              repair_cap = true;
            }
          }
        }
        if (check_fanout) {
          float fanout, fanout_slack;
          sta_->checkFanout(drvr_pin, max_,
                            fanout, max_fanout, fanout_slack);
          if (max_fanout > 0.0 && fanout_slack < 0.0) {
            fanout_violations++;
            repair_fanout = true;
          }
        }
        int wire_length = bnet->maxLoadWireLength();
        if (max_length
            && wire_length > max_length) {
          length_violations++;
          repair_wire = true;
        }
        if (check_slew) {
          float slew1, slew_slack1, max_slew1;
          const Corner *corner1;
          // Check slew at the driver.
          checkSlew(drvr_pin, slew_margin, slew1, max_slew1, slew_slack1, corner1);
          // Max slew violations at the driver pin are repaired by reducing the
          // load capacitance. Wire resistance may shield capacitance from the
          // driver but so this is conservative.
          // Find max load cap that corresponds to max_slew.
          LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
          if (corner1
              && max_slew1 > 0.0) {
            if (drvr_port) {
              float max_cap1 = findSlewLoadCap(drvr_port, max_slew1, corner1);
              max_cap = min(max_cap, max_cap1);
            }
            corner = corner1;
            if (slew_slack1 < 0.0) {
              debugPrint(logger_, RSZ, "repair_net", 2, "drvr slew violation slew={} max_slew={}",
                         delayAsString(slew1, this, 3),
                         delayAsString(max_slew1, this, 3));
              repair_slew = true;
              slew_violations++;
            }
          }
          // Check slew at the loads.
          // Note that many liberty libraries do not have max_transition attributes on
          // input pins.
          // Max slew violations at the load pins are repaired by inserting buffers
          // and reducing the wire length to the load.
          checkLoadSlews(drvr_pin, slew_margin, slew1, max_slew1, slew_slack1, corner1);
          if (slew_slack1 < 0.0) {
            debugPrint(logger_, RSZ, "repair_net", 2,
                       "load slew violation load_slew={} max_slew={}",
                       delayAsString(slew1, this, 3),
                       delayAsString(max_slew1, this, 3));
            corner = corner1;
            // Don't double count the driver/load on same net.
            if (!repair_slew)
              slew_violations++;
            repair_slew = true;
          }
        }
        if (repair_slew
            || repair_cap
            || repair_fanout
            || repair_wire) {
          Point drvr_loc = db_network_->location(drvr->pin());
          debugPrint(logger_, RSZ, "repair_net", 1, "driver {} ({} {}) l={}",
                     sdc_network_->pathName(drvr_pin),
                     units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getX()), 1),
                     units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getY()), 1),
                     units_->distanceUnit()->asString(dbuToMeters(wire_length), 1));
          int wire_length;
          float pin_cap, fanout;
          PinSeq load_pins;
          repairNet(bnet, net, drvr_pin,
                    max_cap, max_fanout, max_length, corner, 0,
                    wire_length, pin_cap, fanout, load_pins, max_load_slew);
          repaired_net_count++;

          if (resize_drvr)
            resizeToTargetSlew(drvr_pin, true);
        }
      }
    }
  }
}

bool
Resizer::checkLimits(const Pin *drvr_pin,
                     double slew_margin,
                     double max_cap_margin,
                     bool check_slew,
                     bool check_cap,
                     bool check_fanout)
{
  if (check_cap) {
    float cap1, max_cap1, cap_slack1;
    const Corner *corner1;
    const RiseFall *tr1;
    sta_->checkCapacitance(drvr_pin, nullptr, max_,
                           corner1, tr1, cap1, max_cap1, cap_slack1);
    max_cap1 *= (1.0 - max_cap_margin / 100.0);
    if (cap1 < max_cap1)
      return true;
  }
  if (check_fanout) {
    float fanout, fanout_slack, max_fanout;
    sta_->checkFanout(drvr_pin, max_,
                      fanout, max_fanout, fanout_slack);
    if (fanout_slack < 0.0)
      return true;

  }
  if (check_slew) {
    float slew1, slew_slack1, max_slew1;
    const Corner *corner1;
    checkSlew(drvr_pin, slew_margin, slew1, max_slew1, slew_slack1, corner1);
    if (slew_slack1 < 0.0)
      return true;
    checkLoadSlews(drvr_pin, slew_margin, slew1, max_slew1, slew_slack1, corner1);
    if (slew_slack1 < 0.0)
      return true;
  }
  return false;
}

void
Resizer::checkSlew(const Pin *drvr_pin,
                   double slew_margin,
                   // Return values.
                   Slew &slew,
                   float &limit,
                   float &slack,
                   const Corner *&corner)
{
  slack = INF;
  limit = INF;
  corner = nullptr;

  const Corner *corner1;
  const RiseFall *tr1;
  Slew slew1;
  float limit1, slack1;
  sta_->checkSlew(drvr_pin, nullptr, max_, false,
                  corner1, tr1, slew1, limit1, slack1);
  if (corner1) {
    limit1 *= (1.0 - slew_margin / 100.0);
    slack1 = limit1 - slew1;
    if (slack1 < slack) {
      slew = slew1;
      limit = limit1;
      slack = slack1;
      corner = corner1;
    }
  }
}

void
Resizer::checkLoadSlews(const Pin *drvr_pin,
                        double slew_margin,
                        // Return values.
                        Slew &slew,
                        float &limit,
                        float &slack,
                        const Corner *&corner)
{
  slack = INF;
  limit = INF;
  PinConnectedPinIterator *pin_iter = network_->connectedPinIterator(drvr_pin);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (pin != drvr_pin) {
      const Corner *corner1;
      const RiseFall *tr1;
      Slew slew1;
      float limit1, slack1;
      sta_->checkSlew(pin, nullptr, max_, false,
                      corner1, tr1, slew1, limit1, slack1);
      if (corner1) {
        limit1 *= (1.0 - slew_margin / 100.0);
        limit = min(limit, limit1);
        slack1 = limit1 - slew1;
        if (slack1 < slack) {
          slew = slew1;
          slack = slack1;
          corner = corner1;
        }
      }
    }
  }
  delete pin_iter;
}

float
Resizer::bufferInputMaxSlew(LibertyCell *buffer,
                            const Corner *corner) const
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  return maxInputSlew(input, corner);
}

float
Resizer::maxInputSlew(const LibertyPort *input,
                      const Corner *corner) const
{
  float limit;
  bool exists;
  sta_->findSlewLimit(input, corner, MinMax::max(), limit, exists);
  // umich brain damage control
  if (limit == 0.0)
    limit = INF;
  return limit;
}

// Find the output port load capacitance that results in slew.
double
Resizer::findSlewLoadCap(LibertyPort *drvr_port,
                         double slew,
                         const Corner *corner)
{
  const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
  double drvr_res = drvr_port->driveResistance();
  if (drvr_res == 0.0)
    return INF;
  // cap1 lower bound
  // cap2 upper bound
  double cap1 = 0.0;
  double cap2 = slew / drvr_res * 2;
  double tol = .01; // 1%
  double diff1 = gateSlewDiff(drvr_port, cap2, slew, dcalc_ap);
  // binary search for diff = 0.
  while (abs(cap1 - cap2) > max(cap1, cap2) * tol) {
    if (diff1 < 0.0) {
      cap1 = cap2;
      cap2 *= 2;
      diff1 = gateSlewDiff(drvr_port, cap2, slew, dcalc_ap);
    }
    else {
      double cap3 = (cap1 + cap2) / 2.0;
      double diff2 = gateSlewDiff(drvr_port, cap3, slew, dcalc_ap);
      if (diff2 < 0.0) {
        cap1 = cap3;
      }
      else {
        cap2 = cap3;
        diff1 = diff2;
      }
    }
  }
  return cap1;
}

// objective function
double
Resizer::gateSlewDiff(LibertyPort *drvr_port,
                      double load_cap,
                      double slew,
                      const DcalcAnalysisPt *dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  Slew gate_slew = max(slews[RiseFall::riseIndex()], slews[RiseFall::fallIndex()]);
  return gate_slew - slew;
}

void
Resizer::repairNet(BufferedNetPtr bnet,
                   Net *net,
                   const Pin *drvr_pin,
                   float max_cap,
                   float max_fanout,
                   int max_length, // dbu
                   const Corner *corner,
                   int level,
                   // Return values.
                   // Remaining parasiics after repeater insertion.
                   int &wire_length, // dbu
                   float &pin_cap,
                   float &fanout,
                   PinSeq &load_pins,
                   float &max_load_slew)
{
  switch (bnet->type()) {
  case BufferedNetType::wire:
    repairNetWire(bnet, net, drvr_pin,
                  max_cap, max_fanout, max_length, corner, level,
                  wire_length, pin_cap, fanout, load_pins, max_load_slew);
    break;
  case BufferedNetType::junction:
    repairNetJunc(bnet, net, drvr_pin,
                  max_cap, max_fanout, max_length, corner, level,
                  wire_length, pin_cap, fanout, load_pins, max_load_slew);
    break;
  case BufferedNetType::load:
    repairNetLoad(bnet, net, drvr_pin,
                  max_cap, max_fanout, max_length, corner, level,
                  wire_length, pin_cap, fanout, load_pins, max_load_slew);
    break;
  case BufferedNetType::buffer:
    logger_->critical(RSZ, 72, "unhandled BufferedNet type");
    break;
  }
}

void
Resizer::repairNetWire(BufferedNetPtr bnet,
                       Net *net,
                       const Pin *drvr_pin,
                       float max_cap,
                       float max_fanout,
                       int max_length, // dbu
                       const Corner *corner,
                       int level,
                       // Return values.
                       // Remaining parasiics after repeater insertion.
                       int &wire_length, // dbu
                       float &pin_cap,
                       float &fanout,
                       PinSeq &load_pins,
                       float &max_load_slew)
{
  repairNet(bnet->ref(), net, drvr_pin, max_cap, max_fanout, max_length,
            corner, level+1,
            wire_length, pin_cap, fanout,
            load_pins, max_load_slew);

  Point pt_loc = bnet->ref()->location();
  int pt_x = pt_loc.getX();
  int pt_y = pt_loc.getY();
  Point prev_loc = bnet->location();
  int length = Point::manhattanDistance(prev_loc, pt_loc);
  wire_length += length;
  // Back up from pt to prev_pt adding repeaters as necessary for
  // length/max_cap/max_slew violations.
  int prev_x = prev_loc.getX();
  int prev_y = prev_loc.getY();
  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}wl={} l={}",
             "", level,
             units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
             units_->distanceUnit()->asString(dbuToMeters(length), 1));
  double wire_length1 = dbuToMeters(wire_length);
  double wire_cap = wireSignalCapacitance(corner);
  double wire_res = wireSignalResistance(corner);
  double load_cap = pin_cap + wire_length1 * wire_cap;
  float r_drvr = driveResistance(drvr_pin);
  double load_slew = (r_drvr + wire_length1 * wire_res) *
    load_cap * elmore_skew_factor_;
  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}load_slew={} r_drvr={}",
             "", level,
             delayAsString(load_slew, this, 3),
             units_->resistanceUnit()->asString(r_drvr, 3));

  LibertyCell *buffer_cell = findTargetCell(buffer_lowest_drive_, load_cap, false);
  while ((max_length > 0 && wire_length > max_length)
         || (wire_cap > 0.0
             && load_cap > max_cap)
         || load_slew > max_load_slew) {
    // Make the wire a bit shorter than necessary to allow for
    // offset from instance origin to pin and detailed placement movement.
    static double length_margin = .05;
    bool split_wire = false;
    bool resize = true;
    // Distance from loads to repeater.
    int split_length = std::numeric_limits<int>::max();
    if (max_length > 0 && wire_length > max_length) {
      debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}max wire length violation {} > {}",
                 "", level,
                 units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
                 units_->distanceUnit()->asString(dbuToMeters(max_length), 1));
      split_length = min(split_length, max_length);
      split_wire = true;
    }
    if (wire_cap > 0.0
        && load_cap > max_cap) {
      debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}max cap violation {} > {}",
                 "", level,
                 units_->capacitanceUnit()->asString(load_cap, 3),
                 units_->capacitanceUnit()->asString(max_cap, 3))
        split_length = min(split_length, metersToDbu((max_cap - pin_cap) / wire_cap));
      split_wire = true;
    }
    if (load_slew > max_load_slew) { 
      debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}max load slew violation {} > {}",
                 "", level,
                 delayAsString(load_slew, this, 3),
                 delayAsString(max_load_slew, this, 3));
      // Using elmore delay to approximate wire
      // load_slew = (Rbuffer + L*Rwire) * (L*Cwire + Cpin) * k_threshold
      // Setting this to max_slew_load is a quadratic in L
      // L^2*Rwire*Cwire + L*(Rbuffer*Cwire + Rwire*Cpin) + Rbuffer*Cpin - max_load_slew/k_threshold
      // Solve using quadradic eqn for L.
      float r_buffer = bufferDriveResistance(buffer_cell);
      float a = wire_res * wire_cap;
      float b = r_buffer * wire_cap + wire_res * pin_cap;
      float c = r_buffer * pin_cap - max_load_slew / elmore_skew_factor_;
      float l = (-b + sqrt(b*b - 4 * a * c)) / (2 * a);
      if (l >= 0.0) {
        split_length = min(split_length, metersToDbu(l));
        split_wire = true;
        resize = false;
      }
      else {
        split_length = 0;
        split_wire = true;
        resize = false;
      }
      debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}split length={}",
                 "", level,
                 units_->distanceUnit()->asString(dbuToMeters(split_length), 1));
    }
    if (split_wire) {
      // Distance from pt to repeater backward toward prev_pt.
      // Note that split_length can be longer than the wire length
      // because it is the maximum value that satisfies max slew/cap.
      double buf_dist = (split_length >= wire_length)
        ? length
        : length - (wire_length - split_length * (1.0 - length_margin));
      double dx = prev_x - pt_x;
      double dy = prev_y - pt_y;
      double d = (length == 0) ? 0.0 : buf_dist / length;
      int buf_x = pt_x + d * dx;
      int buf_y = pt_y + d * dy;
      makeRepeater("wire", buf_x, buf_y, buffer_cell, corner, resize, level,
                   wire_length, pin_cap, fanout, load_pins, max_load_slew);
      // Update for the next round.
      length -= buf_dist;
      wire_length = length;
      pt_x = buf_x;
      pt_y = buf_y;

      wire_length1 = dbuToMeters(wire_length);
      load_cap = pin_cap + wire_length1 * wire_cap;
      load_slew = (r_drvr + wire_length1 * wire_res) *
        load_cap * elmore_skew_factor_;
      buffer_cell = findTargetCell(buffer_lowest_drive_, load_cap, false);
      debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}wl={} l={}",
                 "", level,
                 units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
                 units_->distanceUnit()->asString(dbuToMeters(length), 1));
    }
    else
      break;
  }
}

void
Resizer::repairNetJunc(BufferedNetPtr bnet,
                       Net *net,
                       const Pin *drvr_pin,
                       float max_cap,
                       float max_fanout,
                       int max_length, // dbu
                       const Corner *corner,
                       int level,
                       // Return values.
                       // Remaining parasiics after repeater insertion.
                       int &wire_length, // dbu
                       float &pin_cap,
                       float &fanout,
                       PinSeq &load_pins,
                       float &max_load_slew)
{
  Point loc = bnet->location();
  double wire_cap = wireSignalCapacitance(corner);
  double wire_res = wireSignalResistance(corner);

  BufferedNetPtr left = bnet->ref();
  int wire_length_left = 0;
  float pin_cap_left = 0.0;
  float fanout_left = 0.0;
  float max_load_slew_left = INF;
  PinSeq loads_left;
  if (left)
    repairNet(left, net, drvr_pin, max_cap, max_fanout, max_length,
              corner, level+1,
              wire_length_left, pin_cap_left, fanout_left,
              loads_left, max_load_slew_left);
  double cap_left = pin_cap_left + dbuToMeters(wire_length_left) * wire_cap;

  BufferedNetPtr right = bnet->ref2();
  int wire_length_right = 0;
  float pin_cap_right = 0.0;
  float fanout_right = 0.0;
  PinSeq loads_right;
  float max_load_slew_right = INF;
  if (right)
    repairNet(right, net, drvr_pin, max_cap, max_fanout, max_length,
              corner, level+1,
              wire_length_right, pin_cap_right, fanout_right,
              loads_right, max_load_slew_right);
  double cap_right = pin_cap_right + dbuToMeters(wire_length_right) * wire_cap;

  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}left  l={} pin_cap={} cap={} fanout={}",
             "", level,
             units_->distanceUnit()->asString(dbuToMeters(wire_length_left), 1),
             units_->capacitanceUnit()->asString(pin_cap_left, 3),
             units_->capacitanceUnit()->asString(cap_left, 3),
             fanout_left);
  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}right l={} pin_cap={} cap={} fanout={}",
             "", level,
             units_->distanceUnit()->asString(dbuToMeters(wire_length_right), 1),
             units_->capacitanceUnit()->asString(pin_cap_right, 3),
             units_->capacitanceUnit()->asString(cap_right, 3),
             fanout_right);

  wire_length = wire_length_left + wire_length_right;
  float wire_length1 = dbuToMeters(wire_length);
  pin_cap = pin_cap_left + pin_cap_right;
  float load_cap = cap_left + cap_right;
  fanout = fanout_left + fanout_right;
  max_load_slew = min(max_load_slew_left, max_load_slew_right);
  LibertyCell *buffer_cell = findTargetCell(buffer_lowest_drive_, load_cap, false);

  // Check for violations when the left/right branches are combined.
  // Add a buffer to left or right branch to stay under the max cap/length/fanout.
  bool repeater_left = false;
  bool repeater_right = false;
  float r_drvr = driveResistance(drvr_pin);
  float load_slew = (r_drvr + wire_length1 * wire_res)
    * load_cap * elmore_skew_factor_;
  bool load_slew_violation = load_slew > max_load_slew;
  // Driver slew checks were converted to max cap.
  if (load_slew_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}load slew violation {} > {}",
               "", level,
               delayAsString(load_slew, this, 3),
               delayAsString(max_load_slew, this, 3));
    float slew_slack_left = max_load_slew_left
      - (r_drvr + dbuToMeters(wire_length_left) * wire_res)
      * cap_left * elmore_skew_factor_;
    float slew_slack_right = max_load_slew_right
      - (r_drvr + dbuToMeters(wire_length_right) * wire_res)
      * cap_right * elmore_skew_factor_;
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s} slew slack left={} right={}",
               "", level,
               delayAsString(slew_slack_left, this, 3),
               delayAsString(slew_slack_right, this, 3));
    // Isolate the branch with the smaller slack by buffering the OTHER branch.
    if (slew_slack_left < slew_slack_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  bool cap_violation = (cap_left + cap_right) > max_cap;
  if (cap_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}cap violation", "", level);
    if (cap_left > cap_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  bool length_violation = max_length > 0
    && (wire_length_left + wire_length_right) > max_length;
  if (length_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}length violation", "", level);
    if (wire_length_left > wire_length_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  bool fanout_violation = max_fanout > 0
    // Note that if both fanout_left==max_fanout and fanout_right==max_fanout
    // there is no way repair the violation (adding a buffer to either branch
    // results in max_fanout+1, which is a violation).
    // Leave room for one buffer on the other branch by using >= to avoid
    // this situation.
    && (fanout_left + fanout_right) >= max_fanout;
  if (fanout_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}fanout violation", "", level);
    if (fanout_left > fanout_right)
      repeater_left = true;
    else
      repeater_right = true;
  }

  if (repeater_left)
    makeRepeater("left", loc, buffer_cell, corner, true, level,
                 wire_length_left, pin_cap_left, fanout_left, loads_left,
                 max_load_slew_left);
  if (repeater_right)
    makeRepeater("right", loc, buffer_cell, corner, true, level,
                 wire_length_right, pin_cap_right, fanout_right, loads_right,
                 max_load_slew_right);

  // Update after left/right repeaters are inserted.
  wire_length = wire_length_left + wire_length_right;
  pin_cap = pin_cap_left + pin_cap_right;
  fanout = fanout_left + fanout_right;
  max_load_slew = min(max_load_slew_left, max_load_slew_right);

  // Union left/right load pins.
  for (Pin *load_pin : loads_left)
    load_pins.push_back(load_pin);
  for (Pin *load_pin : loads_right)
    load_pins.push_back(load_pin);
}

void
Resizer::repairNetLoad(BufferedNetPtr bnet,
                       Net *net,
                       const Pin *drvr_pin,
                       float max_cap,
                       float max_fanout,
                       int max_length, // dbu
                       const Corner *corner,
                       int level,
                       // Return values.
                       // Remaining parasiics after repeater insertion.
                       int &wire_length, // dbu
                       float &pin_cap,
                       float &fanout,
                       PinSeq &load_pins,
                       float &max_load_slew)
{
  Pin *load_pin = bnet->loadPin();
  debugPrint(logger_, RSZ, "repair_net", 2, "{:{}s}load {}",
             "", level,
             sdc_network_->pathName(load_pin));
  LibertyPort *load_port = network_->libertyPort(load_pin);
  if (load_port) {
    pin_cap = load_port->capacitance();
    fanout = portFanoutLoad(load_port);
    max_load_slew = maxInputSlew(load_port, corner);
  }
  else {
    pin_cap = 0.0;
    fanout = 1;
    max_load_slew = INF;
  }
  load_pins.push_back(load_pin);
}

LibertyCell *
Resizer::findBufferUnderSlew(float max_slew,
                             float load_cap)
{
  LibertyCell *min_slew_buffer = buffer_lowest_drive_;
  float min_slew = INF;
  LibertyCellSeq *equiv_cells = sta_->equivCells(buffer_lowest_drive_);
  if (equiv_cells) {
    sort(equiv_cells, [this] (const LibertyCell *buffer1,
                              const LibertyCell *buffer2) {
      return bufferDriveResistance(buffer1)
        > bufferDriveResistance(buffer2);
    });
    for (LibertyCell *buffer : *equiv_cells) {
      if (!dontUse(buffer)
          && isLinkCell(buffer)) {
        float slew = bufferSlew(buffer, load_cap, tgt_slew_dcalc_ap_);
        debugPrint(logger_, RSZ, "buffer_under_slew", 1, "{:{}s}pt ({} {})",
                   buffer->name(),
                   units_->timeUnit()->asString(slew));
        if (slew < max_slew) {
          return buffer;
        }
        if (slew < min_slew) {
          min_slew_buffer = buffer;
          min_slew = slew;
        }
      }
    }
  }
  // Could not find a buffer under max_slew but this is min slew achievable.
  return min_slew_buffer;
}

} // namespace
