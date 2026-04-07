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
#include <unordered_set>
#include <utility>

#include "BaseMove.hh"
#include "odb/db.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;

using utl::RSZ;

bool SwapPinsMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Instance* drvr = network_->instance(drvr_pin);

  // Skip if this is don't touch
  if (resizer_->dontTouch(drvr)) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  // Check if we have already dealt with this instance
  // and prevent any further swaps.
  if (hasMoves(drvr)) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: Already swapped {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  // Skip if there is no liberty model or this is a single-input cell
  sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  sta::LibertyCell* drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;

  if (!drvr_cell) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: No liberty cell found for {}",
               network_->pathName(drvr_pin),
               network_->pathName(drvr));
    return false;
  }

  if (drvr_cell->isBuffer() || drvr_cell->isInverter()) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: Cell {} is single output",
               network_->pathName(drvr_pin),
               drvr_cell->name());
    return false;
  }

  sta::Scene* scene = drvr_path->scene(sta_);
  const sta::MinMax* min_max = drvr_path->minMax(sta_);
  const float load_cap = graph_delay_calc_->loadCap(drvr_pin, scene, min_max);
  sta::Pin* drvr_input_pin = drvr_path->prevPath()->pin(sta_);

  // We get the driver port and the cell for that port.
  sta::LibertyPort* input_port = network_->libertyPort(drvr_input_pin);
  sta::LibertyPort* swap_port = input_port;
  LibertyPortVec ports;

  // Skip output to output paths
  if (input_port->direction()->isOutput()) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: Output to output path",
               network_->pathName(drvr_pin));
    return false;
  }

  // Find the equivalent pins for a cell (simple implementation for now)
  // stash them. Ports are unique to a cell so we can just cache by port
  // and that should apply to all instances of that cell with this input_port.
  if (!equiv_pin_map_.contains(input_port)) {
    equivCellPins(drvr_cell, input_port, ports);
    equiv_pin_map_.insert({input_port, ports});
  }
  ports = equiv_pin_map_[input_port];
  if (ports.empty()) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: No equivalent pins found",
               network_->pathName(drvr_pin));
    return false;
  }

  // Pass slews at input pins for more accurate delay/slew estimation
  annotateInputSlews(drvr, scene, min_max);
  findSwapPinCandidate(
      input_port, drvr_port, ports, load_cap, scene, min_max, &swap_port);
  resetInputSlews();

  if (sta::LibertyPort::equiv(swap_port, input_port)) {
    debugPrint(logger_,
               RSZ,
               "swap_pins_move",
               2,
               "REJECT SwapPinsMove {}: Selected swap pin is actually the same "
               "input pin",
               network_->pathName(drvr_pin));
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "swap_pins_move",
             1,
             "ACCEPT SwapPinsMove {}: Cell {}, pins {} <-> {}",
             network_->name(drvr_pin),
             drvr_cell->name(),
             input_port->name(),
             swap_port->name());
  swapPins(drvr, input_port, swap_port);
  countMove(drvr);
  return true;
}

void SwapPinsMove::swapPins(sta::Instance* inst,
                            sta::LibertyPort* port1,
                            sta::LibertyPort* port2)
{
  sta::Pin *found_pin1, *found_pin2;
  sta::Net *net1, *net2;

  odb::dbModNet* mod_net_pin1 = nullptr;
  odb::dbNet* flat_net_pin1 = nullptr;

  odb::dbModNet* mod_net_pin2 = nullptr;
  odb::dbNet* flat_net_pin2 = nullptr;

  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  found_pin1 = found_pin2 = nullptr;
  net1 = net2 = nullptr;
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
    sta::Net* net = network_->net(pin);
    sta::LibertyPort* port = network_->libertyPort(pin);

    // port pointers may change after sizing
    // if (port == port1) {
    if (port->name() == port1->name()) {
      found_pin1 = pin;
      net1 = net;
      flat_net_pin1 = db_network_->flatNet(found_pin1);
      mod_net_pin1 = db_network_->hierNet(found_pin1);
    }
    if (port->name() == port2->name()) {
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
        found_pin1, (sta::Net*) flat_net_pin2, (sta::Net*) mod_net_pin2);

    sta_->disconnectPin(found_pin2);
    db_network_->connectPin(
        found_pin2, (sta::Net*) flat_net_pin1, (sta::Net*) mod_net_pin1);
  }
}

// Lets just look at the first list for now.
// We may want to cache this information somwhere (by building it up for the
// whole library). Or just generate it when the cell is being created
// (depending on agreement).
void SwapPinsMove::equivCellPins(const sta::LibertyCell* cell,
                                 sta::LibertyPort* input_port,
                                 LibertyPortVec& ports)
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
    sta::LibertyPort* port = port_iter.next();
    sta::PortDirection* direction = port->direction();
    if (direction->isOutput()) {
      ++outputs;
    } else if (direction->isInput()) {
      ++inputs;
    } else if (port->isPwrGnd()) {
      // skip
    } else {
      ports.clear();
      return;  // reject tristate/internal/bidirect/unknown cases
    }
  }

  if (outputs < 1 || inputs < 2) {
    return;
  }

  sta::LibertyCellPortIterator port_iter2(cell);
  std::unordered_set<sta::LibertyPort*> seen_ports;
  while (port_iter2.hasNext()) {
    sta::LibertyPort* candidate_port = port_iter2.next();
    if (!candidate_port->direction()->isInput()) {
      continue;
    }

    sta::LibertyCellPortIterator output_port_iter(cell);
    std::optional<bool> is_equivalent;
    // Loop through all the output ports and make sure they are equivalent
    // under swaps of candidate_port and input_port. For multi-ouput gates
    // like full adders.
    while (output_port_iter.hasNext()) {
      sta::LibertyPort* output_candidate_port = output_port_iter.next();
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
    if (is_equivalent.has_value() && is_equivalent.value()
        && !seen_ports.contains(candidate_port)) {
      seen_ports.insert(candidate_port);
      ports.push_back(candidate_port);
    }
  }
  if (!seen_ports.empty()) {  // If we added any ports sort them.
    std::ranges::sort(ports, {}, [](auto* p1) { return p1->id(); });
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
        sta::LibertyPort* port = port_iter.next();
        if (!port->direction()->isInput()) {
          continue;
        }
        LibertyPortVec ports;
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
void SwapPinsMove::findSwapPinCandidate(sta::LibertyPort* input_port,
                                        sta::LibertyPort* drvr_port,
                                        const LibertyPortVec& equiv_ports,
                                        float load_cap,
                                        const sta::Scene* scene,
                                        const sta::MinMax* min_max,
                                        sta::LibertyPort** swap_port)
{
  sta::LibertyCell* cell = drvr_port->libertyCell();
  std::map<sta::LibertyPort*, sta::ArcDelay> port_delays;
  sta::ArcDelay base_delay = -sta::INF;

  // Create map of pins and delays except the input pin.
  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        sta::LibertyPort* port = arc->from();
        float in_slew = 0.0;
        auto it = input_slew_map_.find(port);
        if (it != input_slew_map_.end()) {
          const InputSlews& slew = it->second;
          in_slew = slew[in_rf->index()];
        } else {
          in_slew = tgt_slews_[in_rf->index()];
        }
        sta::LoadPinIndexMap load_pin_index_map(network_);
        sta::ArcDcalcResult dcalc_result
            = arc_delay_calc_->gateDelay(nullptr,
                                         arc,
                                         in_slew,
                                         load_cap,
                                         nullptr,
                                         load_pin_index_map,
                                         scene,
                                         min_max);

        const sta::ArcDelay& gate_delay = dcalc_result.gateDelay();

        if (port == input_port) {
          base_delay = std::max(base_delay, gate_delay);
        } else {
          if (!port_delays.contains(port)) {
            port_delays.insert(std::make_pair(port, gate_delay));
          } else {
            port_delays[input_port] = std::max(port_delays[port], gate_delay);
          }
        }
      }
    }
  }

  for (sta::LibertyPort* port : equiv_ports) {
    // Guard Clause:
    // 1. Check if port delay exists
    // 2. Check if port is NOT an input
    // 3. Check if port is equivalent to input_port
    // 4. Check if port is equivalent to drvr_port
    if (!port_delays.contains(port) || !port->direction()->isInput()
        || sta::LibertyPort::equiv(input_port, port)
        || sta::LibertyPort::equiv(drvr_port, port)) {
      continue;
    }

    auto port_delay = port_delays[port];
    if (port_delay < base_delay) {
      *swap_port = port;
      base_delay = port_delay;
    }
  }
}

void SwapPinsMove::annotateInputSlews(sta::Instance* inst,
                                      const sta::Scene* scene,
                                      const sta::MinMax* min_max)
{
  input_slew_map_.clear();
  std::unique_ptr<sta::InstancePinIterator> inst_pin_iter{
      network_->pinIterator(inst)};
  while (inst_pin_iter->hasNext()) {
    sta::Pin* pin = inst_pin_iter->next();
    if (network_->direction(pin)->isInput()) {
      sta::LibertyPort* port = network_->libertyPort(pin);
      if (port) {
        sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
        InputSlews slews;
        auto ap_index = scene->dcalcAnalysisPtIndex(min_max);
        sta_->findDelays(vertex);
        slews[sta::RiseFall::rise()->index()]
            = sta_->graph()->slew(vertex, sta::RiseFall::rise(), ap_index);
        slews[sta::RiseFall::fall()->index()]
            = sta_->graph()->slew(vertex, sta::RiseFall::fall(), ap_index);
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
