// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "RepairDesign.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BufferedNet.hh"
#include "ResizerObserver.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/RiseFallValues.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/Units.hh"
#include "utl/scope.h"

namespace rsz {

using std::abs;
using std::max;
using std::min;

using utl::RSZ;

using sta::Clock;
using sta::INF;
using sta::InstancePinIterator;
using sta::NetConnectedPinIterator;
using sta::NetIterator;
using sta::NetPinIterator;
using sta::Port;
using sta::TimingArc;
using sta::TimingArcSet;
using sta::TimingRole;

RepairDesign::RepairDesign(Resizer* resizer) : resizer_(resizer)
{
}

RepairDesign::~RepairDesign() = default;

void RepairDesign::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  dbu_ = resizer_->dbu_;
  pre_checks_ = new PreChecks(resizer_);
  parasitics_src_ = resizer_->getParasiticsSrc();
  initial_design_area_ = resizer_->computeDesignArea();
}

// Repair long wires, max slew, max capacitance, max fanout violations
// The whole enchilada.
// max_wire_length zero for none (meters)
void RepairDesign::repairDesign(double max_wire_length,
                                double slew_margin,
                                double cap_margin,
                                double buffer_gain,
                                bool verbose)
{
  init();
  int repaired_net_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repairDesign(max_wire_length,
               slew_margin,
               cap_margin,
               buffer_gain,
               verbose,
               repaired_net_count,
               slew_violations,
               cap_violations,
               fanout_violations,
               length_violations);

  reportViolationCounters(false,
                          slew_violations,
                          cap_violations,
                          fanout_violations,
                          length_violations,
                          repaired_net_count);
}

void RepairDesign::performEarlySizingRound(int& repaired_net_count)
{
  // keep track of user annotations so we don't remove them
  std::set<std::pair<Vertex*, int>> slew_user_annotated;

  // We need to override slews in order to get good required time estimates.
  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex* drvr = resizer_->level_drvr_vertices_[i];
    for (auto rf : {RiseFall::rise(), RiseFall::fall()}) {
      if (!drvr->slewAnnotated(rf, min_) && !drvr->slewAnnotated(rf, max_)) {
        sta_->setAnnotatedSlew(drvr,
                               resizer_->tgt_slew_corner_,
                               sta::MinMaxAll::all(),
                               rf->asRiseFallBoth(),
                               resizer_->tgt_slews_[rf->index()]);
      } else {
        slew_user_annotated.insert(std::make_pair(drvr, rf->index()));
      }
    }
  }
  findBufferSizes();

  sta_->searchPreamble();
  search_->findAllArrivals();

  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex* drvr = resizer_->level_drvr_vertices_[i];
    Pin* drvr_pin = drvr->pin();
    // Always get the flat net for the top level port.
    Net* net = network_->isTopLevelPort(drvr_pin)
                   ? network_->net(network_->term(drvr_pin))
                   : db_network_->dbToSta(db_network_->flatNet(drvr_pin));
    if (!net) {
      continue;
    }
    dbNet* net_db = db_network_->staToDb(net);
    search_->findRequireds(drvr->level() + 1);

    if (net && !resizer_->dontTouch(net) && !net_db->isConnectedByAbutment()
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells and supply nets.
        && !drvr->isConstant()) {
      float fanout, max_fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, max_, fanout, max_fanout, fanout_slack);

      bool repaired_net = false;

      if (performGainBuffering(net, drvr_pin, max_fanout)) {
        repaired_net = true;
      }

      if (resizer_->resizeToCapRatio(drvr_pin, false)) {
        repaired_net = true;
      }

      if (repaired_net) {
        repaired_net_count++;
      }
    }

    for (auto mm : sta::MinMaxAll::all()->range()) {
      for (auto rf : sta::RiseFallBoth::riseFall()->range()) {
        if (!slew_user_annotated.count(std::make_pair(drvr, rf->index()))) {
          const DcalcAnalysisPt* dcalc_ap
              = resizer_->tgt_slew_corner_->findDcalcAnalysisPt(mm);
          drvr->setSlewAnnotated(false, rf, dcalc_ap->index());
        }
      }
    }
  }

  resizer_->level_drvr_vertices_valid_ = false;
  resizer_->ensureLevelDrvrVertices();
}

void RepairDesign::repairDesign(
    double max_wire_length,  // zero for none (meters)
    double slew_margin,
    double cap_margin,
    bool initial_sizing,
    bool verbose,
    int& repaired_net_count,
    int& slew_violations,
    int& cap_violations,
    int& fanout_violations,
    int& length_violations)
{
  init();
  slew_margin_ = slew_margin;
  cap_margin_ = cap_margin;

  slew_violations = 0;
  cap_violations = 0;
  fanout_violations = 0;
  length_violations = 0;
  repaired_net_count = 0;
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  resizer_->resized_multi_output_insts_.clear();

  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();
  sta_->searchPreamble();
  search_->findAllArrivals();

  if (initial_sizing) {
    performEarlySizingRound(repaired_net_count);
  }

  resizer_->incrementalParasiticsBegin();
  int print_iteration = 0;
  if (resizer_->level_drvr_vertices_.size() > size_t(5) * max_print_interval_) {
    print_interval_ = max_print_interval_;
  } else {
    print_interval_ = min_print_interval_;
  }
  printProgress(print_iteration, false, false, repaired_net_count);
  int max_length = resizer_->metersToDbu(max_wire_length);
  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    print_iteration++;
    if (verbose || (print_iteration == 1)) {
      printProgress(print_iteration, false, false, repaired_net_count);
    }
    Vertex* drvr = resizer_->level_drvr_vertices_[i];
    Pin* drvr_pin = drvr->pin();
    // hier fix
    // clang-format off
    Net* net = network_->isTopLevelPort(drvr_pin)
                   ? db_network_->dbToSta(
                       db_network_->flatNet(network_->term(drvr_pin)))
                   : db_network_->dbToSta(db_network_->flatNet(drvr_pin));
    // clang-format on
    if (!net) {
      continue;
    }
    dbNet* net_db = db_network_->staToDb(net);
    bool debug = (drvr_pin == resizer_->debug_pin_);
    if (debug) {
      logger_->setDebugLevel(RSZ, "repair_net", 3);
    }
    if (net && !resizer_->dontTouch(net) && !net_db->isConnectedByAbutment()
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells and supply nets.
        && !drvr->isConstant()) {
      repairNet(net,
                drvr_pin,
                drvr,
                true,
                true,
                true,
                max_length,
                true,
                repaired_net_count,
                slew_violations,
                cap_violations,
                fanout_violations,
                length_violations);
    }
    if (debug) {
      logger_->setDebugLevel(RSZ, "repair_net", 0);
    }
  }
  resizer_->updateParasitics();
  printProgress(print_iteration, true, true, repaired_net_count);
  resizer_->incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    resizer_->level_drvr_vertices_valid_ = false;
  }
}

// Repair long wires from clock input pins to clock tree root buffer
// because CTS ignores the issue.
// no max_fanout/max_cap checks.
// Use max_wire_length zero for none (meters)
void RepairDesign::repairClkNets(double max_wire_length)
{
  init();

  // Lift sizing restrictions for clock buffers.
  // Save old values in area_limit and leakage_limit.
  utl::SetAndRestore<std::optional<double>> area_limit(
      resizer_->sizing_area_limit_, std::nullopt);
  utl::SetAndRestore<std::optional<double>> leakage_limit(
      resizer_->sizing_leakage_limit_, std::nullopt);

  slew_margin_ = 0.0;
  cap_margin_ = 0.0;

  // Need slews to resize inserted buffers.
  sta_->findDelays();

  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int repaired_net_count = 0;
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  resizer_->resized_multi_output_insts_.clear();

  resizer_->incrementalParasiticsBegin();
  int max_length = resizer_->metersToDbu(max_wire_length);
  for (Clock* clk : sdc_->clks()) {
    const PinSet* clk_pins = sta_->pins(clk);
    if (clk_pins) {
      for (const Pin* clk_pin : *clk_pins) {
        // clang-format off
        Net* net = network_->isTopLevelPort(clk_pin)
                       ? db_network_->dbToSta(
                           db_network_->flatNet(network_->term(clk_pin)))
                       : db_network_->dbToSta(db_network_->flatNet(clk_pin));
        // clang-format on
        if (net && network_->isDriver(clk_pin)) {
          Vertex* drvr = graph_->pinDrvrVertex(clk_pin);
          // Do not resize clock tree gates.
          repairNet(net,
                    clk_pin,
                    drvr,
                    false,
                    false,
                    false,
                    max_length,
                    false,
                    repaired_net_count,
                    slew_violations,
                    cap_violations,
                    fanout_violations,
                    length_violations);
        }
      }
    }
  }
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  if (length_violations > 0) {
    logger_->info(RSZ, 47, "Found {} long wires.", length_violations);
  }
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ,
                  48,
                  "Inserted {} buffers in {} nets.",
                  inserted_buffer_count_,
                  repaired_net_count);
    resizer_->level_drvr_vertices_valid_ = false;
  }

  // Restore previous sizing restrictions when area_limit and leakage_limit go
  // out of scope.  This restore works even in the presence of exceptions.
}

// Repair one net (for debugging)
void RepairDesign::repairNet(Net* net,
                             double max_wire_length,  // meters
                             double slew_margin,
                             double cap_margin)
{
  init();
  slew_margin_ = slew_margin;
  cap_margin_ = cap_margin;

  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int repaired_net_count = 0;
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  resizer_->resized_multi_output_insts_.clear();
  resizer_->buffer_moved_into_core_ = false;

  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  resizer_->incrementalParasiticsBegin();
  int max_length = resizer_->metersToDbu(max_wire_length);
  PinSet* drivers = network_->drivers(net);
  if (drivers && !drivers->empty()) {
    PinSet::Iterator drvr_iter(drivers);
    const Pin* drvr_pin = drvr_iter.next();
    Vertex* drvr = graph_->pinDrvrVertex(drvr_pin);
    repairNet(net,
              drvr_pin,
              drvr,
              true,
              true,
              true,
              max_length,
              true,
              repaired_net_count,
              slew_violations,
              cap_violations,
              fanout_violations,
              length_violations);
  }
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();

  reportViolationCounters(true,
                          slew_violations,
                          cap_violations,
                          fanout_violations,
                          length_violations,
                          repaired_net_count);
}

bool RepairDesign::getLargestSizeCin(const Pin* drvr_pin, float& cin)
{
  Instance* inst = network_->instance(drvr_pin);
  LibertyCell* cell = network_->libertyCell(inst);
  cin = 0;
  if (!network_->isTopLevelPort(drvr_pin) && cell != nullptr
      && resizer_->isLogicStdCell(inst)) {
    for (auto size : resizer_->getSwappableCells(cell)) {
      float size_cin = 0;
      sta::LibertyCellPortIterator port_iter(size);
      int nports = 0;
      while (port_iter.hasNext()) {
        const LibertyPort* port = port_iter.next();
        if (port->direction() == sta::PortDirection::input()) {
          size_cin += port->capacitance();
          nports++;
        }
      }
      if (!nports) {
        return false;
      }
      size_cin /= nports;
      if (size_cin > cin) {
        cin = size_cin;
      }
    }
    return true;
  }
  return false;
}

bool RepairDesign::getCin(const Pin* drvr_pin, float& cin)
{
  Instance* inst = network_->instance(drvr_pin);
  LibertyCell* cell = network_->libertyCell(inst);
  cin = 0;
  if (!network_->isTopLevelPort(drvr_pin) && cell != nullptr
      && resizer_->isLogicStdCell(inst)) {
    sta::LibertyCellPortIterator port_iter(cell);
    int nports = 0;
    while (port_iter.hasNext()) {
      const LibertyPort* port = port_iter.next();
      if (port->direction() == sta::PortDirection::input()) {
        cin += port->capacitance();
        nports++;
      }
    }
    if (!nports) {
      return false;
    }
    cin /= nports;
    return true;
  }
  return false;
}

static float bufferCin(const LibertyCell* cell)
{
  LibertyPort *a, *y;
  cell->bufferPorts(a, y);
  return a->capacitance();
}

void RepairDesign::findBufferSizes()
{
  resizer_->findFastBuffers();
  buffer_sizes_.clear();
  buffer_sizes_ = {resizer_->buffer_fast_sizes_.begin(),
                   resizer_->buffer_fast_sizes_.end()};
  std::sort(buffer_sizes_.begin(),
            buffer_sizes_.end(),
            [=](LibertyCell* a, LibertyCell* b) {
              return bufferCin(a) < bufferCin(b);
            });
}

bool RepairDesign::performGainBuffering(Net* net,
                                        const Pin* drvr_pin,
                                        int max_fanout)
{
  struct EnqueuedPin
  {
    Pin* pin;
    Path* required_path;
    Delay required_delay;
    int level;

    Required required(const StaState*) const
    {
      if (required_path == nullptr) {
        return INF;
      }
      return required_path->required() - required_delay;
    }

    std::pair<Required, int> sort_label(const StaState* sta) const
    {
      return std::make_pair(required(sta), -level);
    }

    float capacitance(const Network* network)
    {
      return network->libertyPort(pin)->capacitance();
    }
  };

  class PinRequiredHigher
  {
   private:
    const Network* network_;

   public:
    PinRequiredHigher(const Network* network) : network_(network) {}

    bool operator()(const EnqueuedPin& a, const EnqueuedPin& b) const
    {
      auto la = a.sort_label(network_), lb = b.sort_label(network_);
      if (la > lb) {
        return true;
      }
      if (la < lb) {
        return false;
      }
      return sta::stringLess(network_->pathName(a.pin),
                             network_->pathName(b.pin));
    }
  };

  // Collect all sinks
  std::vector<EnqueuedPin> sinks;

  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin != drvr_pin && !network_->isTopLevelPort(pin)
        && network_->direction(pin) == sta::PortDirection::input()
        && network_->libertyPort(pin)) {
      Instance* inst = network_->instance(pin);
      if (!resizer_->dontTouch(inst)) {
        Vertex* vertex = graph_->pinLoadVertex(pin);
        Path* req_path = sta_->vertexWorstSlackPath(vertex, sta::MinMax::max());
        sinks.push_back({const_cast<Pin*>(pin), req_path, 0.0, 0});
      } else {
        logger_->warn(RSZ,
                      143,
                      "dont-touch instance {} ignored when buffering net {}",
                      network_->name(inst),
                      network_->name(net));
      }
    }
  }
  delete pin_iter;

  // Keep track of the vertices at the boundary of the tree so we know where
  // to ask for delays to be recomputed
  std::vector<Vertex*> tree_boundary;

  const float max_buf_load
      = bufferCin(buffer_sizes_.back()) * resizer_->buffer_sizing_cap_ratio_;

  float cin;
  float has_driver_cin = getLargestSizeCin(drvr_pin, cin);
  static float gate_gain = 4.0f;  // use a fanout-of-4 rule for gates
  bool repaired_net = false;

  float load = 0.0;
  for (auto& sink : sinks) {
    load += sink.capacitance(network_);
  }
  std::sort(sinks.begin(), sinks.end(), PinRequiredHigher(network_));

  // Iterate until we satisfy both the gain condition and max_fanout
  // on drvr_pin
  while (sinks.size() > max_fanout
         || (has_driver_cin && load > cin * gate_gain)) {
    float load_acc = 0;
    auto it = sinks.begin();
    for (; it != sinks.end(); it++) {
      if (it - sinks.begin() == max_fanout) {
        break;
      }
      float sink_load = it->capacitance(network_);
      if (load_acc + sink_load > max_buf_load
          // always include at least one load
          && it != sinks.begin()) {
        break;
      }
      load_acc += sink_load;
    }
    auto group_end = it;

    // Find the smallest buffer satisfying the gain condition on
    // its output pin
    auto size = buffer_sizes_.begin();
    for (; size != buffer_sizes_.end() - 1; size++) {
      if (bufferCin(*size) > load_acc / resizer_->buffer_sizing_cap_ratio_) {
        break;
      }
    }

    if (bufferCin(*size) >= 0.9f * load_acc) {
      // We are getting dimishing returns on inserting a buffer, stop
      // the algorithm here (we might have been called with a low gain value)
      break;
    }

    // Get scope of driver, put any new buffers in that scope
    sta::Pin* driver_pin = nullptr;
    odb::dbModule* driver_parent = db_network_->getNetDriverParentModule(
        net, driver_pin, db_network_->hasHierarchy());
    odb::dbModInst* parent_mod_inst = driver_parent->getModInst();
    Instance* parent;
    if (parent_mod_inst) {
      parent = db_network_->dbToSta(parent_mod_inst);
    } else {
      parent = db_network_->topInstance();
    }

    // note any hierarchical nets.
    // and move them to the output of the buffer.
    odb::dbModNet* driver_mod_net = db_network_->hierNet(driver_pin);
    if (driver_mod_net) {
      // only disconnect the modnet, we hook it to the output of the buffer.
      db_network_->disconnectPin(driver_pin,
                                 db_network_->dbToSta(driver_mod_net));
    }

    // make sure any nets created are scoped within hierarchy
    // backwards compatible. new naming only used for hierarchy code.

    std::string net_name = db_network_->hasHierarchy()
                               ? resizer_->makeUniqueNetName(parent)
                               : resizer_->makeUniqueNetName();
    Net* new_net = db_network_->makeNet(net_name.c_str(), parent);

    dbNet* net_db = db_network_->staToDb(net);
    dbNet* new_net_db = db_network_->staToDb(new_net);
    new_net_db->setSigType(net_db->getSigType());

    std::string buffer_name = resizer_->makeUniqueInstName("gain");
    const Point drvr_loc = db_network_->location(drvr_pin);

    // create instance in driver parent
    Instance* inst
        = resizer_->makeBuffer(*size, buffer_name.c_str(), parent, drvr_loc);

    LibertyPort *size_in, *size_out;
    (*size)->bufferPorts(size_in, size_out);
    Pin* buffer_ip_pin = nullptr;
    Pin* buffer_op_pin = nullptr;
    resizer_->getBufferPins(inst, buffer_ip_pin, buffer_op_pin);
    db_network_->connectPin(buffer_ip_pin, net);

    // connect the buffer output to the new flat net and any modnet
    // Keep the original input net driving the buffer.
    // Update the hierarchical net/flat net correspondence because
    // the hierarhical net is moved to the output of the buffer.

    db_network_->connectPin(
        buffer_op_pin, new_net, db_network_->dbToSta(driver_mod_net));

    repaired_net = true;
    inserted_buffer_count_++;

    int max_level = 0;
    for (auto it = sinks.begin(); it != group_end; it++) {
      Pin* sink_pin = it->pin;
      LibertyPort* sink_port = network_->libertyPort(it->pin);
      Instance* sink_inst = network_->instance(it->pin);
      load -= sink_port->capacitance();
      if (it->level > max_level) {
        max_level = it->level;
      }

      odb::dbModNet* sink_mod_net = db_network_->hierNet(sink_pin);
      // rewire the sink pin, taking care of both the flat net
      // and the hierarchical net. Update the hierarchical net
      // flat net correspondence
      db_network_->disconnectPin(sink_pin);
      db_network_->connectPin(sink_pin,
                              db_network_->dbToSta(new_net_db),
                              db_network_->dbToSta(sink_mod_net));
      if (it->level == 0) {
        Pin* new_pin = network_->findPin(sink_inst, sink_port);
        tree_boundary.push_back(graph_->pinLoadVertex(new_pin));
      }
    }

    Pin* new_input_pin = buffer_ip_pin;

    Delay buffer_delay
        = resizer_->bufferDelay(*size, load_acc, resizer_->tgt_slew_dcalc_ap_);

    auto new_pin = EnqueuedPin{new_input_pin,
                               (group_end - 1)->required_path,
                               (group_end - 1)->required_delay + buffer_delay,
                               max_level + 1};

    sinks.erase(sinks.begin(), group_end);
    sinks.insert(
        std::upper_bound(
            sinks.begin(), sinks.end(), new_pin, PinRequiredHigher(network_)),
        new_pin);

    load += size_in->capacitance();
  }
  sta_->ensureLevelized();
  sta::Level max_level = 0;
  for (auto vertex : tree_boundary) {
    max_level = std::max(vertex->level(), max_level);
  }
  sta_->findDelays(max_level);
  search_->findArrivals(max_level);

  return repaired_net;
}

void RepairDesign::checkDriverArcSlew(const Corner* corner,
                                      const Instance* inst,
                                      const TimingArc* arc,
                                      float load_cap,
                                      float limit,
                                      float& violation)
{
  const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
  const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
  GateTimingModel* model = dynamic_cast<GateTimingModel*>(arc->model());
  Pin* in_pin = network_->findPin(inst, arc->from()->name());

  if (model && in_pin) {
    Slew in_slew = sta_->graph()->slew(
        graph_->pinLoadVertex(in_pin), in_rf, dcalc_ap->index());
    const Pvt* pvt = dcalc_ap->operatingConditions();

    ArcDelay arc_delay;
    Slew arc_slew;
    model->gateDelay(pvt, in_slew, load_cap, false, arc_delay, arc_slew);

    if (arc_slew > limit) {
      violation = max(arc_slew - limit, violation);
    }
  }
}

// Repair max slew violation at a driver pin: Find the smallest
// size which fits max slew; if none can be found, at least pick
// the size for which the slew is lowest
bool RepairDesign::repairDriverSlew(const Corner* corner, const Pin* drvr_pin)
{
  Instance* inst = network_->instance(drvr_pin);
  LibertyCell* cell = network_->libertyCell(inst);

  float load_cap;
  resizer_->ensureWireParasitic(drvr_pin);
  load_cap
      = graph_delay_calc_->loadCap(drvr_pin, corner->findDcalcAnalysisPt(max_));

  if (!network_->isTopLevelPort(drvr_pin) && !resizer_->dontTouch(inst) && cell
      && resizer_->isLogicStdCell(inst)) {
    LibertyCellSeq equiv_cells = resizer_->getSwappableCells(cell);
    if (!equiv_cells.empty()) {
      // Pair of slew violation magnitude and cell pointer
      typedef std::pair<float, LibertyCell*> SizeCandidate;
      std::vector<SizeCandidate> sizes;

      for (LibertyCell* size_cell : equiv_cells) {
        float limit, violation = 0;
        bool limit_exists = false;
        LibertyPort* port
            = size_cell->findLibertyPort(network_->portName(drvr_pin));
        sta_->findSlewLimit(port, corner, max_, limit, limit_exists);

        if (limit_exists) {
          float limit_w_margin = maxSlewMargined(limit);

          for (TimingArcSet* arc_set : size_cell->timingArcSets()) {
            const TimingRole* role = arc_set->role();
            if (!role->isTimingCheck() && role != TimingRole::tristateDisable()
                && role != TimingRole::tristateEnable()
                && role != TimingRole::clockTreePathMin()
                && role != TimingRole::clockTreePathMax()) {
              for (TimingArc* arc : arc_set->arcs()) {
                if (arc->to() == port) {
                  checkDriverArcSlew(
                      corner, inst, arc, load_cap, limit_w_margin, violation);
                }
              }
            }
          }
        }

        sizes.emplace_back(violation, size_cell);
      }

      if (sizes.empty()) {
        logger_->critical(
            RSZ, 144, "sizes list empty for cell {}\n", cell->name());
      }

      std::sort(
          sizes.begin(), sizes.end(), [](SizeCandidate a, SizeCandidate b) {
            if (a.first == 0 && b.first == 0) {
              // both sizes non-violating: sort by area
              return a.second->area() < b.second->area();
            }
            return a.first < b.first;
          });

      LibertyCell* selected_size = sizes.front().second;
      if (selected_size != cell) {
        return resizer_->replaceCell(inst, selected_size, true);
      }
    }
  }

  return false;
}

void RepairDesign::repairNet(Net* net,
                             const Pin* drvr_pin,
                             Vertex* drvr,
                             bool check_slew,
                             bool check_cap,
                             bool check_fanout,
                             int max_length,  // dbu
                             bool resize_drvr,
                             int& repaired_net_count,
                             int& slew_violations,
                             int& cap_violations,
                             int& fanout_violations,
                             int& length_violations)
{
  // Hands off special nets.
  if (!db_network_->isSpecial(net)) {
    debugPrint(logger_,
               RSZ,
               "repair_net",
               1,
               "repair net {}",
               sdc_network_->pathName(drvr_pin));
    const Corner* corner = sta_->cmdCorner();
    bool repaired_net = false;

    // Fanout is addressed by creating region repeaters
    if (check_fanout) {
      float fanout, max_fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, max_, fanout, max_fanout, fanout_slack);
      if (max_fanout > 0.0 && fanout_slack < 0.0) {
        fanout_violations++;
        repaired_net = true;

        debugPrint(logger_, RSZ, "repair_net", 3, "fanout violation");
        LoadRegion region = findLoadRegions(net, drvr_pin, max_fanout);
        corner_ = corner;
        makeRegionRepeaters(region,
                            max_fanout,
                            1,
                            drvr_pin,
                            check_slew,
                            check_cap,
                            max_length,
                            resize_drvr);
      }
    }

    // TO BE REMOVED: Resize the driver to normalize slews before repairing
    // limit violations.
    if (parasitics_src_ == ParasiticsSrc::placement && resize_drvr) {
      resize_count_ += resizer_->resizeToCapRatio(drvr_pin, false);
    }

    float max_cap = INF;
    bool repair_cap = false, repair_load_slew = false, repair_wire = false;

    resizer_->ensureWireParasitic(drvr_pin, net);
    graph_delay_calc_->findDelays(drvr);

    if (check_slew) {
      bool slew_violation = false;

      // First repair driver slew -- addressed by resizing the driver,
      // and if that doesn't fix it fully, by inserting buffers
      float slew1, max_slew1, slew_slack1;
      const Corner* corner1;
      checkSlew(drvr_pin, slew1, max_slew1, slew_slack1, corner1);
      if (slew_slack1 < 0.0f) {
        debugPrint(logger_,
                   RSZ,
                   "repair_net",
                   2,
                   "drvr slew violation pin={} slew={} max_slew={}",
                   network_->name(drvr_pin),
                   delayAsString(slew1, this, 3),
                   delayAsString(max_slew1, this, 3));

        slew_violation = true;
        if (repairDriverSlew(corner1, drvr_pin)) {
          resize_count_++;
          checkSlew(drvr_pin, slew1, max_slew1, slew_slack1, corner1);
        }

        // Slew violation persists after resizing the driver, derive
        // the max cap we need to apply to remove the slew violation
        if (slew_slack1 < 0.0f) {
          LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
          if (drvr_port) {
            max_cap = findSlewLoadCap(drvr_port, max_slew1, corner1);
            corner = corner1;
            repair_cap = true;
          }
        }
      }

      if (!resizer_->isTristateDriver(drvr_pin)) {
        // Check load slew, if violated it will be repaired by inserting
        // buffers later
        resizer_->checkLoadSlews(
            drvr_pin, slew_margin_, slew1, max_slew1, slew_slack1, corner1);
        if (slew_slack1 < 0.0f) {
          debugPrint(logger_,
                     RSZ,
                     "repair_net",
                     2,
                     "load slew violation pin={} load_slew={} max_slew={}",
                     network_->name(drvr_pin),
                     delayAsString(slew1, this, 3),
                     delayAsString(max_slew1, this, 3));

          slew_violation = true;
          repair_load_slew = true;
          // If repair_cap is true, corner is already set to correspond
          // to a max_cap violation, do not override in that case
          if (!repair_cap) {
            corner = corner1;
          }
        }
      }

      if (slew_violation) {
        slew_violations++;
      }
    }

    if (check_cap && !resizer_->isTristateDriver(drvr_pin)) {
      if (needRepairCap(drvr_pin, cap_violations, max_cap, corner)) {
        repair_cap = true;
      }
    }

    // For tristate nets all we can do is resize the driver.
    if (!resizer_->isTristateDriver(drvr_pin)) {
      BufferedNetPtr bnet = resizer_->makeBufferedNetSteiner(drvr_pin, corner);
      if (bnet) {
        int wire_length = bnet->maxLoadWireLength();
        repair_wire
            = needRepairWire(max_length, wire_length, length_violations);

        // Insert buffers on the Steiner tree if need be
        if (repair_cap || repair_load_slew || repair_wire) {
          repaired_net = true;
          repairNet(bnet, drvr_pin, max_cap, max_length, corner);
        }
      }
    }

    if (repaired_net) {
      repaired_net_count++;
    }
  }
}

bool RepairDesign::needRepairCap(const Pin* drvr_pin,
                                 int& cap_violations,
                                 float& max_cap,
                                 const Corner*& corner)
{
  float cap1, max_cap1, cap_slack1;
  const Corner* corner1;
  const RiseFall* tr1;
  sta_->checkCapacitance(
      drvr_pin, nullptr, max_, corner1, tr1, cap1, max_cap1, cap_slack1);
  if (max_cap1 > 0.0 && corner1) {
    max_cap1 *= (1.0 - cap_margin_ / 100.0);

    if (cap1 > max_cap1) {
      max_cap = max_cap1;
      corner = corner1;
      cap_violations++;
      return true;
    }
  }

  return false;
}

bool RepairDesign::needRepairWire(const int max_length,
                                  const int wire_length,
                                  int& length_violations)
{
  if (max_length && wire_length > max_length) {
    length_violations++;
    return true;
  }
  return false;
}

void RepairDesign::checkSlew(const Pin* drvr_pin,
                             // Return values.
                             Slew& slew,
                             float& limit,
                             float& slack,
                             const Corner*& corner)
{
  slack = INF;
  limit = INF;
  corner = nullptr;

  const Corner* corner1;
  const RiseFall* tr1;
  Slew slew1;
  float limit1, slack1;
  sta_->checkSlew(
      drvr_pin, nullptr, max_, false, corner1, tr1, slew1, limit1, slack1);
  if (corner1) {
    limit1 *= (1.0 - slew_margin_ / 100.0);
    slack1 = limit1 - slew1;
    if (slack1 < slack) {
      slew = slew1;
      limit = limit1;
      slack = slack1;
      corner = corner1;
    }
  }
}

float RepairDesign::bufferInputMaxSlew(LibertyCell* buffer,
                                       const Corner* corner) const
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  return resizer_->maxInputSlew(input, corner);
}

// Find the output port load capacitance that results in slew.
double RepairDesign::findSlewLoadCap(LibertyPort* drvr_port,
                                     double slew,
                                     const Corner* corner)
{
  const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
  double drvr_res = drvr_port->driveResistance();
  if (drvr_res == 0.0) {
    return INF;
  }
  // cap1 lower bound
  // cap2 upper bound
  double cap1 = 0.0;
  double cap2 = slew / drvr_res * 2;
  double tol = .01;  // 1%
  double diff1 = gateSlewDiff(drvr_port, cap2, slew, dcalc_ap);
  // binary search for diff = 0.
  while (abs(cap1 - cap2) > max(cap1, cap2) * tol) {
    if (diff1 < 0.0) {
      cap1 = cap2;
      cap2 *= 2;
      diff1 = gateSlewDiff(drvr_port, cap2, slew, dcalc_ap);
    } else {
      double cap3 = (cap1 + cap2) / 2.0;
      double diff2 = gateSlewDiff(drvr_port, cap3, slew, dcalc_ap);
      if (diff2 < 0.0) {
        cap1 = cap3;
      } else {
        cap2 = cap3;
        diff1 = diff2;
      }
    }
  }
  return cap1;
}

// objective function
double RepairDesign::gateSlewDiff(LibertyPort* drvr_port,
                                  double load_cap,
                                  double slew,
                                  const DcalcAnalysisPt* dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  resizer_->gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  Slew gate_slew
      = max(slews[RiseFall::riseIndex()], slews[RiseFall::fallIndex()]);
  return gate_slew - slew;
}

////////////////////////////////////////////////////////////////

void RepairDesign::repairNet(const BufferedNetPtr& bnet,
                             const Pin* drvr_pin,
                             float max_cap,
                             int max_length,  // dbu
                             const Corner* corner)
{
  drvr_pin_ = drvr_pin;
  max_cap_ = max_cap;
  max_length_ = max_length;
  corner_ = corner;

  int wire_length;
  PinSeq load_pins;
  repairNet(bnet, 0, wire_length, load_pins);
}

void RepairDesign::repairNet(const BufferedNetPtr& bnet,
                             int level,
                             // Return values.
                             // Remaining parasiics after repeater insertion.
                             int& wire_length,  // dbu
                             PinSeq& load_pins)
{
  switch (bnet->type()) {
    case BufferedNetType::wire:
      repairNetWire(bnet, level, wire_length, load_pins);
      break;
    case BufferedNetType::junction:
      repairNetJunc(bnet, level, wire_length, load_pins);
      break;
    case BufferedNetType::load:
      repairNetLoad(bnet, level, wire_length, load_pins);
      break;
    case BufferedNetType::buffer:
      logger_->critical(RSZ, 72, "unhandled BufferedNet type");
      break;
  }
}

void RepairDesign::repairNetWire(
    const BufferedNetPtr& bnet,
    int level,
    // Return values.
    // Remaining parasiics after repeater insertion.
    int& wire_length,  // dbu
    PinSeq& load_pins)
{
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}{}",
             "",
             level,
             bnet->to_string(resizer_));
  int wire_length_ref;
  repairNet(bnet->ref(), level + 1, wire_length_ref, load_pins);
  float max_load_slew = bnet->maxLoadSlew();
  float max_load_slew_margined
      = max_load_slew;  // maxSlewMargined(max_load_slew);

  Point to_loc = bnet->ref()->location();
  int to_x = to_loc.getX();
  int to_y = to_loc.getY();
  Point from_loc = bnet->location();
  int length = Point::manhattanDistance(from_loc, to_loc);
  wire_length = wire_length_ref + length;
  int from_x = from_loc.getX();
  int from_y = from_loc.getY();
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}wl={} l={}",
             "",
             level,
             units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
             units_->distanceUnit()->asString(dbuToMeters(length), 1));
  double length1 = dbuToMeters(length);
  double wire_res, wire_cap;
  bnet->wireRC(corner_, resizer_, wire_res, wire_cap);
  // ref_cap includes ref's wire cap
  double ref_cap = bnet->ref()->cap();
  double load_cap = length1 * wire_cap + ref_cap;

  float r_drvr = resizer_->driveResistance(drvr_pin_);
  double load_slew = (r_drvr + dbuToMeters(wire_length) * wire_res) * load_cap
                     * elmore_skew_factor_;
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}load_slew={} r_drvr={}",
             "",
             level,
             delayAsString(load_slew, this, 3),
             units_->resistanceUnit()->asString(r_drvr, 3));

  LibertyCell* buffer_cell = resizer_->findTargetCell(
      resizer_->buffer_lowest_drive_, load_cap, false);
  bnet->setCapacitance(load_cap);
  bnet->setFanout(bnet->ref()->fanout());

  // Check that the slew limit specified is within the bounds of reason.
  pre_checks_->checkSlewLimit(ref_cap, max_load_slew);
  //============================================================================
  // Back up from pt to from_pt adding repeaters as necessary for
  // length/max_cap/max_slew violations.
  while ((max_length_ > 0 && wire_length > max_length_)
         || (wire_cap > 0.0 && max_cap_ > 0.0 && load_cap > max_cap_)
         || load_slew > max_load_slew_margined) {
    // Make the wire a bit shorter than necessary to allow for
    // offset from instance origin to pin and detailed placement movement.
    constexpr double length_margin = .05;
    bool split_wire = false;
    bool resize = true;
    // Distance from repeater to ref_.
    //              length
    // from----------------------------to/ref
    //
    //                     split_length
    // from-------repeater-------------to/ref
    int split_length = std::numeric_limits<int>::max();
    if (max_length_ > 0 && wire_length > max_length_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 3,
                 "{:{}s}max wire length violation {} > {}",
                 "",
                 level,
                 units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
                 units_->distanceUnit()->asString(dbuToMeters(max_length_), 1));
      split_length = min(max(max_length_ - wire_length_ref, 0), length / 2);

      split_wire = true;
    }
    if (wire_cap > 0.0 && load_cap > max_cap_) {
      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 3,
                 "{:{}s}max cap violation {} > {}",
                 "",
                 level,
                 units_->capacitanceUnit()->asString(load_cap, 3),
                 units_->capacitanceUnit()->asString(max_cap_, 3));
      if (ref_cap > max_cap_) {
        split_length = 0;
        split_wire = false;
      } else {
        split_length
            = min(split_length, metersToDbu((max_cap_ - ref_cap) / wire_cap));
        split_wire = true;
      }
    }
    if (load_slew > max_load_slew_margined) {
      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 3,
                 "{:{}s}max load slew violation {} > {}",
                 "",
                 level,
                 delayAsString(load_slew, this, 3),
                 delayAsString(max_load_slew_margined, this, 3));
      // Using elmore delay to approximate wire
      // load_slew = (Rbuffer + (L+Lref)*Rwire) * (L*Cwire + Cref) *
      // elmore_skew_factor_ Setting this to max_load_slew_margined is a
      // quadratic in L L^2*Rwire*Cwire + L*(Rbuffer*Cwire + Rwire*Cref +
      // Rwire*Cwire*Lref)
      //   + Rbuffer*Cref - max_load_slew_margined/elmore_skew_factor_
      // Solve using quadradic eqn for L.
      float wire_length_ref1 = dbuToMeters(wire_length_ref);
      float r_buffer = resizer_->bufferDriveResistance(buffer_cell);
      float a = wire_res * wire_cap;
      float b = r_buffer * wire_cap + wire_res * ref_cap
                + wire_res * wire_cap * wire_length_ref1;
      float c = r_buffer * ref_cap + wire_length_ref1 * wire_res * ref_cap
                - max_load_slew_margined / elmore_skew_factor_;
      float l = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
      if (l >= 0.0) {
        if (split_length > 0.0) {
          split_length = min(split_length, metersToDbu(l));
        } else {
          split_length = metersToDbu(l);
        }
        split_wire = true;
        resize = false;
      } else {
        split_length = 0;
        split_wire = false;
        resize = true;
      }
    }

    if (split_wire) {
      debugPrint(
          logger_,
          RSZ,
          "repair_net",
          3,
          "{:{}s}split length={}",
          "",
          level,
          units_->distanceUnit()->asString(dbuToMeters(split_length), 1));
      // Distance from to_pt to repeater backward toward from_pt.
      // Note that split_length can be longer than the wire length
      // because it is the maximum value that satisfies max slew/cap.
      double buf_dist = (split_length >= length)
                            ? length
                            : split_length * (1.0 - length_margin);
      double dx = from_x - to_x;
      double dy = from_y - to_y;
      double d = (length == 0) ? 0.0 : buf_dist / length;
      int buf_x = to_x + d * dx;
      int buf_y = to_y + d * dy;
      float repeater_cap, repeater_fanout;
      if (!makeRepeater("wire",
                        Point(buf_x, buf_y),
                        buffer_cell,
                        resize,
                        level,
                        load_pins,
                        repeater_cap,
                        repeater_fanout,
                        max_load_slew)) {
        break;
      }
      // Update for the next round.
      length -= buf_dist;
      wire_length = length;
      to_x = buf_x;
      to_y = buf_y;

      length1 = dbuToMeters(length);
      wire_length_ref = 0.0;
      load_cap = repeater_cap + length1 * wire_cap;
      ref_cap = repeater_cap;
      max_load_slew_margined
          = max_load_slew;  // maxSlewMargined(max_load_slew);
      load_slew
          = (r_drvr + length1 * wire_res) * load_cap * elmore_skew_factor_;
      buffer_cell = resizer_->findTargetCell(
          resizer_->buffer_lowest_drive_, load_cap, false);

      bnet->setCapacitance(load_cap);
      bnet->setFanout(repeater_fanout);
      bnet->setMaxLoadSlew(max_load_slew);

      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 3,
                 "{:{}s}l={}",
                 "",
                 level,
                 units_->distanceUnit()->asString(length1, 1));
    } else {
      break;
    }
  }
}

float RepairDesign::maxSlewMargined(float max_slew)
{
  return max_slew * (1.0 - slew_margin_ / 100.0);
}

void RepairDesign::repairNetJunc(
    const BufferedNetPtr& bnet,
    int level,
    // Return values.
    // Remaining parasiics after repeater insertion.
    int& wire_length,  // dbu
    PinSeq& load_pins)
{
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}{}",
             "",
             level,
             bnet->to_string(resizer_));
  Point loc = bnet->location();
  double wire_res, wire_cap;
  resizer_->wireSignalRC(corner_, wire_res, wire_cap);

  BufferedNetPtr left = bnet->ref();
  int wire_length_left = 0;
  PinSeq loads_left;
  repairNet(left, level + 1, wire_length_left, loads_left);
  float cap_left = left->cap();
  float fanout_left = left->fanout();
  float max_load_slew_left = left->maxLoadSlew();

  BufferedNetPtr right = bnet->ref2();
  int wire_length_right = 0;
  PinSeq loads_right;
  repairNet(right, level + 1, wire_length_right, loads_right);
  float cap_right = right->cap();
  float fanout_right = right->fanout();
  float max_load_slew_right = right->maxLoadSlew();

  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}left  l={} cap={} fanout={}",
             "",
             level,
             units_->distanceUnit()->asString(dbuToMeters(wire_length_left), 1),
             units_->capacitanceUnit()->asString(cap_left, 3),
             fanout_left);
  debugPrint(
      logger_,
      RSZ,
      "repair_net",
      3,
      "{:{}s}right l={} cap={} fanout={}",
      "",
      level,
      units_->distanceUnit()->asString(dbuToMeters(wire_length_right), 1),
      units_->capacitanceUnit()->asString(cap_right, 3),
      fanout_right);

  wire_length = wire_length_left + wire_length_right;
  float load_cap = cap_left + cap_right;
  float max_load_slew = min(max_load_slew_left, max_load_slew_right);
  float max_load_slew_margined
      = max_load_slew;  // maxSlewMargined(max_load_slew);
  LibertyCell* buffer_cell = resizer_->findTargetCell(
      resizer_->buffer_lowest_drive_, load_cap, false);

  // Check for violations when the left/right branches are combined.
  // Add a buffer to left or right branch to stay under the max
  // cap/length/fanout.
  bool repeater_left = false;
  bool repeater_right = false;
  float r_drvr = resizer_->driveResistance(drvr_pin_);
  float load_slew = (r_drvr + dbuToMeters(wire_length) * wire_res) * load_cap
                    * elmore_skew_factor_;
  bool load_slew_violation = load_slew > max_load_slew_margined;
  const char* repeater_reason = nullptr;
  // Driver slew checks were converted to max cap.
  if (load_slew_violation) {
    debugPrint(logger_,
               RSZ,
               "repair_net",
               3,
               "{:{}s}load slew violation {} > {}",
               "",
               level,
               delayAsString(load_slew, this, 3),
               delayAsString(max_load_slew_margined, this, 3));
    float slew_left = (r_drvr + dbuToMeters(wire_length_left) * wire_res)
                      * cap_left * elmore_skew_factor_;
    float slew_slack_left = maxSlewMargined(max_load_slew_left) - slew_left;
    float slew_right = (r_drvr + dbuToMeters(wire_length_right) * wire_res)
                       * cap_right * elmore_skew_factor_;
    float slew_slack_right = maxSlewMargined(max_load_slew_right) - slew_right;
    debugPrint(logger_,
               RSZ,
               "repair_net",
               3,
               "{:{}s} slew slack left={} right={}",
               "",
               level,
               delayAsString(slew_slack_left, this, 3),
               delayAsString(slew_slack_right, this, 3));
    // Isolate the branch with the smaller slack.
    if (slew_slack_left < slew_slack_right) {
      repeater_left = true;
    } else {
      repeater_right = true;
    }
    repeater_reason = "load_slew";
  }
  bool cap_violation = (cap_left + cap_right) > max_cap_;
  if (cap_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}cap violation", "", level);
    if (cap_left > cap_right) {
      repeater_left = true;
    } else {
      repeater_right = true;
    }
    repeater_reason = "max_cap";
  }
  bool length_violation
      = max_length_ > 0 && (wire_length_left + wire_length_right) > max_length_;
  if (length_violation) {
    debugPrint(
        logger_, RSZ, "repair_net", 3, "{:{}s}length violation", "", level);
    if (wire_length_left > wire_length_right) {
      repeater_left = true;
    } else {
      repeater_right = true;
    }
    repeater_reason = "max_length";
  }

  if (repeater_left) {
    makeRepeater(repeater_reason,
                 loc,
                 buffer_cell,
                 true,
                 level,
                 loads_left,
                 cap_left,
                 fanout_left,
                 max_load_slew_left);
    wire_length_left = 0;
  }
  if (repeater_right) {
    makeRepeater(repeater_reason,
                 loc,
                 buffer_cell,
                 true,
                 level,
                 loads_right,
                 cap_right,
                 fanout_right,
                 max_load_slew_right);
    wire_length_right = 0;
  }

  // Update after left/right repeaters are inserted.
  wire_length = wire_length_left + wire_length_right;

  bnet->setCapacitance(cap_left + cap_right);
  bnet->setFanout(fanout_right + fanout_left);
  bnet->setMaxLoadSlew(min(max_load_slew_left, max_load_slew_right));

  // Union left/right load pins.
  for (const Pin* load_pin : loads_left) {
    load_pins.push_back(load_pin);
  }
  for (const Pin* load_pin : loads_right) {
    load_pins.push_back(load_pin);
  }
}

void RepairDesign::repairNetLoad(
    const BufferedNetPtr& bnet,
    int level,
    // Return values.
    // Remaining parasiics after repeater insertion.
    int& wire_length,  // dbu
    PinSeq& load_pins)
{
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}{}",
             "",
             level,
             bnet->to_string(resizer_));
  const Pin* load_pin = bnet->loadPin();
  debugPrint(logger_,
             RSZ,
             "repair_net",
             2,
             "{:{}s}load {}",
             "",
             level,
             sdc_network_->pathName(load_pin));
  wire_length = 0;
  load_pins.push_back(load_pin);
}

////////////////////////////////////////////////////////////////

LoadRegion::LoadRegion() = default;

LoadRegion::LoadRegion(PinSeq& pins, Rect& bbox) : pins_(pins), bbox_(bbox)
{
}

LoadRegion RepairDesign::findLoadRegions(const Net* net,
                                         const Pin* drvr_pin,
                                         int max_fanout)
{
  PinSeq loads = findLoads(drvr_pin);
  Rect bbox = findBbox(loads);
  LoadRegion region(loads, bbox);
  if (graphics_) {
    odb::dbNet* db_net = db_network_->staToDb(net);
    graphics_->subdivideStart(db_net);
  }
  subdivideRegion(region, max_fanout);
  if (graphics_) {
    graphics_->subdivideDone();
  }
  return region;
}

void RepairDesign::subdivideRegion(LoadRegion& region, int max_fanout)
{
  if (region.pins_.size() > max_fanout && region.bbox_.dx() > dbu_
      && region.bbox_.dy() > dbu_) {
    int x_min = region.bbox_.xMin();
    int x_max = region.bbox_.xMax();
    int y_min = region.bbox_.yMin();
    int y_max = region.bbox_.yMax();
    region.regions_.resize(2);
    int x_mid = (x_min + x_max) / 2;
    int y_mid = (y_min + y_max) / 2;
    bool horz_partition;
    odb::Line cut;
    if (region.bbox_.dx() > region.bbox_.dy()) {
      region.regions_[0].bbox_ = Rect(x_min, y_min, x_mid, y_max);
      region.regions_[1].bbox_ = Rect(x_mid, y_min, x_max, y_max);
      cut = odb::Line{x_mid, y_min, x_mid, y_max};
      horz_partition = true;
    } else {
      region.regions_[0].bbox_ = Rect(x_min, y_min, x_max, y_mid);
      region.regions_[1].bbox_ = Rect(x_min, y_mid, x_max, y_max);
      horz_partition = false;
      cut = odb::Line{x_min, y_mid, x_max, y_mid};
    }
    for (const Pin* pin : region.pins_) {
      Point loc = db_network_->location(pin);
      int x = loc.x();
      int y = loc.y();
      if ((horz_partition && x <= x_mid) || (!horz_partition && y <= y_mid)) {
        region.regions_[0].pins_.push_back(pin);
      } else if ((horz_partition && x > x_mid)
                 || (!horz_partition && y > y_mid)) {
        region.regions_[1].pins_.push_back(pin);
      } else {
        logger_->critical(RSZ, 83, "pin outside regions");
      }
    }
    if (graphics_) {
      graphics_->subdivide(cut);
    }
    region.pins_.clear();
    for (LoadRegion& sub : region.regions_) {
      if (!sub.pins_.empty()) {
        subdivideRegion(sub, max_fanout);
      }
    }
  }
}

void RepairDesign::makeRegionRepeaters(LoadRegion& region,
                                       int max_fanout,
                                       int level,
                                       const Pin* drvr_pin,
                                       bool check_slew,
                                       bool check_cap,
                                       int max_length,
                                       bool resize_drvr)
{
  // Leaf regions have less than max_fanout pins and are buffered
  // by the enclosing region.
  if (!region.regions_.empty()) {
    // Buffer from the bottom up.
    for (LoadRegion& sub : region.regions_) {
      makeRegionRepeaters(sub,
                          max_fanout,
                          level + 1,
                          drvr_pin,
                          check_slew,
                          check_cap,
                          max_length,
                          resize_drvr);
    }

    PinSeq repeater_inputs;
    PinSeq repeater_loads;
    for (LoadRegion& sub : region.regions_) {
      PinSeq& sub_pins = sub.pins_;
      while (!sub_pins.empty()) {
        repeater_loads.push_back(sub_pins.back());
        sub_pins.pop_back();
        if (repeater_loads.size() == max_fanout) {
          makeFanoutRepeater(repeater_loads,
                             repeater_inputs,
                             region.bbox_,
                             findClosedPinLoc(drvr_pin, repeater_loads),
                             check_slew,
                             check_cap,
                             max_length,
                             resize_drvr);
        }
      }
    }
    while (!region.pins_.empty()) {
      repeater_loads.push_back(region.pins_.back());
      region.pins_.pop_back();
      if (repeater_loads.size() == max_fanout) {
        makeFanoutRepeater(repeater_loads,
                           repeater_inputs,
                           region.bbox_,
                           findClosedPinLoc(drvr_pin, repeater_loads),
                           check_slew,
                           check_cap,
                           max_length,
                           resize_drvr);
      }
    }
    if (!repeater_loads.empty() && repeater_loads.size() >= max_fanout / 2) {
      makeFanoutRepeater(repeater_loads,
                         repeater_inputs,
                         region.bbox_,
                         findClosedPinLoc(drvr_pin, repeater_loads),
                         check_slew,
                         check_cap,
                         max_length,
                         resize_drvr);
    } else {
      region.pins_ = std::move(repeater_loads);
    }

    for (const Pin* pin : repeater_inputs) {
      region.pins_.push_back(pin);
    }
  }
}

void RepairDesign::makeFanoutRepeater(PinSeq& repeater_loads,
                                      PinSeq& repeater_inputs,
                                      const Rect& bbox,
                                      const Point& loc,
                                      bool check_slew,
                                      bool check_cap,
                                      int max_length,
                                      bool resize_drvr)
{
  float ignore2, ignore3, ignore4;
  Net* out_net;
  Pin *repeater_in_pin, *repeater_out_pin;
  if (!makeRepeater("fanout",
                    loc.x(),
                    loc.y(),
                    resizer_->buffer_lowest_drive_,
                    false,
                    1,
                    repeater_loads,
                    ignore2,
                    ignore3,
                    ignore4,
                    out_net,
                    repeater_in_pin,
                    repeater_out_pin)) {
    return;
  }
  Vertex* repeater_out_vertex = graph_->pinDrvrVertex(repeater_out_pin);
  int repaired_net_count = 0, slew_violations = 0, cap_violations = 0;
  int fanout_violations = 0, length_violations = 0;

  repairNet(out_net,
            repeater_out_pin,
            repeater_out_vertex,
            check_slew,
            check_cap,
            false /* check_fanout */,
            max_length,
            resize_drvr,
            repaired_net_count,
            slew_violations,
            cap_violations,
            fanout_violations,
            length_violations);
  repeater_inputs.push_back(repeater_in_pin);
  repeater_loads.clear();
}

Rect RepairDesign::findBbox(PinSeq& pins)
{
  Rect bbox;
  bbox.mergeInit();
  for (const Pin* pin : pins) {
    Point loc = db_network_->location(pin);
    Rect r(loc.x(), loc.y(), loc.x(), loc.y());
    bbox.merge(r);
  }
  return bbox;
}

PinSeq RepairDesign::findLoads(const Pin* drvr_pin)
{
  PinSeq loads;
  Pin* drvr_pin1 = const_cast<Pin*>(drvr_pin);
  PinSeq drvrs;
  PinSet visited_drvrs(db_network_);
  sta::FindNetDrvrLoads visitor(
      drvr_pin1, visited_drvrs, loads, drvrs, network_);
  network_->visitConnectedPins(drvr_pin1, visitor);
  return loads;
}

Point RepairDesign::findClosedPinLoc(const Pin* drvr_pin, PinSeq& pins)
{
  Point drvr_loc = db_network_->location(drvr_pin);
  Point closest_pt = drvr_loc;
  int64_t closest_dist = std::numeric_limits<int64_t>::max();
  for (const Pin* pin : pins) {
    Point loc = db_network_->location(pin);
    int64_t dist = Point::manhattanDistance(loc, drvr_loc);
    if (dist < closest_dist) {
      closest_pt = loc;
      closest_dist = dist;
    }
  }
  return closest_pt;
}

bool RepairDesign::isRepeater(const Pin* load_pin)
{
  dbInst* db_inst = db_network_->staToDb(network_->instance(load_pin));
  odb::dbSourceType source = db_inst->getSourceType();
  return source == odb::dbSourceType::TIMING;
}

////////////////////////////////////////////////////////////////

bool RepairDesign::makeRepeater(const char* reason,
                                const Point& loc,
                                LibertyCell* buffer_cell,
                                bool resize,
                                int level,
                                // Return values.
                                PinSeq& load_pins,
                                float& repeater_cap,
                                float& repeater_fanout,
                                float& repeater_max_slew)
{
  Net* out_net;
  Pin *repeater_in_pin, *repeater_out_pin;
  return makeRepeater(reason,
                      loc.getX(),
                      loc.getY(),
                      buffer_cell,
                      resize,
                      level,
                      load_pins,
                      repeater_cap,
                      repeater_fanout,
                      repeater_max_slew,
                      out_net,
                      repeater_in_pin,
                      repeater_out_pin);
}

////////////////////////////////////////////////////////////////

bool RepairDesign::hasInputPort(const Net* net)
{
  bool has_top_level_port = false;
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->isTopLevelPort(pin)
        && network_->direction(pin)->isAnyInput()) {
      has_top_level_port = true;
      break;
    }
  }
  delete pin_iter;
  return has_top_level_port;
}

bool RepairDesign::makeRepeater(
    const char* reason,
    int x,
    int y,
    LibertyCell* buffer_cell,
    bool resize,
    int level,
    // Return values.
    PinSeq& load_pins,  // inout, read, reset, repopulated.
    float& repeater_cap,
    float& repeater_fanout,
    float& repeater_max_slew,
    Net*& out_net,
    Pin*& repeater_in_pin,
    Pin*& repeater_out_pin)
{
  LibertyPort *buffer_input_port, *buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
  std::string buffer_name = resizer_->makeUniqueInstName(reason);

  debugPrint(logger_,
             RSZ,
             "repair_net",
             2,
             "{:{}s}{} {} {} ({} {})",
             "",
             level,
             reason,
             buffer_name.c_str(),
             buffer_cell->name(),
             units_->distanceUnit()->asString(dbuToMeters(x), 1),
             units_->distanceUnit()->asString(dbuToMeters(y), 1));

  // Inserting a buffer is complicated by the fact that verilog netlists
  // use the net name for input and output ports. This means the ports
  // cannot be moved to a different net.

  // This cannot depend on the net in caller because the buffer may be inserted
  // between the driver and the loads changing the net as the repair works its
  // way from the loads to the driver.

  Net* load_net = nullptr;
  dbNet* load_db_net = nullptr;  // load net, flat

  bool preserve_outputs = false;
  bool top_primary_output = false;

  // Determine the type of the load
  // primary output/ dont touch

  for (const Pin* pin : load_pins) {
    if (network_->isTopLevelPort(pin)) {
      load_db_net = db_network_->flatNet(network_->term(pin));

      // filter: is the top pin a primary output
      if (network_->direction(pin)->isAnyOutput()) {
        preserve_outputs = true;
        top_primary_output = true;
        break;
      }
    } else {
      load_db_net = db_network_->flatNet(pin);

      Instance* inst = network_->instance(pin);
      if (resizer_->dontTouch(inst)) {
        preserve_outputs = true;
        break;
      }
    }
  }

  // force the load net to be a flat net
  load_net = db_network_->dbToSta(load_db_net);

  const bool keep_input = hasInputPort(load_net) || !preserve_outputs;

  // check for dont_touch

  if (!keep_input) {
    // check if driving port is dont touch and reject
    bool driving_pin_dont_touch = false;
    std::unique_ptr<NetPinIterator> pin_iter(network_->pinIterator(load_net));
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      if (network_->direction(pin)->isAnyOutput() && resizer_->dontTouch(pin)) {
        driving_pin_dont_touch = true;
        break;
      }
    }

    if (driving_pin_dont_touch) {
      debugPrint(
          logger_,
          utl::RSZ,
          "repair_net",
          3,
          "Cannot create repeater due to driving pin on {} being dont_touch",
          network_->name(load_net));
      return false;
    }
  }

  PinSet repeater_load_pins(db_network_);

  bool connections_will_be_modified = false;
  if (keep_input) {
    //
    // Case 1
    //------
    // A primary input or do not preserve the outputs
    //
    // use orig net as buffer ip (keep primary input name exposed)
    // use new net as buffer op (ok to use new name on op of buffer).
    // move loads to op side (so might need to rename any hierarchical
    // nets to avoid conflict of names with primary input net).
    //
    // record the driver pin modnet, if any

    for (const Pin* pin : load_pins) {
      Instance* inst = network_->instance(pin);
      if (resizer_->dontTouch(inst)) {
        continue;
      }

      connections_will_be_modified = true;
    }
  } else /* case 2 */ {
    //
    // case 2. One of the loads is a primary output or a dont touch
    // Note that even if all loads dont touch we still insert a buffer
    //
    // Use the new net as the buffer input. Preserve
    // the output net as is. Transfer non repeater loads
    // to input side
    for (const Pin* pin : load_pins) {
      repeater_load_pins.insert(pin);
    }
    // put non repeater loads from op net onto ip net, preserving
    // any hierarchical connection
    std::unique_ptr<NetPinIterator> pin_iter(network_->pinIterator(load_net));
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      if (!repeater_load_pins.hasKey(pin)) {
        Instance* inst = network_->instance(pin);
        // do not disconnect/reconnect don't touch instances
        if (resizer_->dontTouch(inst)) {
          continue;
        }
        connections_will_be_modified = true;
      }
    }
  }  // case 2

  if (!connections_will_be_modified) {
    debugPrint(logger_,
               utl::RSZ,
               "repair_net",
               3,
               "New buffer will not connected to anything on {}.",
               network_->name(load_net));

    // no connections change, so this buffer will be left floating
    repeater_cap = 0;
    repeater_fanout = 0;
    repeater_max_slew = 0;

    return false;
  }

  // Determine parent to put buffer (and net)
  // Determine the driver pin
  // Make the buffer in the root module in case or primary input connections

  Instance* parent = nullptr;
  Pin* driver_pin = nullptr;
  Instance* driver_instance_parent = nullptr;

  if (hasInputPort(load_net) || top_primary_output
      || !db_network_->hasHierarchy()) {
    (void) (db_network_->getNetDriverParentModule(load_net, driver_pin, true));
    parent = db_network_->topInstance();
  } else {
    odb::dbModule* parent_module
        = db_network_->getNetDriverParentModule(load_net, driver_pin, true);
    if (parent_module) {
      odb::dbModInst* parent_mod_inst = parent_module->getModInst();
      if (parent_mod_inst) {
        parent = db_network_->dbToSta(parent_mod_inst);
      } else {
        parent = db_network_->topInstance();
      }
    } else {
      parent = db_network_->topInstance();
    }
  }

  Point buf_loc(x, y);
  Instance* buffer
      = resizer_->makeBuffer(buffer_cell, buffer_name.c_str(), parent, buf_loc);
  driver_instance_parent = parent;

  inserted_buffer_count_++;

  Pin* buffer_ip_pin = nullptr;
  Pin* buffer_op_pin = nullptr;
  resizer_->getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);

  // make sure any nets created are scoped within hierarchy
  // backwards compatible. new naming only used for hierarchy code.
  std::string net_name = db_network_->hasHierarchy()
                             ? resizer_->makeUniqueNetName(parent)
                             : resizer_->makeUniqueNetName();
  Net* new_net = db_network_->makeNet(net_name.c_str(), parent);

  Net* buffer_ip_net = nullptr;
  Net* buffer_op_net = nullptr;

  if (keep_input) {
    //
    // Case 1
    //------
    // A primary input or do not preserve the outputs
    //
    // use orig net as buffer ip (keep primary input name exposed)
    // use new net as buffer op (ok to use new name on op of buffer).
    // move loads to op side (so might need to rename any hierarchical
    // nets to avoid conflict of names with primary input net).
    //
    // record the driver pin modnet, if any
    odb::dbModNet* driver_pin_mod_net = db_network_->hierNet(driver_pin);

    //
    // Copy signal type to new net.
    //
    dbNet* ip_net_db = load_db_net;
    dbNet* op_net_db = db_network_->staToDb(new_net);
    op_net_db->setSigType(ip_net_db->getSigType());
    out_net = new_net;

    buffer_op_net = new_net;
    buffer_ip_net = db_network_->dbToSta(ip_net_db);

    for (const Pin* pin : load_pins) {
      // skip any hierarchical pins in loads
      // in loads.

      if (db_network_->hierPin(pin)) {
        continue;
      }

      Instance* inst = network_->instance(pin);
      if (resizer_->dontTouch(inst)) {
        continue;
      }
      // preserve any hierarchical connection on the load
      // & also connect the buffer output net to this pin
      load_db_net = db_network_->flatNet(pin);

      // New api call: simultaneously disconnects old flat/hier net
      // and connects in new one.

      Instance* load_instance_parent
          = db_network_->getOwningInstanceParent(const_cast<Pin*>(pin));

      db_network_->disconnectPin(const_cast<Pin*>(pin));
      if (db_network_->hasHierarchy()
          && (driver_instance_parent != load_instance_parent)) {
        // In hierarchical mode, construct as necessary hierarchical connection
        // Use the original connection name if possible.
        std::string connection_name;
        if (!driver_pin_mod_net) {
          connection_name = resizer_->makeUniqueNetName(parent);
        } else {
          connection_name = driver_pin_mod_net->getName();
        }
        db_network_->hierarchicalConnect(db_network_->flatPin(buffer_op_pin),
                                         db_network_->flatPin(pin),
                                         connection_name.c_str());
      } else {
        // flat mode, no hierarchy, just hook up flat nets.
        db_network_->connectPin(const_cast<Pin*>(pin), buffer_op_net);
      }
    }
    db_network_->connectPin(buffer_ip_pin, db_network_->dbToSta(load_db_net));
    db_network_->connectPin(buffer_op_pin, buffer_op_net);

    // Preserve any driver pin hierarchical mod net connection
    // push to the output of the buffer. Rename the mod net.
    // to prevent a name clash (recall the primary port net
    // names need to be preserved, so we rename the mod net).
    if (driver_pin_mod_net) {
      dbNet* flat_net = db_network_->flatNet(driver_pin);
      if (!strcmp(flat_net->getName().c_str(), driver_pin_mod_net->getName())) {
        Instance* owning_instance = db_network_->dbToSta(
            driver_pin_mod_net->getParent()->getModInst());
        std::string new_mod_net_name
            = resizer_->makeUniqueNetName(owning_instance);
        driver_pin_mod_net->rename(new_mod_net_name.c_str());
      }
      db_network_->disconnectPin(driver_pin);

      db_network_->connectPin(driver_pin, db_network_->dbToSta(flat_net));
      // connect the propagated hierarchical net to the buffer output
      db_network_->connectPin(buffer_op_pin,
                              db_network_->dbToSta(driver_pin_mod_net));
    }
  } else /* case 2 */ {
    //
    // case 2. One of the loads is a primary output or a dont touch
    // Note that even if all loads dont touch we still insert a buffer
    //
    // Use the new net as the buffer input. Preserve
    // the output net as is. Transfer non repeater loads
    // to input side

    out_net = load_net;
    Net* ip_net = new_net;
    dbNet* op_net_db = load_db_net;
    dbNet* ip_net_db = db_network_->staToDb(new_net);
    ip_net_db->setSigType(op_net_db->getSigType());

    buffer_ip_net = new_net;
    buffer_op_net = db_network_->dbToSta(load_db_net);

    // put non repeater loads from op net onto ip net, preserving
    // any hierarchical connection
    std::unique_ptr<NetPinIterator> pin_iter(network_->pinIterator(load_net));

    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();

      if (db_network_->hierPin(pin)) {
        continue;
      }

      if (!repeater_load_pins.hasKey(pin)) {
        Instance* inst = network_->instance(pin);
        // do not disconnect/reconnect don't touch instances
        if (resizer_->dontTouch(inst)) {
          continue;
        }
        // preserve any hierarchical connection
        odb::dbModNet* mod_net = db_network_->hierNet(pin);
        db_network_->disconnectPin(const_cast<Pin*>(pin));
        db_network_->connectPin(const_cast<Pin*>(pin), ip_net);
        if (mod_net) {
          db_network_->connectPin(const_cast<Pin*>(pin),
                                  db_network_->dbToSta(mod_net));
        }
      }
    }
    db_network_->connectPin(buffer_ip_pin, buffer_ip_net);
    db_network_->connectPin(buffer_op_pin, buffer_op_net);
  }  // case 2

  resizer_->parasiticsInvalid(buffer_ip_net);
  resizer_->parasiticsInvalid(buffer_op_net);

  // Resize repeater as we back up by levels.
  if (resize) {
    Pin* buffer_out_pin = network_->findPin(buffer, buffer_output_port);
    resizer_->resizeToTargetSlew(buffer_out_pin);
    buffer_cell = network_->libertyCell(buffer);
    buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
  }

  repeater_in_pin = network_->findPin(buffer, buffer_input_port);
  repeater_out_pin = network_->findPin(buffer, buffer_output_port);

  load_pins.clear();
  load_pins.push_back(repeater_in_pin);
  repeater_cap = resizer_->portCapacitance(buffer_input_port, corner_);
  repeater_fanout = resizer_->portFanoutLoad(buffer_input_port);
  repeater_max_slew = bufferInputMaxSlew(buffer_cell, corner_);

  return true;
}

LibertyCell* RepairDesign::findBufferUnderSlew(float max_slew, float load_cap)
{
  LibertyCell* min_slew_buffer = resizer_->buffer_lowest_drive_;
  float min_slew = INF;
  LibertyCellSeq swappable_cells
      = resizer_->getSwappableCells(resizer_->buffer_lowest_drive_);
  if (!swappable_cells.empty()) {
    sort(swappable_cells,
         [this](const LibertyCell* buffer1, const LibertyCell* buffer2) {
           return resizer_->bufferDriveResistance(buffer1)
                  > resizer_->bufferDriveResistance(buffer2);
         });
    for (LibertyCell* buffer : swappable_cells) {
      float slew = resizer_->bufferSlew(
          buffer, load_cap, resizer_->tgt_slew_dcalc_ap_);
      debugPrint(logger_,
                 RSZ,
                 "buffer_under_slew",
                 1,
                 "{:{}s}pt ({} {})",
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
  // Could not find a buffer under max_slew but this is min slew achievable.
  return min_slew_buffer;
}

double RepairDesign::dbuToMeters(int dist) const
{
  return dist / (dbu_ * 1e+6);
}

int RepairDesign::metersToDbu(double dist) const
{
  return dist * dbu_ * 1e+6;
}

void RepairDesign::printProgress(int iteration,
                                 bool force,
                                 bool end,
                                 int repaired_net_count) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report(
        "Iteration |   Area    | Resized | Buffers | Nets repaired | "
        "Remaining");
    logger_->report(
        "--------------------------------------------------------------------"
        "-");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    const int nets_left = resizer_->level_drvr_vertices_.size() - iteration;

    std::string itr_field = fmt::format("{}", iteration);
    if (end) {
      itr_field = "final";
    }
    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;

    logger_->report(
        "{: >9s} | {: >+8.1f}% | {: >7d} | {: >7d} | {: >13d} | {: >9d}",
        itr_field,
        area_growth / initial_design_area_ * 1e2,
        resize_count_,
        inserted_buffer_count_,
        repaired_net_count,
        nets_left);
  }

  if (end) {
    logger_->report(
        "--------------------------------------------------------------------"
        "-");
  }
}

void RepairDesign::reportViolationCounters(bool invalidate_driver_vertices,
                                           int slew_violations,
                                           int cap_violations,
                                           int fanout_violations,
                                           int length_violations,
                                           int repaired_net_count)
{
  if (slew_violations > 0) {
    logger_->info(utl::RSZ, 34, "Found {} slew violations.", slew_violations);
  }
  if (fanout_violations > 0) {
    logger_->info(
        utl::RSZ, 35, "Found {} fanout violations.", fanout_violations);
  }
  if (cap_violations > 0) {
    logger_->info(
        utl::RSZ, 36, "Found {} capacitance violations.", cap_violations);
  }
  if (length_violations > 0) {
    logger_->info(utl::RSZ, 37, "Found {} long wires.", length_violations);
  }
  if (resize_count_ > 0) {
    logger_->info(utl::RSZ, 39, "Resized {} instances.", resize_count_);
  }
  if (inserted_buffer_count_ > 0) {
    logger_->info(utl::RSZ,
                  invalidate_driver_vertices ? 55 : 38,
                  "Inserted {} buffers in {} nets.",
                  inserted_buffer_count_,
                  repaired_net_count);
    if (invalidate_driver_vertices) {
      resizer_->level_drvr_vertices_valid_ = false;
    }
  }
}

void RepairDesign::setDebugGraphics(std::shared_ptr<ResizerObserver> graphics)
{
  graphics_ = std::move(graphics);
}

}  // namespace rsz
