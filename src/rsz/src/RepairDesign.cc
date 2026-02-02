// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "RepairDesign.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "BufferedNet.hh"
#include "PreChecks.hh"
#include "ResizerObserver.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Clock.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/RiseFallValues.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/StringUtil.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "sta/Vector.hh"
#include "utl/Logger.h"
#include "utl/mem_stats.h"
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
using sta::PortDirection;
using sta::RiseFallBoth;
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
  estimate_parasitics_ = resizer_->estimate_parasitics_;
  pre_checks_ = std::make_unique<PreChecks>(resizer_);
  parasitics_src_ = estimate_parasitics_->getParasiticsSrc();
  initial_design_area_ = resizer_->computeDesignArea();
  computeSlewRCFactor();

  r_strongest_buffer_ = std::numeric_limits<float>::max();
  for (auto buffer : resizer_->buffer_cells_) {
    r_strongest_buffer_ = std::min(r_strongest_buffer_,
                                   resizer_->bufferDriveResistance(buffer));
  }
}

void RepairDesign::computeSlewRCFactor()
{
  using sta::RiseFall;
  const sta::LibertyLibrary* library = network_->defaultLibertyLibrary();
  float factor = 0.0;
  for (auto rf : sta::RiseFall::range()) {
    // cast both rise and fall into 1->0 transition
    float th_low, th_high;
    if (rf == sta::RiseFall::rise()) {
      // flip
      th_low = 1.0 - library->slewUpperThreshold(rf);
      th_high = 1.0 - library->slewLowerThreshold(rf);
    } else {
      th_low = library->slewLowerThreshold(rf);
      th_high = library->slewUpperThreshold(rf);
    }
    // compute crossing times assuming RC=1 where R is driving resistance and C
    // is load
    float t_high = -log(th_high);
    float t_low = -log(th_low);
    // scale by slew derate
    float rf_factor = (t_low - t_high) / library->slewDerateFromLibrary();
    // check the factor has the right order of magnitude
    if (!(rf_factor > 0.1 && rf_factor < 10.0)) {
      logger_->error(
          RSZ,
          101,
          "RC slew modeling shape factor is out of range: {:.3e} for {}",
          rf_factor,
          rf->name());
    }
    debugPrint(logger_,
               RSZ,
               "slew_rc",
               1,
               "transition {} factor {:.3e}",
               rf->name(),
               rf_factor);
    factor = std::max(factor, rf_factor);
  }
  // Apply 10% modeling pessmism
  const float pessimism = 0.10;
  slew_rc_factor_ = factor * (1 + pessimism);
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
  debugPrint(logger_, RSZ, "early_sizing", 1, "Performing early sizing round.");
  // keep track of user annotations so we don't remove them
  std::set<std::pair<sta::Vertex*, int>> slew_user_annotated;

  // We need to override slews in order to get good required time estimates.
  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    sta::Vertex* drvr = resizer_->level_drvr_vertices_[i];
    debugPrint(logger_,
               RSZ,
               "early_sizing",
               2,
               "Annotating slew for driver {}",
               network_->pathName(drvr->pin()));
    for (auto rf : {sta::RiseFall::rise(), sta::RiseFall::fall()}) {
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
    sta::Vertex* drvr = resizer_->level_drvr_vertices_[i];
    sta::Pin* drvr_pin = drvr->pin();
    debugPrint(logger_,
               RSZ,
               "early_sizing",
               2,
               "Processing driver {}",
               network_->pathName(drvr_pin));
    // Always get the flat net for the top level port.
    sta::Net* net = network_->isTopLevelPort(drvr_pin)
                        ? network_->net(network_->term(drvr_pin))
                        : db_network_->dbToSta(db_network_->flatNet(drvr_pin));
    if (!net) {
      continue;
    }

    odb::dbNet* net_db = nullptr;
    odb::dbModNet* mod_net_db = nullptr;
    db_network_->staToDb(net, net_db, mod_net_db);
    search_->findRequireds(drvr->level() + 1);

    if (resizer_->okToBufferNet(drvr_pin)
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells and supply nets.
        && !drvr->isConstant()) {
      debugPrint(logger_,
                 RSZ,
                 "early_sizing",
                 2,
                 "  Net {} is eligible for repair.",
                 network_->pathName(net));
      float fanout, max_fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, max_, fanout, max_fanout, fanout_slack);

      bool repaired_net = false;
      if (max_fanout <= 0) {
        max_fanout = 1e9;
      }

      if (performGainBuffering(net, drvr_pin, max_fanout)) {
        debugPrint(logger_,
                   RSZ,
                   "early_sizing",
                   2,
                   "  Gain buffering applied to net {}.",
                   network_->pathName(net));
        repaired_net = true;
      }

      if (resizer_->resizeToCapRatio(drvr_pin, false)) {
        debugPrint(logger_,
                   RSZ,
                   "early_sizing",
                   2,
                   "  Resized driver {}.",
                   network_->pathName(drvr_pin));
        repaired_net = true;
      }

      if (repaired_net) {
        repaired_net_count++;
      }
    }

    for (auto mm : sta::MinMaxAll::all()->range()) {
      for (auto rf : sta::RiseFallBoth::riseFall()->range()) {
        if (!slew_user_annotated.contains(std::make_pair(drvr, rf->index()))) {
          const sta::DcalcAnalysisPt* dcalc_ap
              = resizer_->tgt_slew_corner_->findDcalcAnalysisPt(mm);
          drvr->setSlewAnnotated(false, rf, dcalc_ap->index());
        }
      }
    }
  }
  debugPrint(logger_, RSZ, "early_sizing", 1, "Early sizing round finished.");

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

  // keep track of annotations which were added by us
  std::set<sta::Vertex*> annotations_to_clean_up;
  std::map<sta::Vertex*, sta::Corner*> drvr_with_load_slew_viol;
  sta::VertexSeq load_vertices = resizer_->orderedLoadPinVertices();

  // Forward pass: whenever we see violating input pin slew we override
  // it in the graph. This is in order to prevent second order upsizing.
  // The load pin slew will get repaired so we shouldn't propagate it
  // downstream.
  for (auto vertex : load_vertices) {
    if (!vertex->slewAnnotated()) {
      sta_->findDelays(vertex);
      sta::LibertyPort* port = network_->libertyPort(vertex->pin());
      if (port) {
        for (auto corner : *sta_->corners()) {
          const sta::DcalcAnalysisPt* dcalc_ap
              = corner->findDcalcAnalysisPt(max_);
          float limit = resizer_->maxInputSlew(port, corner);
          float actual;
          bool slew_viol = false;
          for (const sta::RiseFall* rf : sta::RiseFall::range()) {
            actual = graph_->slew(vertex, rf, dcalc_ap->index());
            if (actual > limit) {
              slew_viol = true;
              break;
            }
          }
          if (slew_viol) {
            sta_->setAnnotatedSlew(vertex,
                                   corner,
                                   max_->asMinMaxAll(),
                                   RiseFallBoth::riseFall(),
                                   limit);
            annotations_to_clean_up.insert(vertex);
            sta::PinSet* drivers = network_->drivers(vertex->pin());
            if (drivers) {
              for (const sta::Pin* drvr_pin : *drivers) {
                drvr_with_load_slew_viol[graph_->pinDrvrVertex(drvr_pin)]
                    = corner;
                debugPrint(logger_,
                           RSZ,
                           "repair_design",
                           2,
                           "Fwd pass: drvr {} has slew {} vs. limit {}",
                           sdc_network_->pathName(drvr_pin),
                           delayAsString(actual, this, 3),
                           delayAsString(limit, this, 3));
              }
            }
          }
        }
      }
    }
  }

  debugPrint(logger_,
             RSZ,
             "repair_design",
             1,
             "annotated slew to non-violating value on {} load vertices",
             annotations_to_clean_up.size());

  int print_iteration = 0;
  {
    // Fix violations from outputs to inputs
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    if (resizer_->level_drvr_vertices_.size()
        > size_t(5) * max_print_interval_) {
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
      sta::Vertex* drvr = resizer_->level_drvr_vertices_[i];
      repairDriver(drvr,
                   true /* check_slew */,
                   true /* check_cap */,
                   true /* check_fanout */,
                   max_length,
                   true /* resize_driver */,
                   drvr_with_load_slew_viol.contains(drvr)
                       ? drvr_with_load_slew_viol.at(drvr)
                       : nullptr,
                   repaired_net_count,
                   slew_violations,
                   cap_violations,
                   fanout_violations,
                   length_violations);
    }
    estimate_parasitics_->updateParasitics();
  }

  if (!annotations_to_clean_up.empty()) {
    for (auto vertex : annotations_to_clean_up) {
      for (auto corner : *sta_->corners()) {
        const sta::DcalcAnalysisPt* dcalc_ap
            = corner->findDcalcAnalysisPt(max_);
        for (const sta::RiseFall* rf : sta::RiseFall::range()) {
          vertex->setSlewAnnotated(false, rf, dcalc_ap->index());
        }
      }
    }
    sta_->delaysInvalid();
  }

  printProgress(print_iteration, true, true, repaired_net_count);
  if (inserted_buffer_count_ > 0) {
    resizer_->level_drvr_vertices_valid_ = false;
  }
  db_network_->removeUnusedPortsAndPinsOnModuleInstances();
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

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    int max_length = resizer_->metersToDbu(max_wire_length);
    for (Clock* clk : sdc_->clks()) {
      const sta::PinSet* clk_pins = sta_->pins(clk);
      if (clk_pins) {
        for (const sta::Pin* clk_pin : *clk_pins) {
          // clang-format off
          sta::Net* net = network_->isTopLevelPort(clk_pin)
                         ? db_network_->dbToSta(
                             db_network_->flatNet(network_->term(clk_pin)))
                         : db_network_->dbToSta(db_network_->flatNet(clk_pin));
          // clang-format on
          if (net && network_->isDriver(clk_pin)) {
            sta::Vertex* drvr = graph_->pinDrvrVertex(clk_pin);
            // Do not resize clock tree gates.
            repairNet(net,
                      clk_pin,
                      drvr,
                      false,
                      false,
                      false,
                      max_length,
                      false,
                      nullptr,
                      repaired_net_count,
                      slew_violations,
                      cap_violations,
                      fanout_violations,
                      length_violations);
          }
        }
      }
    }
  }

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
void RepairDesign::repairNet(sta::Net* net,
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

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    int max_length = resizer_->metersToDbu(max_wire_length);
    sta::PinSet* drivers = network_->drivers(net);
    if (drivers && !drivers->empty()) {
      sta::PinSet::Iterator drvr_iter(drivers);
      const sta::Pin* drvr_pin = drvr_iter.next();
      sta::Vertex* drvr = graph_->pinDrvrVertex(drvr_pin);
      repairNet(net,
                drvr_pin,
                drvr,
                true,
                true,
                true,
                max_length,
                true,
                nullptr,
                repaired_net_count,
                slew_violations,
                cap_violations,
                fanout_violations,
                length_violations);
    }
  }

  reportViolationCounters(true,
                          slew_violations,
                          cap_violations,
                          fanout_violations,
                          length_violations,
                          repaired_net_count);
}

bool RepairDesign::getLargestSizeCin(const sta::Pin* drvr_pin, float& cin)
{
  sta::Instance* inst = network_->instance(drvr_pin);
  sta::LibertyCell* cell = network_->libertyCell(inst);
  cin = 0;
  if (!network_->isTopLevelPort(drvr_pin) && cell != nullptr
      && resizer_->isLogicStdCell(inst)) {
    for (auto size : resizer_->getSwappableCells(cell)) {
      float size_cin = 0;
      sta::LibertyCellPortIterator port_iter(size);
      int nports = 0;
      while (port_iter.hasNext()) {
        const sta::LibertyPort* port = port_iter.next();
        if (!port->isPwrGnd() && port->direction() == PortDirection::input()) {
          size_cin += port->capacitance();
          nports++;
        }
      }
      if (!nports) {
        return false;
      }
      size_cin /= nports;
      cin = std::max(size_cin, cin);
    }
    return true;
  }
  return false;
}

bool RepairDesign::getCin(const sta::Pin* drvr_pin, float& cin)
{
  sta::Instance* inst = network_->instance(drvr_pin);
  sta::LibertyCell* cell = network_->libertyCell(inst);
  cin = 0;
  if (!network_->isTopLevelPort(drvr_pin) && cell != nullptr
      && resizer_->isLogicStdCell(inst)) {
    sta::LibertyCellPortIterator port_iter(cell);
    int nports = 0;
    while (port_iter.hasNext()) {
      const sta::LibertyPort* port = port_iter.next();
      if (!port->isPwrGnd() && port->direction() == PortDirection::input()) {
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

static float bufferCin(const sta::LibertyCell* cell)
{
  sta::LibertyPort *a, *y;
  cell->bufferPorts(a, y);
  return a->capacitance();
}

void RepairDesign::findBufferSizes()
{
  resizer_->findFastBuffers();
  buffer_sizes_.clear();
  buffer_sizes_ = {resizer_->buffer_fast_sizes_.begin(),
                   resizer_->buffer_fast_sizes_.end()};
  std::ranges::sort(buffer_sizes_,
                    [=](sta::LibertyCell* a, sta::LibertyCell* b) {
                      return bufferCin(a) < bufferCin(b);
                    });
}

/// Gain buffering: Make a buffer tree to satisfy fanout and cap constraints
///
/// Purpose: Reduce fanout count and total loading capacitance.
///
/// @param net The net to be buffered
/// @param drvr_pin The driver pin of the net
/// @param max_fanout Maximum fanout allowed per buffer (gain buffering
/// constraint)
///
/// Algorithm:
///   1. Sinks are collected and sorted by timing criticality (required time).
///   2. Sinks are grouped based on driving cell's max fanout and max buffer
///      load capacity (e.g., 9 * input cap of the largest buffer).
///   3. Insert the smallest buffer satisfying the following criteria.
///      "buffer_input_cap > accumulated_load_cap / gain ratio"
///   4. The new buffer's input is treated as a new sink and re-added to the
///      queue to recursively build the tree.
///   5. Incremental timing update.
///
/// Why use required time instead of slack?
/// - Arrival times change as the buffer tree is built, while required time
///   does not change. So required times is a more stable metric for bottom-up
///   construction and critical path isolation.
///
bool RepairDesign::performGainBuffering(sta::Net* net,
                                        const sta::Pin* drvr_pin,
                                        int max_fanout)
{
  struct EnqueuedPin
  {
    sta::Pin* pin;
    sta::Path* required_path;
    sta::Delay required_delay;
    int level;

    sta::Required required(const StaState*) const
    {
      if (required_path == nullptr) {
        return INF;
      }
      return required_path->required() - required_delay;
    }

    std::pair<sta::Required, int> sort_label(const StaState* sta) const
    {
      return std::make_pair(required(sta), -level);
    }

    float capacitance(const sta::Network* network)
    {
      return network->libertyPort(pin)->capacitance();
    }
  };

  class PinRequiredHigher
  {
   private:
    const sta::Network* network_;

   public:
    PinRequiredHigher(const sta::Network* network) : network_(network) {}

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

  // 1. Collect all sinks
  std::vector<EnqueuedPin> sinks;

  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (pin != drvr_pin && !network_->isTopLevelPort(pin)
        && network_->direction(pin) == PortDirection::input()
        && network_->libertyPort(pin)) {
      sta::Instance* inst = network_->instance(pin);
      if (!resizer_->dontTouch(inst)) {
        sta::Vertex* vertex = graph_->pinLoadVertex(pin);
        sta::Path* req_path
            = sta_->vertexWorstSlackPath(vertex, sta::MinMax::max());
        sinks.push_back({const_cast<sta::Pin*>(pin), req_path, 0.0, 0});
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
  std::vector<sta::Vertex*> tree_boundary;

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
  std::ranges::sort(sinks, PinRequiredHigher(network_));

  // 2. Iterate until we satisfy both the gain condition and max_fanout
  //    on drvr_pin
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
    auto buf_cell = buffer_sizes_.begin();
    for (; buf_cell != buffer_sizes_.end() - 1; buf_cell++) {
      if (bufferCin(*buf_cell)
          > load_acc / resizer_->buffer_sizing_cap_ratio_) {
        break;
      }
    }

    if (bufferCin(*buf_cell) >= 0.9f * load_acc) {
      // We are getting dimishing returns on inserting a buffer, stop
      // the algorithm here (we might have been called with a low gain value)
      break;
    }

    sta::LibertyPort *size_in, *size_out;
    (*buf_cell)->bufferPorts(size_in, size_out);
    int max_level = 0;
    sta::Pin* new_input_pin = nullptr;

    // 3. Insert a new buffer
    sta::PinSet group_set(db_network_);
    for (auto it = sinks.begin(); it != group_end; it++) {
      group_set.insert(it->pin);
      max_level = std::max(it->level, max_level);
      if (it->level == 0) {
        tree_boundary.push_back(graph_->pinLoadVertex(it->pin));
      }
    }

    sta::Instance* inst = resizer_->insertBufferBeforeLoads(
        net, &group_set, *buf_cell, nullptr, "gain");
    if (inst) {
      repaired_net = true;
      inserted_buffer_count_++;
      sta::Pin* buffer_op_pin = nullptr;
      resizer_->getBufferPins(inst, new_input_pin, buffer_op_pin);
    }

    // 4. New buffer input pin is enqueued as a new sink
    sta::Delay buffer_delay = resizer_->bufferDelay(
        *buf_cell, load_acc, resizer_->tgt_slew_dcalc_ap_);

    auto new_pin = EnqueuedPin{new_input_pin,
                               (group_end - 1)->required_path,
                               (group_end - 1)->required_delay + buffer_delay,
                               max_level + 1};

    sinks.erase(sinks.begin(), group_end);
    sinks.insert(
        std::ranges::upper_bound(sinks, new_pin, PinRequiredHigher(network_)),
        new_pin);

    load -= load_acc;
    load += size_in->capacitance();
  }

  // 5. Incremental timing update
  sta_->ensureLevelized();
  sta::Level max_level = 0;
  for (auto vertex : tree_boundary) {
    max_level = std::max(vertex->level(), max_level);
  }
  sta_->findDelays(max_level);
  search_->findArrivals(max_level);

  return repaired_net;
}

void RepairDesign::checkDriverArcSlew(const sta::Corner* corner,
                                      const sta::Instance* inst,
                                      const TimingArc* arc,
                                      float load_cap,
                                      float limit,
                                      float& violation)
{
  const sta::DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
  const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
  sta::GateTimingModel* model
      = dynamic_cast<sta::GateTimingModel*>(arc->model());
  sta::Pin* in_pin = network_->findPin(inst, arc->from()->name());

  if (model && in_pin) {
    const bool use_ideal_clk_slew
        = arc->set()->role()->genericRole() == TimingRole::regClkToQ()
          && clk_network_->isIdealClock(in_pin);
    sta::Slew in_slew
        = use_ideal_clk_slew
              ? clk_network_->idealClkSlew(
                    in_pin, in_rf, dcalc_ap->slewMinMax())
              : graph_->slew(
                    graph_->pinLoadVertex(in_pin), in_rf, dcalc_ap->index());
    const sta::Pvt* pvt = dcalc_ap->operatingConditions();

    sta::ArcDelay arc_delay;
    sta::Slew arc_slew;
    model->gateDelay(pvt, in_slew, load_cap, false, arc_delay, arc_slew);

    if (arc_slew > limit) {
      violation = max(arc_slew - limit, violation);
    }
  }
}

// Repair max slew violation at a driver pin: Find the smallest
// size which fits max slew; if none can be found, at least pick
// the size for which the slew is lowest
bool RepairDesign::repairDriverSlew(const sta::Corner* corner,
                                    const sta::Pin* drvr_pin)
{
  sta::Instance* inst = network_->instance(drvr_pin);
  sta::LibertyCell* cell = network_->libertyCell(inst);

  float load_cap;
  estimate_parasitics_->ensureWireParasitic(drvr_pin);
  load_cap
      = graph_delay_calc_->loadCap(drvr_pin, corner->findDcalcAnalysisPt(max_));

  if (!network_->isTopLevelPort(drvr_pin) && !resizer_->dontTouch(inst) && cell
      && resizer_->isLogicStdCell(inst)) {
    sta::LibertyCellSeq equiv_cells = resizer_->getSwappableCells(cell);
    if (!equiv_cells.empty()) {
      // Pair of slew violation magnitude and cell pointer
      using SizeCandidate = std::pair<float, sta::LibertyCell*>;
      std::vector<SizeCandidate> sizes;

      for (sta::LibertyCell* size_cell : equiv_cells) {
        float limit, violation = 0;
        bool limit_exists = false;
        sta::LibertyPort* port
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
                checkDriverArcSlew(
                    corner, inst, arc, load_cap, limit_w_margin, violation);
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

      std::ranges::sort(sizes, [](SizeCandidate a, SizeCandidate b) {
        if (a.first == 0 && b.first == 0) {
          // both sizes non-violating: sort by area
          return a.second->area() < b.second->area();
        }
        return a.first < b.first;
      });

      sta::LibertyCell* selected_size = sizes.front().second;
      if (selected_size != cell) {
        return resizer_->replaceCell(inst, selected_size, true);
      }
    }
  }

  return false;
}

void RepairDesign::repairDriver(sta::Vertex* drvr,
                                bool check_slew,
                                bool check_cap,
                                bool check_fanout,
                                int max_length,  // dbu
                                bool resize_drvr,
                                sta::Corner* corner_w_load_slew_viol,
                                int& repaired_net_count,
                                int& slew_violations,
                                int& cap_violations,
                                int& fanout_violations,
                                int& length_violations)
{
  sta::Pin* drvr_pin = drvr->pin();
  sta::Net* net = db_network_->findFlatNet(drvr_pin);
  if (!net) {
    return;
  }
  odb::dbNet* net_db = db_network_->staToDb(net);
  bool debug = (drvr_pin == resizer_->debug_pin_);
  if (debug) {
    logger_->setDebugLevel(RSZ, "repair_net", 3);
  }
  // Don't check okToBufferNet here as we are going to do a mix of driver
  // sizing and buffering.  Further checks exist in repairNet.
  if (net && !resizer_->dontTouch(net) && !net_db->isConnectedByAbutment()
      && !sta_->isClock(drvr_pin)
      // Exclude tie hi/low cells and supply nets.
      && !drvr->isConstant()) {
    repairNet(net,
              drvr_pin,
              drvr,
              check_slew,
              check_cap,
              check_fanout,
              max_length,
              resize_drvr,
              corner_w_load_slew_viol,
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

void RepairDesign::repairNet(sta::Net* net,
                             const sta::Pin* drvr_pin,
                             sta::Vertex* drvr,
                             bool check_slew,
                             bool check_cap,
                             bool check_fanout,
                             int max_length,  // dbu
                             bool resize_drvr,
                             sta::Corner* corner_w_load_slew_viol,
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
    const sta::Corner* corner = sta_->cmdCorner();
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

    float max_cap = INF;
    bool repair_cap = false, repair_load_slew = false, repair_wire = false;

    estimate_parasitics_->ensureWireParasitic(drvr_pin, net);
    graph_delay_calc_->findDelays(drvr);

    if (check_slew) {
      bool slew_violation = false;

      // First repair driver slew -- addressed by resizing the driver,
      // and if that doesn't fix it fully, by inserting buffers
      float slew1, max_slew1, slew_slack1;
      const sta::Corner* corner1;
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
          estimate_parasitics_->updateParasitics();
          sta_->findDelays(drvr);
          checkSlew(drvr_pin, slew1, max_slew1, slew_slack1, corner1);
        }

        // Slew violation persists after resizing the driver, derive
        // the max cap we need to apply to remove the slew violation
        if (slew_slack1 < 0.0f) {
          sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
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
        } else if (corner_w_load_slew_viol) {
          // There's a violation hidden by an annotation. Repair still
          slew_violation = true;
          repair_load_slew = true;
          if (!repair_cap) {
            corner = corner_w_load_slew_viol;
          }
        }
      }

      if (slew_violation) {
        slew_violations++;
      }
    }

    if (check_cap && !resizer_->isTristateDriver(drvr_pin)) {
      // Check that the max cap limit specified is within the bounds of reason.
      pre_checks_->checkCapLimit(drvr_pin);
      if (needRepairCap(drvr_pin, cap_violations, max_cap, corner)) {
        repair_cap = true;
      }
    }

    // For tristate nets all we can do is resize the driver.
    if (!resizer_->isTristateDriver(drvr_pin)) {
      BufferedNetPtr bnet = resizer_->makeBufferedNet(drvr_pin, corner);

      if (!bnet) {
        // Create a Steiner bnet in case we haven't selected a source of
        // parasitics
        bnet = resizer_->makeBufferedNetSteiner(drvr_pin, corner);
      }

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

bool RepairDesign::needRepairCap(const sta::Pin* drvr_pin,
                                 int& cap_violations,
                                 float& max_cap,
                                 const sta::Corner*& corner)
{
  float cap1, max_cap1, cap_slack1;
  const sta::Corner* corner1;
  const sta::RiseFall* tr1;
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

void RepairDesign::checkSlew(const sta::Pin* drvr_pin,
                             // Return values.
                             sta::Slew& slew,
                             float& limit,
                             float& slack,
                             const sta::Corner*& corner)
{
  slack = INF;
  limit = INF;
  corner = nullptr;

  const sta::Corner* corner1;
  const sta::RiseFall* tr1;
  sta::Slew slew1;
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

float RepairDesign::bufferInputMaxSlew(sta::LibertyCell* buffer,
                                       const sta::Corner* corner) const
{
  sta::LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  return resizer_->maxInputSlew(input, corner);
}

// Find the output port load capacitance that results in slew.
double RepairDesign::findSlewLoadCap(sta::LibertyPort* drvr_port,
                                     double slew,
                                     const sta::Corner* corner)
{
  const sta::DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
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
double RepairDesign::gateSlewDiff(sta::LibertyPort* drvr_port,
                                  double load_cap,
                                  double slew,
                                  const sta::DcalcAnalysisPt* dcalc_ap)
{
  sta::ArcDelay delays[sta::RiseFall::index_count];
  sta::Slew slews[sta::RiseFall::index_count];
  resizer_->gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  sta::Slew gate_slew = max(slews[sta::RiseFall::riseIndex()],
                            slews[sta::RiseFall::fallIndex()]);
  return gate_slew - slew;
}

////////////////////////////////////////////////////////////////

void RepairDesign::repairNet(const BufferedNetPtr& bnet,
                             const sta::Pin* drvr_pin,
                             float max_cap,
                             int max_length,  // dbu
                             const sta::Corner* corner)
{
  drvr_pin_ = drvr_pin;
  max_cap_ = max_cap;
  max_length_ = max_length;
  corner_ = corner;

  if (graphics_) {
    sta::Net* net = db_network_->net(drvr_pin);
    odb::dbNet* db_net = db_network_->staToDb(net);
    graphics_->repairNetStart(bnet, db_net);
  }

  int wire_length;
  sta::PinSeq load_pins;
  repairNet(bnet, 0, wire_length, load_pins);

  if (graphics_) {
    graphics_->repairNetDone();
  }
}

// Repair dispatch
//
// As we make our way up through the tree, we propagate the local slew limit
// held in `bnet->maxLoadSlew()`. This is the effective limit at the `bnet`
// node. The rules of propagation are made simple by the additive nature of
// Elmore approximation, and they are the following:
//
//  load -- limit seeded from pin limit
//  junction -- limit is the min over limit on the branches
//  wire -- limit is the downstream limit minus Elmore contribution
//          of the wire, which is `r_wire * (C_wire / 2 + C_downstream)`
//  via -- limit is the downstream limit minus `r_via * C_downstream`
//
// Once the limit dips below the would-be driver pin slew if we hypothetically
// connected the driver directly to `bnet`, we know we need to insert a buffer.
//
void RepairDesign::repairNet(const BufferedNetPtr& bnet,
                             int level,
                             // Return values.
                             // Remaining parasiics after repeater insertion.
                             int& wire_length,  // dbu
                             sta::PinSeq& load_pins)
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
    case BufferedNetType::via:
      repairNetVia(bnet, level, wire_length, load_pins);
      break;
    case BufferedNetType::buffer:
      logger_->critical(RSZ, 72, "unhandled BufferedNet type");
      break;
  }
}

void RepairDesign::repairNetVia(const BufferedNetPtr& bnet,
                                int level,
                                // Return values.
                                // Remaining parasiics after repeater insertion.
                                int& wire_length,  // dbu
                                sta::PinSeq& load_pins)
{
  repairNet(bnet->ref(), level + 1, wire_length, load_pins);
  bnet->setCapacitance(bnet->ref()->cap());
  bnet->setFanout(bnet->ref()->fanout());
  float r_via = bnet->viaResistance(corner_, resizer_, estimate_parasitics_);
  assert(slew_rc_factor_.has_value());
  bnet->setMaxLoadSlew(bnet->ref()->maxLoadSlew()
                       - (r_via * bnet->ref()->cap() * (*slew_rc_factor_)));
}

void RepairDesign::repairNetWire(
    const BufferedNetPtr& bnet,
    int level,
    // Return values.
    // Remaining parasiics after repeater insertion.
    int& wire_length,  // dbu
    sta::PinSeq& load_pins)
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
  float max_load_slew = bnet->ref()->maxLoadSlew();
  float max_load_slew_margined = maxSlewMargined(max_load_slew);

  odb::Point to_loc = bnet->ref()->location();
  int to_x = to_loc.getX();
  int to_y = to_loc.getY();
  odb::Point from_loc = bnet->location();
  int length = odb::Point::manhattanDistance(from_loc, to_loc);
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
  bnet->wireRC(corner_, resizer_, estimate_parasitics_, wire_res, wire_cap);
  // ref_cap includes ref's wire cap
  double ref_cap = bnet->ref()->cap();
  double load_cap = length1 * wire_cap + ref_cap;

  // Calculate estimated slew based on Elmore
  float r_drvr = resizer_->driveResistance(drvr_pin_);

  // For top ports without a specified input drive, r_drvr is zero
  // which can make us miss the buffer insertion point. Clip r_drvr
  // to be no smaller than the drive resistance of the beefiest buffer
  // to address this.
  r_drvr = std::max(r_drvr, r_strongest_buffer_);

  double r_wire = length1 * wire_res;
  double c_wire = length1 * wire_cap;

  assert(slew_rc_factor_.has_value());
  double load_slew
      = (r_drvr * (c_wire + ref_cap) + r_wire * ref_cap + r_wire * c_wire / 2)
        * (*slew_rc_factor_);

  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}load_slew={} r_drvr={} max_load_slew={} r_wire={} "
             "ref_cap={} layer={} wire_res={}",
             "",
             level,
             delayAsString(load_slew, this, 3),
             units_->resistanceUnit()->asString(r_drvr, 3),
             delayAsString(bnet->ref()->maxLoadSlew(), this, 3),
             r_wire,
             ref_cap,
             bnet->layer(),
             wire_res);

  sta::LibertyCell* buffer_cell = resizer_->findTargetCell(
      resizer_->buffer_lowest_drive_, load_cap, false);

  bnet->setCapacitance(load_cap);
  bnet->setFanout(bnet->ref()->fanout());
  bnet->setMaxLoadSlew(
      bnet->ref()->maxLoadSlew()
      - (r_wire * (c_wire / 2 + ref_cap) * (*slew_rc_factor_)));

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
      split_length = min(split_length,
                         max(metersToDbu((max_cap_ - ref_cap) / wire_cap), 0));
      split_wire = true;
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
      // We are inserting a buffer to cut this wire segment short.
      // The slew at the end of the wire segment is a quadratic polynomial
      // in terms of the wire segment's length (in Elmore approx.).
      //
      // We solve a quadratic eq. to find the maximum conforming length.
      float a = wire_res * wire_cap / 2;
      float b = (r_drvr * wire_cap) + (wire_res * ref_cap);
      float c
          = (r_drvr * ref_cap) - (max_load_slew_margined / (*slew_rc_factor_));
      float l = 0.0;
      if (a > 1e-12) {  // Quadratic case
        const float discriminant = b * b - 4 * a * c;
        if (discriminant >= 0.0) {
          l = (-b + sqrt(discriminant)) / (2 * a);
        }
      } else if (b > 1e-12) {
        // a * l^2 + b * l + c = 0 becomes
        // b * l + c = 0 when a is very small
        l = -c / b;
      }
      if (l >= 0.0) {
        split_length = min(split_length, metersToDbu(l));
      } else {
        split_length = 0;
      }
      split_wire = true;
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
                        odb::Point(buf_x, buf_y),
                        buffer_cell,
                        /* resize= */ true,
                        level,
                        load_pins,
                        repeater_cap,
                        repeater_fanout,
                        max_load_slew)) {
        debugPrint(logger_,
                   RSZ,
                   "repair_net",
                   3,
                   "{:{}s}makeRepeater failed"
                   "",
                   level);
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
      max_load_slew_margined = maxSlewMargined(max_load_slew);
      r_wire = length1 * wire_res;
      c_wire = length1 * wire_cap;
      load_slew = (r_drvr * (c_wire + ref_cap) + r_wire * ref_cap
                   + r_wire * c_wire / 2)
                  * (*slew_rc_factor_);
      buffer_cell = resizer_->findTargetCell(
          resizer_->buffer_lowest_drive_, load_cap, false);

      bnet->setCapacitance(load_cap);
      bnet->setFanout(repeater_fanout);
      bnet->setMaxLoadSlew(
          max_load_slew
          - (r_wire * (c_wire / 2 + ref_cap) * (*slew_rc_factor_)));

      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 3,
                 "{:{}s}l={} post buffer slew={}",
                 "",
                 level,
                 units_->distanceUnit()->asString(length1, 1),
                 delayAsString(load_slew, this, 3));
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
    sta::PinSeq& load_pins)
{
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}{}",
             "",
             level,
             bnet->to_string(resizer_));
  odb::Point loc = bnet->location();

  BufferedNetPtr left = bnet->ref();
  int wire_length_left = 0;
  sta::PinSeq loads_left;
  repairNet(left, level + 1, wire_length_left, loads_left);
  float cap_left = left->cap();
  float fanout_left = left->fanout();
  float max_load_slew_left = left->maxLoadSlew();

  BufferedNetPtr right = bnet->ref2();
  int wire_length_right = 0;
  sta::PinSeq loads_right;
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

  wire_length = std::max(wire_length_left, wire_length_right);
  float load_cap = cap_left + cap_right;
  float max_load_slew = min(max_load_slew_left, max_load_slew_right);
  float max_load_slew_margined = maxSlewMargined(max_load_slew);
  sta::LibertyCell* buffer_cell = resizer_->findTargetCell(
      resizer_->buffer_lowest_drive_, load_cap, false);

  // Check for violations when the left/right branches are combined.
  // Add a buffer to left or right branch to stay under the max
  // cap/length/fanout.
  bool repeater_left = false;
  bool repeater_right = false;

  // Calculate estimated slew based on RC
  float r_drvr = resizer_->driveResistance(drvr_pin_);
  assert(slew_rc_factor_.has_value());
  float load_slew = r_drvr * load_cap * (*slew_rc_factor_);
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
    double slew_left = r_drvr * cap_left * (*slew_rc_factor_);
    double slew_slack_left = maxSlewMargined(max_load_slew_left) - slew_left;
    double slew_right = r_drvr * cap_right * (*slew_rc_factor_);
    double slew_slack_right = maxSlewMargined(max_load_slew_right) - slew_right;
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
  wire_length = std::max(wire_length_left, wire_length_right);

  bnet->setCapacitance(cap_left + cap_right);
  bnet->setFanout(fanout_right + fanout_left);
  bnet->setMaxLoadSlew(min(max_load_slew_left, max_load_slew_right));

  // Union left/right load pins.
  for (const sta::Pin* load_pin : loads_left) {
    load_pins.push_back(load_pin);
  }
  for (const sta::Pin* load_pin : loads_right) {
    load_pins.push_back(load_pin);
  }
}

void RepairDesign::repairNetLoad(
    const BufferedNetPtr& bnet,
    int level,
    // Return values.
    // Remaining parasiics after repeater insertion.
    int& wire_length,  // dbu
    sta::PinSeq& load_pins)
{
  debugPrint(logger_,
             RSZ,
             "repair_net",
             3,
             "{:{}s}{}",
             "",
             level,
             bnet->to_string(resizer_));
  const sta::Pin* load_pin = bnet->loadPin();
  debugPrint(logger_,
             RSZ,
             "repair_net",
             2,
             "{:{}s}load {}",
             "",
             level,
             sdc_network_->pathName(load_pin));
  wire_length = 0;

  // Check that the slew limit specified is within the bounds of reason.
  pre_checks_->checkSlewLimit(bnet->cap(), bnet->maxLoadSlew());

  load_pins.push_back(load_pin);
}

////////////////////////////////////////////////////////////////

LoadRegion::LoadRegion() = default;

LoadRegion::LoadRegion(sta::PinSeq& pins, odb::Rect& bbox)
    : pins_(pins), bbox_(bbox)
{
}

LoadRegion RepairDesign::findLoadRegions(const sta::Net* net,
                                         const sta::Pin* drvr_pin,
                                         int max_fanout)
{
  sta::PinSeq loads = findLoads(drvr_pin);
  odb::Rect bbox = findBbox(loads);
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
      region.regions_[0].bbox_ = odb::Rect(x_min, y_min, x_mid, y_max);
      region.regions_[1].bbox_ = odb::Rect(x_mid, y_min, x_max, y_max);
      cut = odb::Line{x_mid, y_min, x_mid, y_max};
      horz_partition = true;
    } else {
      region.regions_[0].bbox_ = odb::Rect(x_min, y_min, x_max, y_mid);
      region.regions_[1].bbox_ = odb::Rect(x_min, y_mid, x_max, y_max);
      horz_partition = false;
      cut = odb::Line{x_min, y_mid, x_max, y_mid};
    }
    for (const sta::Pin* pin : region.pins_) {
      odb::Point loc = db_network_->location(pin);
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
                                       const sta::Pin* drvr_pin,
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

    sta::PinSeq repeater_inputs;
    sta::PinSeq repeater_loads;
    for (LoadRegion& sub : region.regions_) {
      sta::PinSeq& sub_pins = sub.pins_;
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

    for (const sta::Pin* pin : repeater_inputs) {
      region.pins_.push_back(pin);
    }
  }
}

void RepairDesign::makeFanoutRepeater(sta::PinSeq& repeater_loads,
                                      sta::PinSeq& repeater_inputs,
                                      const odb::Rect& bbox,
                                      const odb::Point& loc,
                                      bool check_slew,
                                      bool check_cap,
                                      int max_length,
                                      bool resize_drvr)
{
  float ignore2, ignore3, ignore4;
  sta::Net* out_net;
  sta::Pin *repeater_in_pin, *repeater_out_pin;
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
  sta::Vertex* repeater_out_vertex = graph_->pinDrvrVertex(repeater_out_pin);
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
            nullptr,
            repaired_net_count,
            slew_violations,
            cap_violations,
            fanout_violations,
            length_violations);
  repeater_inputs.push_back(repeater_in_pin);
  repeater_loads.clear();
}

odb::Rect RepairDesign::findBbox(sta::PinSeq& pins)
{
  odb::Rect bbox;
  bbox.mergeInit();
  for (const sta::Pin* pin : pins) {
    odb::Point loc = db_network_->location(pin);
    odb::Rect r(loc.x(), loc.y(), loc.x(), loc.y());
    bbox.merge(r);
  }
  return bbox;
}

sta::PinSeq RepairDesign::findLoads(const sta::Pin* drvr_pin)
{
  sta::PinSeq loads;
  sta::Pin* drvr_pin1 = const_cast<sta::Pin*>(drvr_pin);
  sta::PinSeq drvrs;
  sta::PinSet visited_drvrs(db_network_);
  sta::FindNetDrvrLoads visitor(
      drvr_pin1, visited_drvrs, loads, drvrs, network_);
  network_->visitConnectedPins(drvr_pin1, visitor);
  return loads;
}

odb::Point RepairDesign::findClosedPinLoc(const sta::Pin* drvr_pin,
                                          sta::PinSeq& pins)
{
  odb::Point drvr_loc = db_network_->location(drvr_pin);
  odb::Point closest_pt = drvr_loc;
  int64_t closest_dist = std::numeric_limits<int64_t>::max();
  for (const sta::Pin* pin : pins) {
    odb::Point loc = db_network_->location(pin);
    int64_t dist = odb::Point::manhattanDistance(loc, drvr_loc);
    if (dist < closest_dist) {
      closest_pt = loc;
      closest_dist = dist;
    }
  }
  return closest_pt;
}

bool RepairDesign::isRepeater(const sta::Pin* load_pin)
{
  odb::dbInst* db_inst = db_network_->staToDb(network_->instance(load_pin));
  odb::dbSourceType source = db_inst->getSourceType();
  return source == odb::dbSourceType::TIMING;
}

////////////////////////////////////////////////////////////////

bool RepairDesign::makeRepeater(const char* reason,
                                const odb::Point& loc,
                                sta::LibertyCell* buffer_cell,
                                bool resize,
                                int level,
                                // Return values.
                                sta::PinSeq& load_pins,
                                float& repeater_cap,
                                float& repeater_fanout,
                                float& repeater_max_slew)
{
  sta::Net* out_net;
  sta::Pin *repeater_in_pin, *repeater_out_pin;
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

bool RepairDesign::hasInputPort(const sta::Net* net)
{
  bool has_top_level_port = false;
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
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
    sta::LibertyCell* buffer_cell,
    bool resize,
    int level,
    // Return values.
    sta::PinSeq& load_pins,  // inout, read, reset, repopulated.
    float& repeater_cap,
    float& repeater_fanout,
    float& repeater_max_slew,
    sta::Net*& out_net,
    sta::Pin*& repeater_in_pin,
    sta::Pin*& repeater_out_pin)
{
  debugPrint(logger_,
             RSZ,
             "repair_net",
             2,
             "{:{}s}{} {} {} ({} {})",
             "",
             level,
             reason,
             reason,
             buffer_cell->name(),
             units_->distanceUnit()->asString(dbuToMeters(x), 1),
             units_->distanceUnit()->asString(dbuToMeters(y), 1));

  // Insert buffer
  odb::Point loc(x, y);
  sta::Instance* buffer = resizer_->insertBufferBeforeLoads(
      nullptr, &load_pins, buffer_cell, &loc, reason);
  if (!buffer) {
    return false;
  }

  inserted_buffer_count_++;

  odb::dbInst* new_buffer = db_network_->staToDb(buffer);
  out_net = db_network_->dbToSta(new_buffer->getFirstOutput()->getNet());

  if (graphics_) {
    odb::dbInst* db_inst = db_network_->staToDb(buffer);
    graphics_->makeBuffer(db_inst);
  }

  sta::LibertyPort* buffer_input_port;
  sta::LibertyPort* buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

  // Resize repeater as we back up by levels.
  if (resize) {
    sta::Pin* buffer_out_pin = network_->findPin(buffer, buffer_output_port);
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

sta::LibertyCell* RepairDesign::findBufferUnderSlew(float max_slew,
                                                    float load_cap)
{
  sta::LibertyCell* min_slew_buffer = resizer_->buffer_lowest_drive_;
  float min_slew = INF;
  sta::LibertyCellSeq swappable_cells
      = resizer_->getSwappableCells(resizer_->buffer_lowest_drive_);
  if (!swappable_cells.empty()) {
    sort(swappable_cells,
         [this](const sta::LibertyCell* buffer1,
                const sta::LibertyCell* buffer2) {
           return resizer_->bufferDriveResistance(buffer1)
                  > resizer_->bufferDriveResistance(buffer2);
         });
    for (sta::LibertyCell* buffer : swappable_cells) {
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

    debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
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

float RepairDesign::getSlewRCFactor()
{
  if (!slew_rc_factor_.has_value()) {
    init();
  }
  assert(slew_rc_factor_.has_value());
  return *slew_rc_factor_;
}

}  // namespace rsz
