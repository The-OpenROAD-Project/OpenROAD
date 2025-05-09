// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "SwapPinsMove.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "BaseMove.hh"

namespace rsz {

using std::string;

using utl::RSZ;

using sta::ArcDcalcResult;
using sta::ArcDelay;
using sta::DcalcAnalysisPt;
using sta::INF;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::TimingArc;
using sta::TimingArcSet;
using sta::Vertex;

bool SwapPinsMove::doMove(const Path* drvr_path,
                          int drvr_index,
                          Slack drvr_slack,
                          PathExpanded* expanded,
                          float setup_slack_margin)
{
  Pin* drvr_pin = drvr_path->pin(this);
  // Skip if there is no liberty model or this is a single-input cell
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  if (drvr_port == nullptr) {
    return false;
  }
  LibertyCell* cell = drvr_port->libertyCell();
  if (cell == nullptr) {
    return false;
  }
  if (cell->isBuffer() || cell->isInverter()) {
    return false;
  }
  Instance* drvr = network_->instance(drvr_pin);
  const DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  // int lib_ap = dcalc_ap->libertyIndex(); : check cornerPort
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  const int in_index = drvr_index - 1;
  const Path* in_path = expanded->path(in_index);
  Pin* in_pin = in_path->pin(sta_);

  if (!resizer_->dontTouch(drvr)) {
    // We get the driver port and the cell for that port.
    LibertyPort* input_port = network_->libertyPort(in_pin);
    LibertyPort* swap_port = input_port;
    sta::LibertyPortSet ports;

    // Skip output to output paths
    if (input_port->direction()->isOutput()) {
      return false;
    }

    // Check if we have already dealt with this instance
    // and prevent any further swaps.
    if (hasMoves(drvr) > 0) {
      return false;
    }

    // Find the equivalent pins for a cell (simple implementation for now)
    // stash them. Ports are unique to a cell so we can just cache by port
    // and that should apply to all instances of that cell with this input_port.
    if (equiv_pin_map_.find(input_port) == equiv_pin_map_.end()) {
      equivCellPins(cell, input_port, ports);
      equiv_pin_map_.insert(input_port, ports);
    }
    ports = equiv_pin_map_[input_port];
    if (!ports.empty()) {
      // Pass slews at input pins for more accurate delay/slew estimation
      annotateInputSlews(drvr, dcalc_ap);
      findSwapPinCandidate(
          input_port, drvr_port, ports, load_cap, dcalc_ap, &swap_port);
      resetInputSlews();

      if (!sta::LibertyPort::equiv(swap_port, input_port)) {
        debugPrint(logger_,
                   RSZ,
                   "repair_setup",
                   3,
                   "Swap {} ({}) {} {}",
                   network_->name(drvr),
                   cell->name(),
                   input_port->name(),
                   swap_port->name());

        debugPrint(logger_,
                   RSZ,
                   "moves",
                   1,
                   "swap_pins {} ({}) {}<->{}",
                   network_->name(drvr),
                   cell->name(),
                   input_port->name(),
                   swap_port->name());
        swapPins(drvr, input_port, swap_port);
        addMove(drvr);
        return true;
      }
    }
  }
  return false;
}

void SwapPinsMove::swapPins(Instance* inst,
                            LibertyPort* port1,
                            LibertyPort* port2)
{
  Pin *found_pin1, *found_pin2;
  Net *net1, *net2;

  odb::dbModNet* mod_net_pin1 = nullptr;
  odb::dbNet* flat_net_pin1 = nullptr;

  odb::dbModNet* mod_net_pin2 = nullptr;
  odb::dbNet* flat_net_pin2 = nullptr;

  InstancePinIterator* pin_iter = network_->pinIterator(inst);
  found_pin1 = found_pin2 = nullptr;
  net1 = net2 = nullptr;
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    Net* net = network_->net(pin);
    LibertyPort* port = network_->libertyPort(pin);

    // port pointers may change after sizing
    // if (port == port1) {
    if (std::strcmp(port->name(), port1->name()) == 0) {
      found_pin1 = pin;
      net1 = net;
      flat_net_pin1 = db_network_->flatNet(found_pin1);
      mod_net_pin1 = db_network_->hierNet(found_pin1);
    }
    if (std::strcmp(port->name(), port2->name()) == 0) {
      found_pin2 = pin;
      net2 = net;
      flat_net_pin2 = db_network_->flatNet(found_pin2);
      mod_net_pin2 = db_network_->hierNet(found_pin2);
    }
  }

  if (net1 != nullptr && net2 != nullptr) {
    // Swap the ports and nets
    // Support for hierarchy, swap modnets as well as dbnets

    // Simultaneously connect both flat and hier net so
    // they are reassociated.

    // disconnect everything connected to found_pin1
    sta_->disconnectPin(found_pin1);
    // new api call which keeps association
    db_network_->connectPin(
        found_pin1, (Net*) flat_net_pin2, (Net*) mod_net_pin2);

    sta_->disconnectPin(found_pin2);
    db_network_->connectPin(
        found_pin2, (Net*) flat_net_pin1, (Net*) mod_net_pin1);

    // Invalidate the parasitics on these two nets.
    if (resizer_->haveEstimatedParasitics()) {
      resizer_->invalidateParasitics(
          found_pin2,
          db_network_->dbToSta(flat_net_pin1));  // net1);
      resizer_->invalidateParasitics(
          found_pin1,
          db_network_->dbToSta(flat_net_pin2));  // net2);
    }
  }
}

// Lets just look at the first list for now.
// We may want to cache this information somwhere (by building it up for the
// whole library). Or just generate it when the cell is being created
// (depending on agreement).
void SwapPinsMove::equivCellPins(const LibertyCell* cell,
                                 LibertyPort* input_port,
                                 sta::LibertyPortSet& ports)
{
  if (cell->hasSequentials() || cell->isIsolationCell()) {
    ports.clear();
    return;
  }
  sta::LibertyCellPortIterator port_iter(cell);
  int outputs = 0;
  int inputs = 0;

  // count number of output ports.
  while (port_iter.hasNext()) {
    LibertyPort* port = port_iter.next();
    if (port->direction()->isOutput()) {
      ++outputs;
    } else {
      ++inputs;
    }
  }

  if (outputs >= 1 && inputs >= 2) {
    sta::LibertyCellPortIterator port_iter2(cell);
    while (port_iter2.hasNext()) {
      LibertyPort* candidate_port = port_iter2.next();
      if (!candidate_port->direction()->isInput()) {
        continue;
      }

      sta::LibertyCellPortIterator output_port_iter(cell);
      std::optional<bool> is_equivalent;
      // Loop through all the output ports and make sure they are equivalent
      // under swaps of candidate_port and input_port. For multi-ouput gates
      // like full adders.
      while (output_port_iter.hasNext()) {
        LibertyPort* output_candidate_port = output_port_iter.next();
        sta::FuncExpr* output_expr = output_candidate_port->function();
        if (!output_candidate_port->direction()->isOutput()) {
          continue;
        }

        if (output_expr == nullptr) {
          continue;
        }

        if (input_port == candidate_port) {
          continue;
        }

        bool is_equivalent_result
            = isPortEqiv(output_expr, cell, input_port, candidate_port);

        if (!is_equivalent.has_value()) {
          is_equivalent = is_equivalent_result;
          continue;
        }

        is_equivalent = is_equivalent.value() && is_equivalent_result;
      }

      // candidate_port is equivalent to input_port under all output ports
      // of this cell.
      if (is_equivalent.has_value() && is_equivalent.value()) {
        ports.insert(candidate_port);
      }
    }
  }
}

void SwapPinsMove::reportSwappablePins()
{
  std::unique_ptr<sta::LibertyLibraryIterator> iter(
      db_network_->libertyLibraryIterator());
  while (iter->hasNext()) {
    sta::LibertyLibrary* library = iter->next();
    sta::LibertyCellIterator cell_iter(library);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        LibertyPort* port = port_iter.next();
        if (!port->direction()->isInput()) {
          continue;
        }
        sta::LibertyPortSet ports;
        equivCellPins(cell, port, ports);
        std::ostringstream ostr;
        for (auto port : ports) {
          ostr << ' ' << port->name();
        }
        logger_->report("{}/{} ->{}", cell->name(), port->name(), ostr.str());
      }
    }
  }
}

// Create a map of all the pins that are equivalent and then use the fastest pin
// for our violating path. Current implementation does not handle the case
// where 2 paths go through the same gate (we could end up swapping pins twice)
void SwapPinsMove::findSwapPinCandidate(LibertyPort* input_port,
                                        LibertyPort* drvr_port,
                                        const sta::LibertyPortSet& equiv_ports,
                                        float load_cap,
                                        const DcalcAnalysisPt* dcalc_ap,
                                        LibertyPort** swap_port)
{
  LibertyCell* cell = drvr_port->libertyCell();
  std::map<LibertyPort*, ArcDelay> port_delays;
  ArcDelay base_delay = -INF;

  // Create map of pins and delays except the input pin.
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (TimingArc* arc : arc_set->arcs()) {
        const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        LibertyPort* port = arc->from();
        float in_slew = 0.0;
        auto it = input_slew_map_.find(port);
        if (it != input_slew_map_.end()) {
          const InputSlews& slew = it->second;
          in_slew = slew[in_rf->index()];
        } else {
          in_slew = tgt_slews_[in_rf->index()];
        }
        LoadPinIndexMap load_pin_index_map(network_);
        ArcDcalcResult dcalc_result
            = arc_delay_calc_->gateDelay(nullptr,
                                         arc,
                                         in_slew,
                                         load_cap,
                                         nullptr,
                                         load_pin_index_map,
                                         dcalc_ap);

        const ArcDelay& gate_delay = dcalc_result.gateDelay();

        if (port == input_port) {
          base_delay = std::max(base_delay, gate_delay);
        } else {
          if (port_delays.find(port) == port_delays.end()) {
            port_delays.insert(std::make_pair(port, gate_delay));
          } else {
            port_delays[input_port] = std::max(port_delays[port], gate_delay);
          }
        }
      }
    }
  }

  for (LibertyPort* port : equiv_ports) {
    if (port_delays.find(port) == port_delays.end()) {
      // It's possible than an equivalent pin doesn't have
      // a path to the driver.
      continue;
    }

    if (port->direction()->isInput()
        && !sta::LibertyPort::equiv(input_port, port)
        && !sta::LibertyPort::equiv(drvr_port, port)
        && port_delays[port] < base_delay) {
      *swap_port = port;
      base_delay = port_delays[port];
    }
  }
}

void SwapPinsMove::annotateInputSlews(Instance* inst,
                                      const DcalcAnalysisPt* dcalc_ap)
{
  input_slew_map_.clear();
  std::unique_ptr<InstancePinIterator> inst_pin_iter{
      network_->pinIterator(inst)};
  while (inst_pin_iter->hasNext()) {
    Pin* pin = inst_pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      LibertyPort* port = network_->libertyPort(pin);
      if (port) {
        Vertex* vertex = graph_->pinDrvrVertex(pin);
        InputSlews slews;
        slews[RiseFall::rise()->index()]
            = sta_->vertexSlew(vertex, RiseFall::rise(), dcalc_ap);
        slews[RiseFall::fall()->index()]
            = sta_->vertexSlew(vertex, RiseFall::fall(), dcalc_ap);
        input_slew_map_.emplace(port, slews);
      }
    }
  }
}

void SwapPinsMove::resetInputSlews()
{
  input_slew_map_.clear();
}

}  // namespace rsz
