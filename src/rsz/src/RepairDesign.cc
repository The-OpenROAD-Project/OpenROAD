// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "RepairDesign.hh"

#include <algorithm>
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
#include <fstream>
#include <filesystem>
#include "BufferedTree.h"
#include "BufferedNet.hh"
#include "ResizerObserver.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

#include "rsz/Resizer.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh" 
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/RiseFallValues.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
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
using sta::Port;
using sta::PortDirection;
using sta::TimingArc;
using sta::TimingArcSet;
using sta::TimingRole;
using sta::VertexInEdgeIterator;
using std::string;
using std::vector;
using std::shared_ptr;
using std::pair;
using BufferedTreePtr = std::shared_ptr<BufferedTree>;
using odb::dbBlock;
using odb::dbInst;
using odb::dbNet;

RepairDesign::RepairDesign(Resizer* resizer) 
  : resizer_(resizer)
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
}

const sta::Pin* findDriverPin(rsz::Resizer* rsz, sta::Net* net) {
  sta::Network* network = rsz->network();
  std::unique_ptr<sta::NetPinIterator> it(network->pinIterator(net));
  while (it->hasNext()) {
    const sta::Pin* pin = it->next();
    if (network->isDriver(pin))
      return pin;
  }
  return nullptr;
  // If you already have a util for this, use it instead.
  //             // or rsz->pinIterator(net) / network->pinIterator(net)
                           // or network_->isDriver(pin)
}

void RepairDesign::unitWireRC(double& r_per_m, double& c_per_m) const
{
  r_per_m = 0.0;
  c_per_m = 0.0;
  if (estimate_parasitics_) {
    // Per-meter R and C for signal nets (Ohm/m and F/m)
    estimate_parasitics_->wireSignalRC(corner_, r_per_m, c_per_m);
  }
}


void RepairDesign::annotateWireDelaysOnTrees(std::vector<BufferedTreePtr>& trees)
{
  double r_per_m = 0.0, c_per_m = 0.0;
  unitWireRC(r_per_m, c_per_m);
  if (r_per_m <= 0.0 || c_per_m <= 0.0) return;

  for (auto& tree : trees) {
    if (!tree || !tree->getRoot()) continue;

    std::queue<BufferedTreeNodePtr> q;
    q.push(tree->getRoot());
    while (!q.empty()) {
      auto node = q.front(); q.pop();
      for (auto& child : node->children) {
        const int L_dbu = Point::manhattanDistance({node->x, node->y}, {child->x, child->y});
        const double L_m = dbuToMeters(L_dbu);
        const double t_ps = 0.5 * r_per_m * c_per_m * L_m * L_m * 1e12; // Elmore
        child->net_wire_delay_ps = t_ps;
        q.push(child);
      }
    }
  }
}

sta::Parasitics* RepairDesign::getStaParasitics() { return sta_ ? sta_->parasitics() : nullptr; }
//Functions added by Cshah for wire RC extraction

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
               false,
               verbose,
               repaired_net_count,
               slew_violations,
               cap_violations,
               fanout_violations,
               length_violations,
               store_buffered_trees_flag_
              );

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
  std::set<std::pair<Vertex*, int>> slew_user_annotated;

  // We need to override slews in order to get good required time estimates.
  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex* drvr = resizer_->level_drvr_vertices_[i];
    debugPrint(logger_,
               RSZ,
               "early_sizing",
               2,
               "Annotating slew for driver {}",
               network_->pathName(drvr->pin()));
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
    debugPrint(logger_,
               RSZ,
               "early_sizing",
               2,
               "Processing driver {}",
               network_->pathName(drvr_pin));
    // Always get the flat net for the top level port.
    Net* net = network_->isTopLevelPort(drvr_pin)
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
        if (!slew_user_annotated.count(std::make_pair(drvr, rf->index()))) {
          const DcalcAnalysisPt* dcalc_ap
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
    double buffer_gain,
    bool initial_sizing,
    bool verbose,
    int& repaired_net_count,
    int& slew_violations,
    int& cap_violations,
    int& fanout_violations,
    int& length_violations,
    bool store_buffered_trees_flag_)
{
  if (store_buffered_trees_flag_) {
    repairDesign_update(max_wire_length,
                        slew_margin,
                        cap_margin,
                        buffer_gain,
                        verbose,
                        repaired_net_count,
                        slew_violations,
                        cap_violations,
                        fanout_violations,
                        length_violations);
    // YL: store buffered trees
    saveBufferedTrees();
    // std::cout << "--------------Save buffered trees to the csv file----------------" << std::endl;
    // std::cout << "Buffered Trees Pre Size (#net): " << buffered_trees_pre_.size() << std::endl;
    // std::cout << "Buffered Trees Post Size (#buffered net): " << buffered_trees_post_.size() << std::endl;
    // std::cout << "#buffered net (test): " << repaired_net_count << std::endl;
    // std::cout << "#buffer: " << inserted_buffer_count_ << std::endl;
    return;
  }

  init();
  slew_margin_ = slew_margin;
  cap_margin_ = cap_margin;
  buffer_gain_ = buffer_gain;
  bool gain_buffering = (buffer_gain_ != 0.0);

  slew_violations = 0;
  cap_violations = 0;
  fanout_violations = 0;
  length_violations = 0;
  repaired_net_count = 0;
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  resizer_->resized_multi_output_insts_.clear();

  std::set<std::pair<Vertex*, int>> slew_user_annotated;

  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();
  sta_->searchPreamble();
  search_->findAllArrivals();

  if (initial_sizing) {
    performEarlySizingRound(repaired_net_count);
  }

  // keep track of annotations which were added by us
  std::set<Vertex*> annotations_to_clean_up;
  std::map<Vertex*, Corner*> drvr_with_load_slew_viol;
  VertexSeq load_vertices = resizer_->orderedLoadPinVertices();

  // Forward pass: whenever we see violating input pin slew we override
  // it in the graph. This is in order to prevent second order upsizing.
  // The load pin slew will get repaired so we shouldn't propagate it
  // downstream.
  for (auto vertex : load_vertices) {
    if (!vertex->slewAnnotated()) {
      sta_->findDelays(vertex);
      LibertyPort* port = network_->libertyPort(vertex->pin());
      if (port) {
        for (auto corner : *sta_->corners()) {
          const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
          float limit = resizer_->maxInputSlew(port, corner);
          for (const RiseFall* rf : RiseFall::range()) {
            float actual = graph_->slew(vertex, rf, dcalc_ap->index());
            if (actual > limit) {
              sta_->setAnnotatedSlew(vertex,
                                     corner,
                                     max_->asMinMaxAll(),
                                     rf->asRiseFallBoth(),
                                     limit);
              annotations_to_clean_up.insert(vertex);
              PinSet* drivers = network_->drivers(vertex->pin());
              if (drivers) {
                for (const Pin* drvr_pin : *drivers) {
                  drvr_with_load_slew_viol[graph_->pinDrvrVertex(drvr_pin)]
                      = corner;
                }
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

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    int print_iteration = 0;
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
      // Don't check okToBufferNet here as we are going to do a mix of driver
      // sizing and buffering.  Further checks exist in repairNet.
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
                  drvr_with_load_slew_viol.count(drvr)
                      ? drvr_with_load_slew_viol.at(drvr)
                      : nullptr,
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
    estimate_parasitics_->updateParasitics();
    printProgress(print_iteration, true, true, repaired_net_count);
  }

  if (!annotations_to_clean_up.empty()) {
    for (auto vertex : annotations_to_clean_up) {
      for (auto corner : *sta_->corners()) {
        const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
        for (const RiseFall* rf : RiseFall::range()) {
          vertex->setSlewAnnotated(false, rf, dcalc_ap->index());
        }
      }
    }
    sta_->delaysInvalid();
  }

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
  buffer_gain_ = 0.0;

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
void RepairDesign::repairNet(Net* net,
                             double max_wire_length,  // meters
                             double slew_margin,
                             double cap_margin)
{
  init();
  slew_margin_ = slew_margin;
  cap_margin_ = cap_margin;
  buffer_gain_ = 0.0;

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
        if (port->direction() == PortDirection::input()) {
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
      if (port->direction() == PortDirection::input()) {
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
        && network_->direction(pin) == PortDirection::input()
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
      = bufferCin(buffer_sizes_.back()) * resizer_->buffer_sizing_cap_ratio_; //CShah: do we need to add buffer_gain_ here?

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

    Net* new_net = db_network_->makeNet(parent);
    dbNet* net_db = db_network_->staToDb(net);
    dbNet* new_net_db = db_network_->staToDb(new_net);
    new_net_db->setSigType(net_db->getSigType());

    const Point drvr_loc = db_network_->location(drvr_pin);

    // create instance in driver parent
    Instance* inst = resizer_->makeBuffer(*size, "gain", parent, drvr_loc);

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
    if (graphics_) {
      dbInst* db_inst = db_network_->staToDb(inst);
      graphics_->makeBuffer(db_inst);
    }

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
    const bool use_ideal_clk_slew
        = arc->set()->role()->genericRole() == TimingRole::regClkToQ()
          && clk_network_->isIdealClock(in_pin);
    Slew in_slew
        = use_ideal_clk_slew
              ? clk_network_->idealClkSlew(
                    in_pin, in_rf, dcalc_ap->slewMinMax())
              : graph_->slew(
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
  estimate_parasitics_->ensureWireParasitic(drvr_pin);
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
                             Corner* corner_w_load_slew_viol,
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

    float max_cap = INF;
    bool repair_cap = false, repair_load_slew = false, repair_wire = false;

    estimate_parasitics_->ensureWireParasitic(drvr_pin, net);
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
          graph_delay_calc_->findDelays(drvr);
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
bool RepairDesign::needRepairSlew(const Pin* drvr_pin,
                                  int& slew_violations,
                                  float& max_cap,
                                  const Corner*& corner)
{
  bool repair_slew = false;
  float slew1, slew_slack1, max_slew1;
  const Corner* corner1;
  // Check slew at the driver.
  checkSlew(drvr_pin, slew1, max_slew1, slew_slack1, corner1);
  // Max slew violations at the driver pin are repaired by reducing the
  // load capacitance. Wire resistance may shield capacitance from the
  // driver but so this is conservative.
  // Find max load cap that corresponds to max_slew.
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  if (corner1 && max_slew1 > 0.0) {
    if (drvr_port) {
      float max_cap1 = findSlewLoadCap(drvr_port, max_slew1, corner1);
      max_cap = min(max_cap, max_cap1);
    }
    corner = corner1;
    if (slew_slack1 < 0.0) {
      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 2,
                 "drvr slew violation slew={} max_slew={}",
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
  resizer_->checkLoadSlews(
      drvr_pin, slew_margin_, slew1, max_slew1, slew_slack1, corner1);
  if (slew_slack1 < 0.0) {
    debugPrint(logger_,
               RSZ,
               "repair_net",
               2,
               "load slew violation load_slew={} max_slew={}",
               delayAsString(slew1, this, 3),
               delayAsString(max_slew1, this, 3));
    corner = corner1;
    // Don't double count the driver/load on same net.
    if (!repair_slew) {
      slew_violations++;
    }
    repair_slew = true;
  }

  return repair_slew;
}

bool RepairDesign::needRepair(const Pin* drvr_pin,
                              const Corner*& corner,
                              const int max_length,
                              const int wire_length,
                              const bool check_cap,
                              const bool check_slew,
                              float& max_cap,
                              int& slew_violations,
                              int& cap_violations,
                              int& length_violations)
{
  bool repair_cap = false;
  bool repair_slew = false;
  if (check_cap) {
    repair_cap = needRepairCap(drvr_pin, cap_violations, max_cap, corner);
  }
  bool repair_wire = needRepairWire(max_length, wire_length, length_violations);
  if (check_slew) {
    repair_slew = needRepairSlew(drvr_pin, slew_violations, max_cap, corner);
  }

  return repair_cap || repair_wire || repair_slew;
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
  // std::cout << "[Check] truly enter repairNet "<<std::endl;
  drvr_pin_ = drvr_pin;
  max_cap_ = max_cap;
  max_length_ = max_length;
  corner_ = corner;

  if (graphics_) {
    Net* net = db_network_->net(drvr_pin);
    odb::dbNet* db_net = db_network_->staToDb(net);
    graphics_->repairNetStart(bnet, db_net);
  }

  int wire_length;
  PinSeq load_pins;
  repairNet(bnet, 0, wire_length, load_pins);

  if (graphics_) {
    graphics_->repairNetDone();
  }
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
    if(store_buffered_trees_flag_==true)
  {
    repairNetWire_update(bnet, level, wire_length, load_pins);
    return;
  }

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
  bnet->wireRC(corner_, resizer_, resizer_->getEstimateParasitics(), wire_res, wire_cap);
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
  if(store_buffered_trees_flag_==true)
  {
    repairNetJunc_update(bnet, level, wire_length, load_pins);
    return;
  }

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
  estimate_parasitics_->wireSignalRC(corner_, wire_res, wire_cap);

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

LoadRegion::LoadRegion(PinSeq& pins, odb::Rect& bbox) : pins_(pins), bbox_(bbox)
{
}

LoadRegion RepairDesign::findLoadRegions(const Net* net,
                                         const Pin* drvr_pin,
                                         int max_fanout)
{
  PinSeq loads = findLoads(drvr_pin);
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
                                      const odb::Rect& bbox,
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
            nullptr,
            repaired_net_count,
            slew_violations,
            cap_violations,
            fanout_violations,
            length_violations);
  repeater_inputs.push_back(repeater_in_pin);
  repeater_loads.clear();
}

odb::Rect RepairDesign::findBbox(PinSeq& pins)
{
  odb::Rect bbox;
  bbox.mergeInit();
  for (const Pin* pin : pins) {
    Point loc = db_network_->location(pin);
    odb::Rect r(loc.x(), loc.y(), loc.x(), loc.y());
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
  if (store_buffered_trees_flag_==true)
  {
    float drvr_output_cap = 0.0;
    float drvr_output_slew = 0.0;

    makeRepeater_update(reason, x, y, buffer_cell, resize, level,
                               load_pins, repeater_cap, repeater_fanout,
                               repeater_max_slew, out_net,
                               repeater_in_pin, repeater_out_pin, drvr_output_cap,
                               drvr_output_slew);
    return true;
  }
  // Free vars set by the lambdas


  Net* load_net = nullptr;
  dbNet* load_db_net = nullptr;  // load net, flat
  bool preserve_outputs = false;
  bool top_primary_output = false;
  bool connections_will_be_modified = false;
  bool keep_input;
  Instance* parent = nullptr;
  Pin* driver_pin = nullptr;
  PinSet repeater_load_pins(db_network_);
  Pin* buffer_ip_pin = nullptr;
  Pin* buffer_op_pin = nullptr;
  Instance* buffer = nullptr;

  //
  // Helper sub-functions, written as lambdas
  //

  /*
    Classify the load types in the load_pins
   */
  auto ClassifyLoadTypes = [&]() {
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
  };

  auto connectionsWillBeModified = [&]() {
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
  };

  auto determineParentToPutBufferIn = [&]() {
    // Determine parent to put buffer (and net)
    // Determine the driver pin (driver_pin)
    // Make the buffer in the root module in case or primary input connections

    if (hasInputPort(load_net) || top_primary_output
        || !db_network_->hasHierarchy()) {
      (void) (db_network_->getNetDriverParentModule(
          load_net, driver_pin, true));
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
  };

  // Debug routines, left in
  /*
  auto reportLoadPins = [&]() {
    static int debug;
    debug++;
    odb::dbITerm* iterm;
    odb::dbBTerm* bterm;
    odb::dbModBTerm* modbterm;
    odb::dbModITerm* moditerm;

    Net* driver_net_flat = (Net*) (db_network_->flatNet(driver_pin));
    Net* driver_net_hier = (Net*) (db_network_->hierNet(driver_pin));

    printf(
        "D:%d ++Make repeater entry: loads from driver %s (flat net: %s hier "
        "net %s)\n",
        debug,
        db_network_->name(driver_pin),
        driver_net_flat ? db_network_->name(driver_net_flat) : " none ",
        driver_net_hier ? db_network_->name(driver_net_hier) : " none ");

    for (const Pin* pin : load_pins) {
      db_network_->staToDb(pin, iterm, bterm, moditerm, modbterm);
      bool primary_port = (bterm != nullptr);
      dbNet* flat_net = db_network_->flatNet(pin);
      odb::dbModNet* hier_net = db_network_->hierNet(pin);
      printf("Pin %s(%s) (hier_net %s, flat_net %s)\n",
             db_network_->name(pin),
             primary_port ? "primary port" : "",
             hier_net ? hier_net->getName() : " none ",
             flat_net ? flat_net->getName().c_str() : " none ");
    }
    printf("--Make repeater entry: loads\n");
  };

  auto reportDriverPinConnections = [&]() {
    dbNet* driver_net_flat = db_network_->flatNet(driver_pin);
    odb::dbModNet* driver_net_hier = db_network_->hierNet(driver_pin);
    printf("+++ Driver Pin Connections\n");
    printf("Driver pin has flat net %s with %d iterms %d bterms \n",
           db_network_->name(driver_pin),
           driver_net_flat ? driver_net_flat->getITerms().size() : 0,
           driver_net_flat ? driver_net_flat->getBTerms().size() : 0);
    printf(
        "Driver pin has hier net %s with %d iterms %d bterms %d moditerms %d "
        "modbterms\n",
        db_network_->name(driver_pin),
        driver_net_hier ? driver_net_hier->getITerms().size() : 0,
        driver_net_hier ? driver_net_hier->getBTerms().size() : 0,
        driver_net_hier ? driver_net_hier->getModITerms().size() : 0,
        driver_net_hier ? driver_net_hier->getModBTerms().size() : 0);
    printf("-- Driver Pin Connections\n");
  };

  auto reportBufferConnections = [&]() {
    Net* ip_net_flat = (Net*) (db_network_->flatNet(buffer_ip_pin));
    Net* ip_net_hier = (Net*) (db_network_->hierNet(buffer_ip_pin));
    Net* op_net_flat = (Net*) (db_network_->flatNet(buffer_op_pin));
    Net* op_net_hier = (Net*) (db_network_->hierNet(buffer_op_pin));

    printf("+++ Buffer connections\n");
    printf("Buffer %s ip net-flat %s net-hier  %s op net-flat %s net-hier %s\n",
           db_network_->name(buffer),
           ip_net_flat ? db_network_->name(ip_net_flat) : " none",
           ip_net_hier ? db_network_->name(ip_net_hier) : " none",
           op_net_flat ? db_network_->name(op_net_flat) : " none",
           op_net_hier ? db_network_->name(op_net_hier) : " none");
    if (ip_net_flat) {
      printf("Flat ip net %s connected to %d iterms %d bterms\n",
             ((dbNet*) ip_net_flat)->getName().c_str(),
             ((dbNet*) ip_net_flat)->getITerms().size(),
             ((dbNet*) ip_net_flat)->getBTerms().size());

      printf("\t+++ Flat ip net iterms:\n");
      for (auto iterm : ((dbNet*) ip_net_flat)->getITerms()) {
        printf("\tIterm %s\n", iterm->getName('/').c_str());
      }
      printf("\t--- Flat ip net iterms:\n");
    }
    if (ip_net_hier) {
      printf(
          "Hier ip net %s connected to %d iterms %d bterms %d moditerms %d "
          "modbterms\n",
          ((odb::dbModNet*) ip_net_hier)->getName(),
          ((odb::dbModNet*) ip_net_hier)->getITerms().size(),
          ((odb::dbModNet*) ip_net_hier)->getBTerms().size(),
          ((odb::dbModNet*) ip_net_hier)->getModITerms().size(),
          ((odb::dbModNet*) ip_net_hier)->getModBTerms().size());
    }
    std::set<dbITerm*> op_net_flat_iterms;
    if (op_net_flat) {
      printf("Flat op net %s connected to %d iterms %d bterms\n",
             ((dbNet*) op_net_flat)->getName().c_str(),
             ((dbNet*) op_net_flat)->getITerms().size(),
             ((dbNet*) op_net_flat)->getBTerms().size());
      printf("\t+++ Flat op iterms\n");
      for (auto iterm : ((dbNet*) op_net_flat)->getITerms()) {
        op_net_flat_iterms.insert(iterm);
        printf("\tIterm %s\n", iterm->getName().c_str());
      }
      printf("\t--- Flat op iterms\n");

      for (auto iterm : ((dbNet*) ip_net_flat)->getITerms()) {
        if (network_->isDriver((Pin*) iterm))
          printf("flat ip net driver %s\n", iterm->getName('/').c_str());
        if (op_net_flat_iterms.find(iterm) != op_net_flat_iterms.end()) {
          printf(
              "Error: buffer output iterm set overlaps with input iterm set "
              "!\n");
          printf("Check Iterm %s\n", iterm->getName('/').c_str());
        }
      }
    }
    if (op_net_hier) {
      printf(
          "Hier op net %s connected to %d iterms %d bterms %d moditerms %d "
          "modbterms\n",
          ((odb::dbModNet*) op_net_hier)->getName(),
          ((odb::dbModNet*) op_net_hier)->getITerms().size(),
          ((odb::dbModNet*) op_net_hier)->getBTerms().size(),
          ((odb::dbModNet*) op_net_hier)->getModITerms().size(),
          ((odb::dbModNet*) op_net_hier)->getModBTerms().size());
    }
    printf("--- Buffer connections\n");
  };
  */
  //--- helper subfunctions

  LibertyPort *buffer_input_port, *buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

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

  // Inserting a buffer is complicated by the fact that verilog netlists
  // use the net name for input and output ports. This means the ports
  // cannot be moved to a different net.

  // This cannot depend on the net in caller because the buffer may be inserted
  // between the driver and the loads changing the net as the repair works its
  // way from the loads to the driver.

  // Determine the type of the load
  // primary output/ dont touch. Set preserve_outputs,
  // top_primary_output and load_db_net

  ClassifyLoadTypes();

  load_net = db_network_->dbToSta(load_db_net);
  keep_input = hasInputPort(load_net) || !preserve_outputs;

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

  // Put the repeater loads in repeater_loads_pins
  // Decide if connections will be modified.
  connectionsWillBeModified();

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
  // set parent and driver_pin.
  determineParentToPutBufferIn();

  Point buf_loc(x, y);
  buffer = resizer_->makeBuffer(buffer_cell, reason, parent, buf_loc);

  inserted_buffer_count_++;
  buffer_ip_pin = nullptr;
  buffer_op_pin = nullptr;
  resizer_->getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);

  Net* new_net = db_network_->makeNet(parent);
  Net* buffer_ip_net = nullptr;
  Net* buffer_op_net = nullptr;

  odb::dbModNet* driver_pin_mod_net = db_network_->hierNet(driver_pin);

  // TODO: Refactor this later
  // original code to preserve regressions
  // It turns out that original code is sensitive to buffer
  // connection order. To preserve backward compatibility
  // in regressions we keep original code for designs without
  // hierarchical elements

  if (!db_network_->hasHierarchicalElements()) {
    if (keep_input) {
      //
      // Case 1
      //------
      // A primary input or do not preserve the outputs
      //
      // use orig net as buffer ip (keep primary input name exposed)
      // use new net as buffer op (ok to use new name on op of buffer).
      // move loads to op side
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
        // flat mode, no hierarchy, just hook up flat nets.
        db_network_->connectPin(const_cast<Pin*>(pin), buffer_op_net);
      }
      db_network_->connectPin(buffer_ip_pin, db_network_->dbToSta(load_db_net));
      db_network_->connectPin(buffer_op_pin, buffer_op_net);
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
          db_network_->disconnectPin(const_cast<Pin*>(pin));
          db_network_->connectPin(const_cast<Pin*>(pin), ip_net);
        }
      }
      // Note bufffers connected at end in original code
      db_network_->connectPin(buffer_ip_pin, buffer_ip_net);
      db_network_->connectPin(buffer_op_pin, buffer_op_net);
    }  // case 2
  }

  //
  // new code, which supports hierarchy
  // and wires the buffer in different order
  //
  else {
    if (keep_input) {
      /*
      reportDriverPinConnections();
      reportLoadPins();
      reportBufferConnections();
      */
      //
      // Case 1
      //------
      // Driver is a primary input or loads do not have primary output
      //
      // use orig net as buffer ip (keep primary input name exposed)
      // use new net as buffer op (ok to use new name on op of buffer).
      // move loads to op side (so might need to rename any hierarchical
      // nets to avoid conflict of names with primary input net).
      //
      // record the driver pin modnet, if any

      //
      // Copy signal type to new net.
      //
      dbNet* ip_net_db = load_db_net;  // orig net
      dbNet* op_net_db = db_network_->staToDb(new_net);
      op_net_db->setSigType(ip_net_db->getSigType());
      out_net = new_net;

      buffer_op_net = new_net;
      buffer_ip_net = db_network_->dbToSta(ip_net_db);

      //
      // note in hierarchical mode we are setting up the buffer
      // connections before doing the hierarchical conneciton,
      // this means hiearchical connect can use the buffer_op_pin
      // net, a new net, without having to make a new one).
      //

      Net* driver_hier_net
          = db_network_->dbToSta(db_network_->hierNet(driver_pin));
      db_network_->connectPin(buffer_ip_pin, buffer_ip_net, driver_hier_net);
      db_network_->connectPin(buffer_op_pin, buffer_op_net);

      // The new net is on the output side, we leave the driver
      // untouched and clean it up later.

      for (const Pin* load_pin : load_pins) {
        Instance* inst = network_->instance(load_pin);
        if (resizer_->dontTouch(inst)) {
          continue;
        }

        // disconnect the load pin from everything
        db_network_->disconnectPin(const_cast<Pin*>(load_pin));

        db_network_->hierarchicalConnect(db_network_->flatPin(buffer_op_pin),
                                         db_network_->flatPin(load_pin));
      }

    } else /* case 2 */ {
      //
      // case 2. One of the loads is a primary output or a dont touch
      // Note that even if all loads dont touch we still insert a buffer
      //
      // Use the new net as the buffer input. Preserve
      // the output net as is. Transfer non repeater loads
      // to input side

      // completely disconnect the driver pin. Note we still have the
      // driver_pin_mod
      db_network_->disconnectPin(driver_pin);

      out_net = load_net;
      dbNet* op_net_db = load_db_net;
      dbNet* ip_net_db = db_network_->staToDb(new_net);
      ip_net_db->setSigType(op_net_db->getSigType());

      buffer_ip_net = new_net;
      buffer_op_net = db_network_->dbToSta(load_db_net);

      // only a flat net on driver pin
      db_network_->connectPin(driver_pin,
                              buffer_ip_net);  // to new net

      // hook up buffer.

      db_network_->connectPin(buffer_op_pin,
                              buffer_op_net);  // original net on op of buffer
      db_network_->connectPin(buffer_ip_pin, buffer_ip_net);  // new net on ip

      //
      // move non repeater loads from op net onto ip net, preserving
      // any hierarchical connection. Note we skip the buffer op pin
      // which is connected to the buffer_op_net.
      //

      // note pin iterator does not include top level bterms !
      // a latent bug which seems to pervade the system.
      // bterms are not regarded as pins

      std::unique_ptr<NetPinIterator> pin_iter(
          network_->pinIterator(buffer_op_net));
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();

        if (pin == buffer_op_pin) {
          continue;
        }

        if (db_network_->hierPin(pin)) {
          continue;
        }
        // non-repeater load pins
        if (!repeater_load_pins.hasKey(pin)) {
          Instance* inst = network_->instance(pin);
          // do not disconnect/reconnect don't touch instances
          if (resizer_->dontTouch(inst)) {
            continue;
          }

          // disconnect the load pin
          db_network_->disconnectPin(const_cast<Pin*>(pin));

          // connect it to the new buffer net, the buffer input net
          // non repeater loads go on input side
          db_network_->connectPin(const_cast<Pin*>(pin), buffer_ip_net);

          db_network_->hierarchicalConnect(db_network_->flatPin(driver_pin),
                                           db_network_->flatPin(pin));
        }
      }

      // If the driver pin mod net still (after removing the
      // non load objects) has connections, then
      // connect it to the output of the buffer.

      if (driver_pin_mod_net && driver_pin_mod_net->connectionCount() > 1) {
        db_network_->disconnectPin(buffer_op_pin);
        db_network_->connectPin(
            buffer_op_pin, buffer_op_net, (Net*) driver_pin_mod_net);
      }
    }
  }

  if (graphics_) {
    dbInst* db_inst = db_network_->staToDb(buffer);
    graphics_->makeBuffer(db_inst);
  }

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

    debugPrint(logger_, RSZ, "memory", 1, "RSS = {}", utl::getCurrentRSS());
  }

  if (end) {
    logger_->report(
        "--------------------------------------------------------------------"
        "-");
  }
}


void RepairDesign::initBufferedTrees()
{
  buffered_trees_pre_.clear();
  buffered_trees_post_.clear();
  buffered_trees_mid_.clear();
  buffered_trees_prob_.clear();
}

void RepairDesign::writeBufferedTrees(const std::string& file_name)
{
  std::cout << "This is a dummy function" << std::endl;
}

void RepairDesign::repairDesign_update(
  double max_wire_length,  // zero for none (meters)
  double slew_margin,
  double cap_margin,
  double buffer_gain,
  bool verbose,
  int& repaired_net_count,
  int& slew_violations,
  int& cap_violations,
  int& fanout_violations,
  int& length_violations)

{
  init();
  initBufferedTrees();
  slew_margin_ = slew_margin;
  cap_margin_ = cap_margin;
  buffer_gain_ = buffer_gain;
  bool gain_buffering = (buffer_gain_ != 0.0);
  int print_iteration = 0;

  slew_violations = 0;
  cap_violations = 0;
  fanout_violations = 0;
  length_violations = 0;
  repaired_net_count = 0;
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  resizer_->resized_multi_output_insts_.clear();

  // keep track of user annotations so we don't remove them
  std::set<std::pair<Vertex*, int>> slew_user_annotated;

  if (gain_buffering) {
    // If gain-based buffering is enabled, we need to override slews in order to
    // get good required time estimates.
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
  }

  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();
  sta_->searchPreamble();
  search_->findAllArrivals();

  //resizer_->incrementalParasiticsBegin();
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  
  if (resizer_->level_drvr_vertices_.size() > size_t(5) * max_print_interval_) {
    print_interval_ = max_print_interval_;
  } else {
    print_interval_ = min_print_interval_;
  }
  if (verbose) {
    printProgress(print_iteration, false, false, repaired_net_count);
  }
  int max_length = resizer_->metersToDbu(max_wire_length);
  for (int i = resizer_->level_drvr_vertices_.size() - 1; i >= 0; i--) {
    print_iteration++;
    if (verbose) {
      printProgress(print_iteration, false, false, repaired_net_count);
    }
    Vertex* drvr = resizer_->level_drvr_vertices_[i];
    Pin* drvr_pin = drvr->pin();
    Net* net = network_->isTopLevelPort(drvr_pin)
                   ? network_->net(network_->term(drvr_pin))
                   // hier fix
                   : db_network_->dbToSta(db_network_->flatNet(drvr_pin));
    dbNet* net_db = db_network_->staToDb(net);
    bool debug = (drvr_pin == resizer_->debug_pin_);
    if (debug) {
      logger_->setDebugLevel(RSZ, "repair_net", 3);
    }
    if (gain_buffering) {
      search_->findRequireds(drvr->level() + 1);
    }
    if (net && !resizer_->dontTouch(net) && !net_db->isConnectedByAbutment()
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells and supply nets.
        && !drvr->isConstant()) {
      // To do: create a buffered tree to store the original net
      float max_cap = INF;
      createPreBufferedTree(net, drvr_pin, max_cap);
      // std::cout << "Begin Pre Tree Print\n";
      // buffered_trees_pre_.back()->printTree();
      // std::cout << "End Pre Tree Print\n";

      buffered_trees_post_.push_back(std::make_shared<BufferedTree>(*buffered_trees_pre_.back()));
      //buffered_trees_post_.push_back(buffered_trees_pre_.back());
      buffer_order_count_ = 0; // recored the order of the inserted buffers
      repairNet_update(net,
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

    if (gain_buffering) {
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
  }

  estimate_parasitics_->updateParasitics();
  if (verbose) {
    printProgress(print_iteration, true, true, repaired_net_count);
  }
  //resizer_->incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    resizer_->level_drvr_vertices_valid_ = false;
  }
}

void RepairDesign::repairNet_update(Net* net,
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
  // update the flag
  store_buffered_trees_flag_ = true;
  const sta::Pin* pin = drvr_pin_;
  auto OpenSTASt = std::chrono::high_resolution_clock::now();
  // logger_->report("[MAX Wirelength] : {}", max_length);  //YL 
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
    const Pin* load_pin = nullptr;

    if (buffer_gain_ != 0.0) {
      float fanout, max_fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, max_, fanout, max_fanout, fanout_slack);

      int resized = resizer_->resizeToTargetSlew(drvr_pin);
      if (performGainBuffering(net, drvr_pin, max_fanout)) {
        repaired_net = true;
      }
      // Resize again post buffering as the load changed
      resized += resizer_->resizeToTargetSlew(drvr_pin);
      if (resized > 0) {
        repaired_net = true;
        resize_count_ += 1;
      }
    }

    if (check_fanout) {
      float fanout, max_fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, max_, fanout, max_fanout, fanout_slack);
      
      if (max_fanout > 0.0 && fanout_slack < 0.0) {
        fanout_violations++;
        repaired_net = true;

        debugPrint(logger_, RSZ, "repair_net", 3, "fanout violation");
        LoadRegion region = findLoadRegions(net, drvr_pin, max_fanout);
        // logger_->report("[MAX Fanout] : {}", max_fanout);  //YL  
        //exit(1);
        
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

    // Resize the driver to normalize slews before repairing limit violations.
    //if (parasitics_src_ == ParasiticsSrc::estimate && resize_drvr) { //Cshah - had to modify ParasiticsSrc::placement to ParasiticsSrc::global_routing 
    if (resize_drvr) {
      resize_count_ += resizer_->resizeToTargetSlew(drvr_pin);
    }
    // For tristate nets all we can do is resize the driver.
    if (!resizer_->isTristateDriver(drvr_pin)) {
      BufferedNetPtr bnet = resizer_->makeBufferedNetSteiner(drvr_pin, corner);
      if (bnet) {
        int wire_length = bnet->maxLoadWireLength();
        const sta::Parasitics* paras = sta_->parasitics();
        if (pin && paras) {
          auto* mpin   = const_cast<sta::Pin*>(pin);
          auto* mparas = const_cast<sta::Parasitics*>(paras);
          resizer_->makeWireParasitic(net, mpin, nullptr, wire_length, corner_, mparas);
        }

        //resizer_->makeWireParasitic(net, pin, nullptr, wire_length, corner, paras);
        graph_delay_calc_->findDelays(drvr);

        float max_cap = INF;
        
        bool needs_repair = needRepair(drvr_pin,
                                      corner,
                                      max_length,
                                      wire_length,
                                      check_cap,
                                      check_slew,
                                      max_cap,
                                      slew_violations,
                                      cap_violations,
                                      length_violations);

        if (needs_repair) {
          //if (parasitics_src_ == ParasiticsSrc::estimate && resize_drvr) {
          if (resize_drvr)  {
            resize_count_ += resizer_->resizeToTargetSlew(drvr_pin);
            wire_length = bnet->maxLoadWireLength();
            needs_repair = needRepair(drvr_pin,
                                     corner,
                                     max_length,
                                     wire_length,
                                     check_cap,
                                     check_slew,
                                     max_cap,
                                     slew_violations,
                                     cap_violations,
                                     length_violations);
          }
          if (needs_repair) {
            Point drvr_loc = db_network_->location(drvr->pin());
            debugPrint(
                logger_,
                RSZ,
                "repair_net",
                1,
                "driver {} ({} {}) l={}",
                sdc_network_->pathName(drvr_pin),
                units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getX()),
                                                 1),
                units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getY()),
                                                 1),
                units_->distanceUnit()->asString(dbuToMeters(wire_length), 1));

            // ----------------------------------------------------------------
            // YL: recored problematic nets to files for MLBuf / adhoc processing
            auto OpenSTAEnd = std::chrono::high_resolution_clock::now();
            auto OpenSTADuration = std::chrono::duration_cast<std::chrono::milliseconds>(OpenSTAEnd - OpenSTASt);
            
            BufferedTreePtr problematic_net = buffered_trees_pre_.back();
            auto copied_tree = std::make_shared<rsz::BufferedTree>(*problematic_net); // Make a deep copy
            buffered_trees_prob_.push_back(copied_tree);

            auto tree = copied_tree;
           
            BufferedTreeNodePtr root = tree->getRoot();
            
            auto repairSt = std::chrono::high_resolution_clock::now();
            

            // -------------------------------------------------------
            repairNet(bnet, drvr_pin, max_cap, max_length, corner);
            repaired_net = true;

            if (resize_drvr) {
              resize_count_ += resizer_->resizeToTargetSlew(drvr_pin);
            }

          }
        }
      }
    }
    
    if (repaired_net) {

      repaired_net_count++;
    }
  }
  store_buffered_trees_flag_ = false;
}

void RepairDesign::makeRepeater_update(const char* reason,
                                const Point& loc,
                                LibertyCell* buffer_cell,
                                bool resize,
                                int level,
                                // Return values.
                                PinSeq& load_pins,
                                float& repeater_cap,
                                float& repeater_fanout,
                                float& repeater_max_slew,
                                float drvr_output_cap,
                                float drvr_output_slew)
{ 

  Net* out_net;
  Pin *repeater_in_pin, *repeater_out_pin;
  makeRepeater_update(reason,
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
               repeater_out_pin,
               drvr_output_cap, //YL
               drvr_output_slew //YL
              );
}

void RepairDesign::makeRepeater_update(const char* reason,
                                int x,
                                int y,
                                LibertyCell* buffer_cell,
                                bool resize,
                                int level,
                                // Return values.
                                PinSeq& load_pins,
                                float& repeater_cap,
                                float& repeater_fanout,
                                float& repeater_max_slew,
                                Net*& out_net,
                                Pin*& repeater_in_pin,
                                Pin*& repeater_out_pin,
                                float drvr_output_cap,
                                float drvr_output_slew
                              )
{
  //odb::dbBlock* blk = block();
  LibertyPort *buffer_input_port, *buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
  Instance* buffer = resizer_->makeBuffer(
    buffer_cell,
    "rsz_buf",                    // base name; Resizer will uniquify
    db_network_->topInstance(),   // parent
    Point(x, y));  
  odb::dbInst* db_buf = db_network_->staToDb(buffer);
  std::string buffer_name = db_buf->getName();

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

  Net *net = nullptr, *in_net;
  bool preserve_outputs = false;
  for (const Pin* pin : load_pins) {
    if (network_->isTopLevelPort(pin)) {
      net = network_->net(network_->term(pin));
      if (network_->direction(pin)->isAnyOutput()) {
        preserve_outputs = true;
        break;
      }
    } else {
      net = network_->net(pin);
      Instance* inst = network_->instance(pin);
      if (resizer_->dontTouch(inst)) {
        preserve_outputs = true;
        break;
      }
    }
  }
  Instance* parent = db_network_->topInstance();
  odb::dbBlock* blk = resizer_->getDbBlock();
  std::string base = "rsz_net";
  std::string name = base; 
  int i = 0;

  // If the net is driven by an input port,
  // use the net as the repeater input net so the port stays connected to it.
  if (hasInputPort(net) || !preserve_outputs) {
    in_net = net;
    //out_net = createUniqueNet(blk, "rsz_net");
    // Copy signal type to new net.
    while (blk->findNet(name.c_str()))
    name = base + "_" + std::to_string(++i);
    odb::dbNet* db_new = odb::dbNet::create(blk, name.c_str());
    out_net = db_network_->dbToSta(db_new);
    dbNet* out_net_db = db_network_->staToDb(out_net);
    dbNet* in_net_db = db_network_->staToDb(in_net);
    out_net_db->setSigType(in_net_db->getSigType());

    // Move load pins to out_net.
    for (const Pin* pin : load_pins) {
      Port* port = network_->port(pin);
      Instance* inst = network_->instance(pin);

      // do not disconnect/reconnect don't touch instances
      if (resizer_->dontTouch(inst)) {
        continue;
      }
      sta_->disconnectPin(const_cast<Pin*>(pin));
      sta_->connectPin(inst, port, out_net);
    }
  } else {
    // One of the loads is an output port.
    // Use the net as the repeater output net so the port stays connected to it.
    //in_net = createUniqueNet(blk, "rsz_net"); //added by CShah - unique helper
    i = 0;
    name = base;
    while (blk->findNet(name.c_str()))
    name = base + "_" + std::to_string(++i);
    odb::dbNet* db_new = odb::dbNet::create(blk, name.c_str());
    in_net = db_network_->dbToSta(db_new);
    out_net = net;
    // Copy signal type to new net.
    dbNet* out_net_db = db_network_->staToDb(out_net);
    dbNet* in_net_db = db_network_->staToDb(in_net);
    in_net_db->setSigType(out_net_db->getSigType());

    // Move non-repeater load pins to in_net.
    PinSet load_pins1(db_network_);
    for (const Pin* pin : load_pins) {
      load_pins1.insert(pin);
    }

    NetPinIterator* pin_iter = network_->pinIterator(out_net);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      if (!load_pins1.hasKey(pin)) {
        Port* port = network_->port(pin);
        Instance* inst = network_->instance(pin);
        sta_->disconnectPin(const_cast<Pin*>(pin));
        sta_->connectPin(inst, port, in_net);
      }
    }
  }

  //Point buf_loc(x, y);
  //Instance* buffer
  //    = resizer_->makeBuffer(buffer_cell, buffer_name, parent, buf_loc);
  inserted_buffer_count_++;

  // --------- save buf_loc to file if BUF_APPROACH is rsz ------- //
  dbInst* db_inst = db_network_->staToDb(buffer);
  odb::dbMaster* buf_master = db_inst->getMaster();
  const int buf_width = buf_master->getWidth();
  const int buf_height = buf_master->getHeight();
  int ux = x + buf_width;
  int uy = y + buf_height;
  int buf_area = buf_width * buf_height;

  std::string buf_approach = std::getenv("BUF_APPROACH");
  if (buf_approach == "rsz"){
    const char* output_file = std::getenv("OUTPUT");
    std::ofstream outfile(output_file, std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "can not open rsz_buf_save.csv" << std::endl;
        exit(1);
    }
    outfile << x << "," << y << "," << ux << "," << uy << "," << buf_area << std::endl;
    outfile.close();
    }

  if (buf_approach == "rsz"){
      const char* out_env = std::getenv("OUTPUT");
      std::string out_path = (out_env && *out_env)? std::string(out_env): std::string("rsz_buf_save.csv");
      try {
        std::filesystem::path p(out_path);
        if (p.has_parent_path()) {
          std::error_code ec;
          std::filesystem::create_directories(p.parent_path(), ec); // ignore if exists
        }
        std::ofstream outfile(out_path, std::ios::out | std::ios::app);
        if (!outfile.is_open()) {
          std::cerr << "cannot open output file: " << out_path
                    << " (cwd=" << std::filesystem::current_path().string() << ")\n";
          std::exit(1);
        }
        outfile << x << "," << y << "," << ux << "," << uy << "," << buf_area << '\n';
      }
      catch (const std::exception& e) {
        std::cerr << "failed to write buffered-tree row to " << out_path
                  << " (cwd=" << std::filesystem::current_path().string()
                  << "): " << e.what() << "\n";
        std::exit(1);
      }
    }
  // ------------------------------------------------------
  
  sta_->connectPin(buffer, buffer_input_port, in_net);
  sta_->connectPin(buffer, buffer_output_port, out_net);
  sta::Parasitics* paras = sta_->parasitics();

  //resizer_->parasiticsInvalid(in_net);
  //resizer_->parasiticsInvalid(out_net);
  if (in_net) {
    if (const sta::Pin* drvr_in_c = findDriverPin(resizer_, in_net)) {
      sta::Pin* drvr_in = const_cast<sta::Pin*>(drvr_in_c);
      resizer_->makeWireParasitic(in_net, drvr_in, nullptr, 0, corner_, paras);
    }
  }

  if (out_net) {
    if (const sta::Pin* drvr_out_c = findDriverPin(resizer_, out_net)) {
      sta::Pin* drvr_out = const_cast<sta::Pin*>(drvr_out_c);
      resizer_->makeWireParasitic(out_net, drvr_out, nullptr, 0, corner_, paras);
    }
  }

  if (in_net && drvr_pin_)  resizer_->makeWireParasitic(in_net, const_cast<sta::Pin*>(drvr_pin_), nullptr, 0, corner_, paras);
  if (out_net && drvr_pin_) resizer_->makeWireParasitic(out_net, const_cast<sta::Pin*>(drvr_pin_), nullptr, 0, corner_, paras);
  //Cshah - added to update delays after parasitic changes

  if (resize) {
    Pin* buffer_out_pin = network_->findPin(buffer, buffer_output_port);
    resizer_->resizeToTargetSlew(buffer_out_pin);
    buffer_cell = network_->libertyCell(buffer);
    buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
  }

  repeater_in_pin = network_->findPin(buffer, buffer_input_port);
  repeater_out_pin = network_->findPin(buffer, buffer_output_port);
  
  
  repeater_cap = resizer_->portCapacitance(buffer_input_port, corner_);
  repeater_fanout = resizer_->portFanoutLoad(buffer_input_port);
  repeater_max_slew = bufferInputMaxSlew(buffer_cell, corner_);
  
  // Save original features
  if (buffer_order_count_ == 0){
    BufferedTreePtr tree = buffered_trees_post_.back(); //YL: get the last tree
    // Make a deep copy
    auto copied_tree = std::make_shared<rsz::BufferedTree>(*tree);
    // Push the copy into buffered_trees_mid_
    buffered_trees_mid_.push_back(copied_tree);

  }
  buffer_order_count_++; 
  createPostBufferedTree(
    reason,
    buffer, 
    buffer_cell, 
    x, y, 
    load_pins,
    repeater_fanout,
    repeater_in_pin,
    repeater_out_pin,
    repeater_cap,
    drvr_output_cap,
    drvr_output_slew);
  
  load_pins.clear();
  load_pins.push_back(repeater_in_pin);
}


void RepairDesign::repairNetWire_update(
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
  bnet->wireRC(corner_, resizer_, estimate_parasitics_, wire_res, wire_cap);
  // std::cout<<"[lyt test in repairNetWire_update] wire_res: "<<wire_res<<"wire_cap: "<<wire_cap<<std::endl;
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
  odb::dbNet* db_net = nullptr;
  if (drvr_pin_) {
    if (auto* s_net = db_network_->net(drvr_pin_)) {
      db_net = db_network_->staToDb(s_net);
    }
  }
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

      // makeRepeater_update("wire",
      //              Point(buf_x, buf_y),
      //              buffer_cell,
      //              resize,
      //              level,
      //              load_pins,
      //              repeater_cap,
      //              repeater_fanout,
      //              max_load_slew);

      // --------- YL: update repeater_cap -------------
      LibertyPort *buffer_input_port, *buffer_output_port;
      buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
      repeater_cap = resizer_->portCapacitance(buffer_input_port, corner_);
      // -------------------------------------------------

      // Update for the next round.
      length -= buf_dist;
      wire_length = length;
      to_x = buf_x;
      to_y = buf_y;

      length1 = dbuToMeters(length);
      wire_length_ref = 0.0;
      load_cap = repeater_cap + length1 * wire_cap; // YL: driver output cap
      ref_cap = repeater_cap;
      load_slew= (r_drvr + length1 * wire_res) * load_cap * elmore_skew_factor_;
          //YL: driver output slew    
      makeRepeater_update("wire",
              Point(buf_x, buf_y),
              buffer_cell,
              resize,
              level,
              load_pins,
              repeater_cap,
              repeater_fanout,
              max_load_slew,
              load_cap,   // driver updated output cap 
              load_slew); // driver updated output slew

      max_load_slew_margined = max_load_slew;  // maxSlewMargined(max_load_slew);
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


void RepairDesign::repairNetJunc_update(
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
  resizer_->getEstimateParasitics()->wireSignalRC(corner_, wire_res, wire_cap); //Cshah - changed from wireRC to wireSignalResistance
  // std::cout<<"[lyt test in repairNetJunc_update] wire_res: "<<wire_res<<"wire_cap: "<<wire_cap<<std::endl;

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
  
  // YL: calculate the driver output cap and slew
  float drvr_output_cap = cap_left + cap_right;
  if(repeater_left) {
    wire_length_left = 0;
  }
  if(repeater_right) {
    wire_length_right = 0;
  }
  float drvr_output_slew = (r_drvr + dbuToMeters(wire_length_left + wire_length_right) * wire_res) * drvr_output_cap
                  * elmore_skew_factor_;

  if (repeater_left) {
    makeRepeater_update(repeater_reason,
                 loc,
                 buffer_cell,
                 true,
                 level,
                 loads_left,
                 cap_left,
                 fanout_left,
                 max_load_slew_left,
                 drvr_output_cap, 
                 drvr_output_slew);
    wire_length_left = 0;
  }
  if (repeater_right) {
    makeRepeater_update(repeater_reason,
                 loc,
                 buffer_cell,
                 true,
                 level,
                 loads_right,
                 cap_right,
                 fanout_right,
                 max_load_slew_right,
                 drvr_output_cap, 
                 drvr_output_slew);
    wire_length_right = 0;
  }

  // Update after left/right repeaters are inserted.
  wire_length = wire_length_left + wire_length_right;

  bnet->setCapacitance(cap_left + cap_right); //driver output cap
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


//YL: verfity the constructed buffered tree
void RepairDesign::testBufferedTree(Pin*& repeater_in_pin,
                        Pin*& repeater_out_pin, 
                        Net*& in_net, 
                        Net*& out_net)
{
  std::string buffer_in_pin_name = sdc_network_->pathName(repeater_in_pin);
  std::string buffer_out_pin_name = sdc_network_->pathName(repeater_out_pin);
  std::cout<<"Buffer in pin name: "<<buffer_in_pin_name<<std::endl;
  std::cout<<"Buffer out pin name: "<<buffer_out_pin_name<<std::endl;
  NetConnectedPinIterator* pin_iter_in = network_->connectedPinIterator(in_net);
  std::cout<<"In pin name: ";
  while (pin_iter_in->hasNext()) {
    const Pin* pin = pin_iter_in->next();
    std::string in_pin_name = sdc_network_->pathName(pin);
    Point sink_loc = db_network_->location(pin);
    float in_pin_x = dbuToMeters(sink_loc.getX()) * 1e6;
    float in_pin_y = dbuToMeters(sink_loc.getY()) * 1e6;
    std::cout<<" "<<in_pin_name<<" ("<<in_pin_x<<", "<<in_pin_y<<") \t";
  }

  NetConnectedPinIterator* pin_iter_out = network_->connectedPinIterator(out_net);
  std::cout<<"\n Out pin name: ";
  while (pin_iter_out->hasNext()) {
    const Pin* pin = pin_iter_out->next();
    std::string out_pin_name = sdc_network_->pathName(pin);
    Point sink_loc = db_network_->location(pin);
    float out_pin_x = dbuToMeters(sink_loc.getX()) * 1e6;
    float out_pin_y = dbuToMeters(sink_loc.getY()) * 1e6;
    std::cout<<" "<<out_pin_name<<" ("<<out_pin_x<<",  "<<out_pin_y<<") \t";
  }

}

// ----------------------------------------------------------------------------
//  New functions for MLBuf
// ----------------------------------------------------------------------------


float RepairDesign::getOutputCap(const Pin* drvr_pin, float& max_cap)
{
  float cap1, max_cap1, cap_slack1;
  const Corner* corner1;
  const RiseFall* tr1;
  sta_->checkCapacitance(drvr_pin, nullptr, max_, corner1, tr1, cap1, max_cap1, cap_slack1);
  if (max_cap1 > 0.0 && corner1) {
    max_cap1 *= (1.0 - cap_margin_ / 100.0);
    max_cap = max_cap1;
    return cap1;
  } 

  return 0.0;
}

void RepairDesign::getSlew(
  const Pin* drvr_pin, 
  float& max_cap,
  float &output_slew,
  float &input_slew)
{
  float slew1, slew_slack1, max_slew1;
  const Corner* corner1;
  // Check slew at the driver.
  checkSlew(drvr_pin, slew1, max_slew1, slew_slack1, corner1);
  // Max slew violations at the driver pin are repaired by reducing the
  // load capacitance. Wire resistance may shield capacitance from the
  // driver but so this is conservative.
  // Find max load cap that corresponds to max_slew.
  LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
  output_slew = slew1;
  if (corner1 && max_slew1 > 0.0) {
    if (drvr_port) {
      float max_cap1 = findSlewLoadCap(drvr_port, max_slew1, corner1);
      max_cap = min(max_cap, max_cap1);
    }
  }
  // Check slew at the loads.
  // Note that many liberty libraries do not have max_transition attributes on
  // input pins.
  // Max slew violations at the load pins are repaired by inserting buffers
  // and reducing the wire length to the load.
  resizer_->checkLoadSlews(
      drvr_pin, slew_margin_, slew1, max_slew1, slew_slack1, corner1);
  input_slew = slew1;
}


// Signoff at 2025/01/29
void RepairDesign::createPreBufferedTree(
  Net* net, 
  const Pin* drvr_pin, 
  float max_cap)
{
  BufferedTreePtr tree = std::make_shared<BufferedTree>(net);
  BufferedTreeNodePtr root = std::make_shared<BufferedTreeNode>();
  root->pin_name = sdc_network_->pathName(drvr_pin);
  Point drvr_loc = db_network_->location(drvr_pin);
  root->x = dbuToMeters(drvr_loc.getX()) * 1e6;
  root->y = dbuToMeters(drvr_loc.getY()) * 1e6;
  root->resistance = resizer_->driveResistance(drvr_pin);
  root->buffer_order = 0;
  root->buffer_type = "-1";
  root->buf_reason = "-1";
  // Check Capacitance
  root->output_cap = getOutputCap(drvr_pin, max_cap);
  root->input_cap = -1.0;

  // check slew
  float input_slew; // for sinks
  float output_slew;
  getSlew(drvr_pin, max_cap, output_slew, input_slew);  
  root->output_slew = output_slew;
  root->input_slew = -1.0;
  root->max_output_cap = max_cap;
  root->parent = nullptr;
  root->node_type = BufferedTreeNodeType::DRIVER;

 
  float slew1, slew_slack1, max_slew1;
  const Corner* corner1;
  // Check slew at the driver.
  checkSlew(drvr_pin, slew1, max_slew1, slew_slack1, corner1);
  double slack_ps = std::numeric_limits<double>::quiet_NaN();
  if (drvr_pin){
    sta::Vertex* vtx = graph_->pinDrvrVertex(drvr_pin);
    if (vtx){
      sta::Slack s = sta_->vertexSlack(vtx, max_);
      if (std::isfinite(static_cast<double>(s))){
        slack_ps = static_cast<double>(s) * 1e12; // convert to ps
      }
    }
  }
  root->driver_slack_ps = slack_ps;
  std::unordered_map<const sta::Pin*, double> sink_wire_ps;

  if (corner1){ // if corner1 is nullptr (drvr does not related to any timing-related path), we can skip this net
    // Check all the sink pins
    NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      // ignore all the sinks that are "output-ports"
      if (pin != drvr_pin && !network_->isTopLevelPort(pin)
        && network_->direction(pin) == sta::PortDirection::input()
        && network_->libertyPort(pin)) {
        BufferedTreeNodePtr sink = std::make_shared<BufferedTreeNode>();
        sink->pin_name = sdc_network_->pathName(pin);
        Point sink_loc = db_network_->location(pin);
        sink->x = dbuToMeters(sink_loc.getX()) * 1e6;
        sink->y = dbuToMeters(sink_loc.getY()) * 1e6;
        // Check Capacitance
        sink->output_cap = -1.0;
        LibertyPort* input_port = network_->libertyPort(pin);
        sink->input_cap = resizer_->portCapacitance(input_port, corner1);
        // check slew
        sink->output_slew = -1.0;
        sink->input_slew = input_slew;
        sink->max_output_cap = -1.0;
        sink->parent = root;
        sink->node_type = BufferedTreeNodeType::SINK;
        sink->resistance = -1;
        sink->buffer_type = "-1";
        sink->buf_reason = "-1";
        sink->buffer_order = 0;
        root->children.push_back(sink);
      }
    } 
  }
  else {
    logger_->report("[Warning] Failed to retrieve corner for driver pin: {}", sdc_network_->pathName(drvr_pin));    
  }
  /* if (!load_pins.empty() && corner1){
    const sta::DcalcAnalysisPt* dcalc_ap = corner1->findDcalcAnalysisPt(max_);
    sta::ArchDelayCalc* adc = sta_->ArchDelayCalc();
    sta::Parasitics* paras = sta_->parasitics();

    sta::LoadPinIndexMap lpim(network_);
    int idx = 0;
    for (const sta::Pin* lp : load_pins) {
      lpim[const_cast<sta::Pin*>(lp)] = idx++;
    }
    const sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    const sta::LibertyCell* drvr_cell = network_->libertyCell(drvr_pin);
    bool computed = false;
    for (sta::TimingArcSet* set : drvr_cell->timingArcSets()){
      if (set->to() != drvr_port) continue;
      for (sta::TimingArc* arc : set->arcs()){
        const sta::RiseFall* rf = arc->toEdge()->asRiseFall();
        if (!rf) continue;
        sta::Parasitic* drvr_par = adc->findParasitic(
            drvr_pin,
            rf,
            dcalc_ap,
            paras);
        if (!drvr_par) continue;
        float load_cap = paras->totalCapacitance(drvr_par);
        doubt in_slew = 0;
        sta::ArcDcalcResult dcalc = adc-> gateDelay(const_cast<sta::Pin*>(drvr_pin),
                                                    drvr_par,
                                                    rf,
                                                    dcalc_ap,
                                                    paras,
                                                    &lpim,
                                                    load_cap,
                                                    in_slew);
        for (const sta::Pin* lp : load_pins) {
          int i = lpim[const_cast<sta::Pin*>(lp)];
          double w_ps = static_cast<double>(dcalc.wireDelay(i))*1e12;
          sink_wire_ps[lp] = std::isfinite(w_ps) ? w_ps : std;
    }
    adc->finishDrvrPin();
    computed = true;
    break;
  }
  if (computed) break;
  } 
  root->fanout = root->children.size();
  tree->setRoot(root);
  
  auto it = sink_wire_ps.find(sink_pin);
  sink_node->net_wire_delay_ps = (it != sink_wire_ps.end()) ? it->second : std::numeric_limits<double>::quiet_NaN();*/
  root->fanout = root->children.size();
  tree->setRoot(root);
  buffered_trees_pre_.push_back(tree);
}

void RepairDesign::createPostBufferedTree(
  const char* reason,
  Instance* buffer_inst,
  LibertyCell* buffer_cell,
  int x, int y,
  PinSeq& load_pins,
  float repeater_fanout,
  Pin*& repeater_in_pin,
  Pin*& repeater_out_pin,
  float repeater_input_cap,
  float drvr_output_cap,
  float drvr_output_slew)
{
  BufferedTreePtr& tree = buffered_trees_post_.back(); //YL: get the last tree
  
  // 1. create buffer node
  BufferedTreeNodePtr buffer = std::make_shared<BufferedTreeNode>();
  buffer->pin_name = sdc_network_->pathName(repeater_in_pin); // buffer cell input pin
  buffer->x = dbuToMeters(x) * 1e6;
  buffer->y = dbuToMeters(y) * 1e6;
  buffer->buffer_order = buffer_order_count_;
  buffer->buffer_type = buffer_cell->name();
  buffer->buf_reason = reason;
  // buffer resistance
  float r_buffer = resizer_->bufferDriveResistance(buffer_cell);
  buffer->resistance = r_buffer;
  // const Corner* corner1;

  // Check Capacitance
  float max_cap = INF;
  //YL: does the buffer need output_cap?
  buffer->output_cap = getOutputCap(repeater_out_pin, max_cap);
  buffer->input_cap = repeater_input_cap;
  
  //YL: does the buffer need output_slew?
  buffer->output_slew = -1.0;
  buffer->input_slew = drvr_output_slew;

  buffer->max_output_cap = max_cap;
  buffer->node_type = BufferedTreeNodeType::BUFFER;

  // 2. add buffer to the child of the root
  BufferedTreeNodePtr root = tree->getRoot();
  buffer->parent = root;
  root->children.push_back(buffer);
  root->buffer_order = buffer_order_count_;
  // YL: Update output cap and output slew of the driver
  if (drvr_output_cap !=0 and drvr_output_slew !=0)
  {
    root->output_cap = drvr_output_cap;
    root->output_slew = drvr_output_slew;
  }
  
  // 3. find sinks driven by the buffer
  std::vector<BufferedTreeNodePtr> nodes_to_remove;
  auto& root_children = tree->getRoot()->children;
  // Precompute load pin names for faster lookup
  std::unordered_set<std::string> load_pin_names;
  for (const Pin* pin : load_pins) {
      load_pin_names.insert(sdc_network_->pathName(pin));
  }
  // Iterate through root's children to find matching loads
  for (auto& child : root_children) {
    child->buffer_order = buffer_order_count_;
      if (load_pin_names.find(child->pin_name) != load_pin_names.end()) {
          buffer->children.push_back(child);
          child->parent = buffer;
          nodes_to_remove.push_back(child);
      }
      //YL: update input slew of the sink
      else if (drvr_output_slew!=0) {
        child->input_slew = drvr_output_slew;
      }
  }

  // 4. Remove loads from the root
  for (const auto& node : nodes_to_remove) {
      root_children.erase(
          std::remove(root_children.begin(), root_children.end(), node),
          root_children.end()
      );
  }

  //5. update fanout
  root->fanout = root->children.size();
  buffer->fanout = buffer->children.size();
  // tree->setRoot(root);
  
  // Make a deep copy
  auto copied_tree = std::make_shared<rsz::BufferedTree>(*tree);

  // Push the copy into buffered_trees_mid_
  buffered_trees_mid_.push_back(copied_tree);

}

                       
// ----------------------------------------------------------------------------
//  Save Buffered Trees
// ----------------------------------------------------------------------------

// count the number of sinks and buffers in the tree
void RepairDesign::countNodes(const BufferedTreePtr& tree, int& sink_count, int& buffer_count) {
  if (!tree || !tree->getRoot()) return;

  std::queue<BufferedTreeNodePtr> q;
  q.push(tree->getRoot());

  sink_count = 0;
  buffer_count = 0;

  while (!q.empty()) {
      BufferedTreeNodePtr node = q.front();
      q.pop();

      if (node->node_type == BufferedTreeNodeType::SINK) {
          sink_count++;
      } else if (node->node_type == BufferedTreeNodeType::BUFFER) {
          buffer_count++;
      }

      for (const auto& child : node->children) {
          q.push(child);
      }
  }
}

// save to csv file
void RepairDesign::printTreeToCSV(const std::vector<BufferedTreePtr>& trees, const std::string& filename, bool buffered_tree_print_flag) {
  std::ofstream file(filename);

  if (!file.is_open()) {
      std::cerr << "Error: Unable to open file for writing!\n";
      return;
  }

  // csv header
  file << "Net Name,Tree ID,Depth,Node ID,Node Type,Pin Name,X,Y,Input Slew,Output Slew,";
  file << "Input Cap,Output Cap,Max Output Cap,Fanout,Resistance,Sink Count,Buffer Count,Parent Node ID,#Children,Buffer Order,Buffer Type, Buffer Reason, Driver Slack (ps), Wire Delay (ps)\n";

  int tree_id = 0;
  for (const auto& tree : trees) {
    if (!tree || !tree->getRoot()) continue;

    // Skip this tree if it does not contain any "Buffer" node
    // if (!treeContainsBuffer(tree) && buffered_tree_print_flag) continue;
    
    // Identify whether the tree contains buf and sinks
    bool hasBuf, hasSink;
    std::tie(hasBuf, hasSink) = treeContainsSpecificNode(tree);
    // Skip this tree if it does not contain any "Buffer" node
    if (!hasBuf && buffered_tree_print_flag) continue;

    // Skip this tree if it does not contain any "Sink" node
    if (!hasSink) continue;
  
    int sink_count = 0, buffer_count = 0;
    countNodes(tree, sink_count, buffer_count);

    // obtain net name
    dbNet* net_db = db_network_->staToDb(tree->getNet());
    std::string net_name = net_db->getName();

    std::queue<std::pair<BufferedTreeNodePtr, int>> q;
    
    q.push({tree->getRoot(), 0});
    int node_id = 0;

    while (!q.empty()) {
      auto [node, depth] = q.front();
      q.pop();
      node->node_id = node_id++;



      file << net_name << ",";                         // Net Name
      file << tree_id << ",";                          // Tree ID
      file << depth << ",";                            // Depth
      file << node->node_id << ",";                    // Node ID
      file << bufferedTreeNodeTypeToString(node->node_type) << ",";  // Node Type
      file << node->pin_name << ",";                   // Pin Name
      file << node->x << "," << node->y << ",";        // X, Y
      file << node->input_slew << "," << node->output_slew << ",";  // Slew
      file << node->input_cap << "," << node->output_cap << ",";    // Capacitance
      file << node->max_output_cap << ",";             // Max Output Cap
      file << node->fanout << ",";                     // Fanout
      file << node->resistance << ",";                     // Resistance
      file << sink_count << "," << buffer_count << ","; // Sink Count, Buffer Count
      file << (node->parent ? std::to_string(node->parent->node_id) : "None") << ",";  // Parent Node ID
      file << node->children.size() << ",";            // #Children
      file << node->buffer_order << "," << node->buffer_type << "," << node->buf_reason <<","; // Buffer Order, Buffer Type, Reason
      if (node->node_type == BufferedTreeNodeType::DRIVER && !std::isnan(node->driver_slack_ps)) {
        file << node->driver_slack_ps << ","; // Driver Slack (ps)
      } else {
        file << ",";
      }
      if (std::isfinite(node->net_wire_delay_ps)){
        file << node->net_wire_delay_ps << "\n";
      }
      else {
        file << "\n"; // driver/root has no incoming wire segment
      }         
      for (const auto& child : node->children) {
          q.push({child, depth + 1});
      }
    }
    tree_id++;
  }
  file.close();
}

// check if the tree contains buffer or sink 
std::pair<bool, bool> RepairDesign::treeContainsSpecificNode(const BufferedTreePtr& tree) {
  bool buffer_flag = false;
  bool sink_flag = false;
  // driver_flag = false;
  if (!tree || !tree->getRoot()) return {buffer_flag, sink_flag};

  std::queue<BufferedTreeNodePtr> q;
  q.push(tree->getRoot());

  while (!q.empty()) {
      auto node = q.front();
      q.pop();

      if (node->node_type == BufferedTreeNodeType::BUFFER) {
          buffer_flag = true; // Found at least one buffer node
          // return true; 
      }
      if (node->node_type == BufferedTreeNodeType::SINK) {
          sink_flag = true;
          // return true; // Found at least one buffer node
      }
      if (buffer_flag && sink_flag) return {buffer_flag, sink_flag};

      for (const auto& child : node->children) {
          q.push(child);
      }
      
  }
  return {buffer_flag, sink_flag}; // No buffer node found
}

// check if the tree contains buffer
bool RepairDesign::treeContainsBuffer(const BufferedTreePtr& tree) {
    if (!tree || !tree->getRoot()) return false;

    std::queue<BufferedTreeNodePtr> q;
    q.push(tree->getRoot());

    while (!q.empty()) {
        auto node = q.front();
        q.pop();

        if (node->node_type == BufferedTreeNodeType::BUFFER) {
            return true; // Found at least one buffer node
        }

        for (const auto& child : node->children) {
            q.push(child);
        }
    }
    return false; // No buffer node found
}

int RepairDesign::countNodes(const BufferedTreeNodePtr& node) 
{
  if (!node) {
    return 0;
  }
  int count = 1; 
  for (auto& child : node->children) {
    count += countNodes(child);
  }
  return count;
}

// YL: for MLBuf Integration
void RepairDesign::saveProbNets() {
  if (buffered_trees_prob_.size() > 0) {
    bool buffered_tree_print_flag = false;
    const char* buf_approach = std::getenv("BUF_APPROACH");
    const std::string default_approach = "rsz";
    std::string buffer_approach = (buf_approach != nullptr) ? std::string(buf_approach) : default_approach;

    if (buffer_approach == "MLBuf" || buffer_approach == "Adhoc") {
      std::string prob_net_file = std::getenv("INPUT"); // save problematic nets
      printTreeToCSV(buffered_trees_prob_, prob_net_file, buffered_tree_print_flag);
      std::cout << "buffered_trees_prob_ saved to " << prob_net_file << std::endl;
    } 
    
  }
}

// save
void RepairDesign::saveBufferedTrees() {
  // std::cout << "Total Trees (Pre Repair): " << buffered_trees_pre_.size() << std::endl;
  // std::cout << "Total Trees (Post Repair) " << buffered_trees_post_.size() << std::endl;
  // std::cout << "Total Trees (Mid Repair) " << buffered_trees_mid_.size() << std::endl;

  std::string pre_filename = generateUniqueFilename("buffered_trees_pre");
  std::string post_filename = generateUniqueFilename("buffered_trees_post");
  std::string mid_filename = generateUniqueFilename("buffered_trees_mid");

  if (buffered_trees_pre_.size() > 0) {
    bool buffered_tree_print_flag = false;
    annotateWireDelaysOnTrees(buffered_trees_pre_);
    printTreeToCSV(buffered_trees_pre_, pre_filename, buffered_tree_print_flag);
    std::cout << "buffered_trees_pre saved to " << pre_filename << std::endl;
  }

  if (buffered_trees_post_.size() > 0) {
    bool buffered_tree_print_flag = true;
    annotateWireDelaysOnTrees(buffered_trees_post_);
    printTreeToCSV(buffered_trees_post_, post_filename, buffered_tree_print_flag);
    std::cout << "buffered_trees_post saved to " << post_filename << std::endl;
  }

  if (buffered_trees_mid_.size() > 0) {
    bool buffered_tree_print_flag = false;
    annotateWireDelaysOnTrees(buffered_trees_mid_);
    printTreeToCSV(buffered_trees_mid_, mid_filename, buffered_tree_print_flag);
    std::cout << "buffered_trees_mid saved to " << mid_filename << std::endl;
  }

  // printTreeToCSV(buffered_trees_pre_, "buffered_trees_pre.csv");
  // printTreeToCSV(buffered_trees_post_, "buffered_trees_post.csv");
}

// obtain the current time stamp
std::string RepairDesign::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);
    
    std::ostringstream oss;
    oss << (1900 + local_time->tm_year) 
        << (local_time->tm_mon + 1) 
        << local_time->tm_mday << "_" 
        << local_time->tm_hour 
        << local_time->tm_min 
        << local_time->tm_sec;
    
    return oss.str();
}

std::string RepairDesign::generateUniqueFilename(const std::string& base_name) {
  const std::string default_directory = "/home/dgx_projects/MLBuf/virtual_buffer/DATA/buffered_trees_data/";  
  
  // User-defined directory
  const char* env_dir = std::getenv("SAVE_DIR");
  std::string save_directory = (env_dir != nullptr) ? std::string(env_dir) : default_directory;
  
  // Make sure the directory name is ended with '/'
  if (save_directory.back() != '/') {
    save_directory += "/";
  }
  
  if (!std::filesystem::exists(save_directory)) {
      std::filesystem::create_directories(save_directory);
  }
  return save_directory + base_name + "_" + getCurrentTimestamp() + ".csv";
}


// ----------------------------------------------------------------------------
// BufferedTreeNode
// ----------------------------------------------------------------------------

std::string bufferedTreeNodeTypeToString(BufferedTreeNodeType type)
{
  switch (type) {
    case BufferedTreeNodeType::DRIVER:
      return "Driver";
    case BufferedTreeNodeType::SINK:
      return "Sink";
    case BufferedTreeNodeType::BUFFER:
      return "Buffer";
  }
}

// ----------------------------------------------------------------------------
// BufferedTree
// ----------------------------------------------------------------------------
// YL: Deep copy constructor
BufferedTree::BufferedTree(const BufferedTree& other)
{
  netId_ = other.netId_;
  net_   = other.net_;   // You may or may not want to copy the same sta::Net*.
  root_  = copyNode(other.root_);
}

void BufferedTree::setRoot(BufferedTreeNodePtr root_node)
{
  root_ = std::move(root_node);
}

BufferedTreeNodePtr BufferedTree::getRoot() const
{
  return root_;
}

//YL
sta::Net* BufferedTree::getNet() const 
{ 
  return net_; 
}

void BufferedTree::printTree() const
{
  // Print the tree in a pre-order DFS traversal.
  int node_id = 0;
  std::queue<std::pair<BufferedTreeNodePtr, int>> q; // Queue now stores a pair of node and its depth
  q.push({root_, 0}); // Root node is at depth 0

  // std::string net_name_str;
  // net_name_str = net_->getName().c_str();
  // std::cout << "Net_name: " << net_name_str << "\t";

  while (!q.empty()) {
      auto [node, depth] = q.front(); // Unpack the node and its depth
      q.pop();
      node->node_id = node_id;
      node_id++;
      for (const auto& child : node->children) {
          q.push({child, depth + 1}); // Increment depth for children
      }
}

  // Reset the queue and print the nodes.
  auto printNode = [](BufferedTreeNodePtr node, int depth) {
    std::cout << "Depth: " << depth << "\t";
    std::cout << "Node ID: " << node->node_id << "\t";
    std::cout << "Node type: " << bufferedTreeNodeTypeToString(node->node_type) << "\t";
    std::cout << "Pin name: " << node->pin_name << "\t";
    std::cout << "Location: ( " << node->x << " , " << node->y << " )\t";
    std::cout << "Input slew: " << node->input_slew << "\t";
    std::cout << "Output slew: " << node->output_slew << "\t";
    std::cout << "Input cap: " << node->input_cap << "\t";
    std::cout << "Output cap: " << node->output_cap << "\t";
    std::cout << "Max output cap: " << node->max_output_cap << "\t";
    std::cout << "Fanout: " << node->fanout << "\t";
    std::cout << "Parent: " << (node->parent ? std::to_string(node->parent->node_id) : "None") << "\t";
    std::cout << "#Children: " << node->children.size() << "\n";
  };

  q.push({root_, 0}); // Reset the queue with root node and depth 0
  while (!q.empty()) {
    auto [node, depth] = q.front(); // Unpack the node and its depth
    q.pop();
    printNode(node, depth); 
    for (const auto& child : node->children) {
        q.push({child, depth + 1}); // Increment depth for children
    }
  }
}

void BufferedTree::clear()
{
  // Disconnect all nodes from the tree.
  std::queue<BufferedTreeNodePtr> q;
  q.push(root_);
  while (!q.empty()) {
    auto node = q.front();
    q.pop();

    for (const auto& child : node->children) {
      q.push(child);
    }

    node->children.clear();
    node->parent = nullptr;
  }
  root_ = nullptr;
}


// YL: Private helper to recursively copy a node
BufferedTreeNodePtr BufferedTree::copyNode(const BufferedTreeNodePtr &node) const
{
  if (!node) {
    return nullptr;
  }

  // Allocate a new BufferedTreeNode by copying all data from 'node'
  auto new_node = std::make_shared<BufferedTreeNode>(*node);

  // Clear children in the new node so we can re-populate them
  new_node->children.clear();

  // Recursively copy children
  for (const auto &child : node->children) {
    auto new_child = copyNode(child);
    if (new_child) {
      new_child->parent = new_node;
      new_node->children.push_back(new_child);
    }
  }

  // Return newly created node
  return new_node;
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
