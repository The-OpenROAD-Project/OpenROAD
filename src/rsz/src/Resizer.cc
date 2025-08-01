// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rsz/Resizer.hh"

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "AbstractSteinerRenderer.h"
#include "BufferMove.hh"
#include "BufferedNet.hh"
#include "CloneMove.hh"
#include "ConcreteSwapArithModules.hh"
#include "Rebuffer.hh"
#include "RecoverPower.hh"
#include "RepairDesign.hh"
#include "RepairHold.hh"
#include "RepairSetup.hh"
#include "ResizerObserver.hh"
#include "SizeDownMove.hh"
#include "SizeUpMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"
#include "UnbufferMove.hh"
#include "boost/multi_array.hpp"
#include "db_sta/dbNetwork.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Corner.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/LeakagePower.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Parasitics.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/StaMain.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/scope.h"

// http://vlsicad.eecs.umich.edu/BK/Slots/cache/dropzone.tamu.edu/~zhuoli/GSRC/fast_buffer_insertion.html

namespace rsz {

using std::abs;
using std::map;
using std::max;
using std::min;
using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

using odb::dbBoolProperty;
using odb::dbBox;
using odb::dbDoubleProperty;
using odb::dbInst;
using odb::dbMaster;
using odb::dbPlacementStatus;
using odb::dbSet;

using sta::ConcreteLibraryCellIterator;
using sta::FindNetDrvrLoads;
using sta::FuncExpr;
using sta::InstancePinIterator;
using sta::LeafInstanceIterator;
using sta::Level;
using sta::LibertyCellIterator;
using sta::LibertyLibraryIterator;
using sta::NetConnectedPinIterator;
using sta::NetIterator;
using sta::NetPinIterator;
using sta::NetTermIterator;
using sta::NetworkEdit;
using sta::Port;
using sta::stringLess;
using sta::TimingArcSet;
using sta::TimingArcSetSeq;
using sta::TimingRole;

using sta::ArcDcalcResult;
using sta::ArcDelayCalc;
using sta::BfsBkwdIterator;
using sta::BfsFwdIterator;
using sta::BfsIndex;
using sta::ClkArrivalSearchPred;
using sta::Clock;
using sta::Corners;
using sta::Edge;
using sta::fuzzyGreaterEqual;
using sta::INF;
using sta::InputDrive;
using sta::LoadPinIndexMap;
using sta::PinConnectedPinIterator;
using sta::Sdc;
using sta::SearchPredNonReg2;
using sta::VertexIterator;
using sta::VertexOutEdgeIterator;

using sta::BufferUse;
using sta::CLOCK;
using sta::LeakagePower;
using sta::LeakagePowerSeq;

Resizer::Resizer()
    : recover_power_(std::make_unique<RecoverPower>(this)),
      repair_design_(std::make_unique<RepairDesign>(this)),
      repair_setup_(std::make_unique<RepairSetup>(this)),
      repair_hold_(std::make_unique<RepairHold>(this)),
      swap_arith_modules_(std::make_unique<ConcreteSwapArithModules>(this)),
      rebuffer_(std::make_unique<Rebuffer>(this)),
      wire_signal_res_(0.0),
      wire_signal_cap_(0.0),
      wire_clk_res_(0.0),
      wire_clk_cap_(0.0),
      tgt_slews_{0.0, 0.0}
{
}

Resizer::~Resizer() = default;

void Resizer::init(Logger* logger,
                   dbDatabase* db,
                   dbSta* sta,
                   SteinerTreeBuilder* stt_builder,
                   GlobalRouter* global_router,
                   dpl::Opendp* opendp,
                   std::unique_ptr<AbstractSteinerRenderer> steiner_renderer)
{
  opendp_ = opendp;
  logger_ = logger;
  db_ = db;
  block_ = nullptr;
  dbStaState::init(sta);
  stt_builder_ = stt_builder;
  global_router_ = global_router;
  incr_groute_ = nullptr;
  db_network_ = sta->getDbNetwork();
  resized_multi_output_insts_ = InstanceSet(db_network_);
  steiner_renderer_ = std::move(steiner_renderer);
  db_cbk_ = std::make_unique<OdbCallBack>(this, network_, db_network_);

  db_network_->addObserver(this);

  buffer_move_ = std::make_unique<BufferMove>(this);
  clone_move_ = std::make_unique<CloneMove>(this);
  size_down_move_ = std::make_unique<SizeDownMove>(this);
  size_up_move_ = std::make_unique<SizeUpMove>(this);
  split_load_move_ = std::make_unique<SplitLoadMove>(this);
  swap_pins_move_ = std::make_unique<SwapPinsMove>(this);
  unbuffer_move_ = std::make_unique<UnbufferMove>(this);
}

////////////////////////////////////////////////////////////////

double Resizer::coreArea() const
{
  return dbuToMeters(core_.dx()) * dbuToMeters(core_.dy());
}

double Resizer::utilization()
{
  initDesignArea();
  double core_area = coreArea();
  if (core_area > 0.0) {
    return design_area_ / core_area;
  }
  return 1.0;
}

double Resizer::maxArea() const
{
  return max_area_;
}

////////////////////////////////////////////////////////////////

class VertexLevelLess
{
 public:
  VertexLevelLess(const Network* network);
  bool operator()(const Vertex* vertex1, const Vertex* vertex2) const;

 protected:
  const Network* network_;
};

VertexLevelLess::VertexLevelLess(const Network* network) : network_(network)
{
}

bool VertexLevelLess::operator()(const Vertex* vertex1,
                                 const Vertex* vertex2) const
{
  Level level1 = vertex1->level();
  Level level2 = vertex2->level();
  return (level1 < level2)
         || (level1 == level2
             // Break ties for stable results.
             && stringLess(network_->pathName(vertex1->pin()),
                           network_->pathName(vertex2->pin())));
}

////////////////////////////////////////////////////////////////
constexpr static double double_equal_tolerance
    = std::numeric_limits<double>::epsilon() * 10;
bool rszFuzzyEqual(double v1, double v2)
{
  if (v1 == v2) {
    return true;
  }
  if (v1 == 0.0 || v2 == 0.0) {
    return std::abs(v1 - v2) < double_equal_tolerance;
  }
  return std::abs(v1 - v2) < 1E-9 * std::max(std::abs(v1), std::abs(v2));
}

// block_ indicates core_, design_area_, db_network_ etc valid.
void Resizer::initBlock()
{
  if (db_->getChip() == nullptr) {
    logger_->error(RSZ, 162, "Database does not have a loaded design");
  }
  block_ = db_->getChip()->getBlock();
  if (block_ == nullptr) {
    logger_->error(RSZ, 163, "Database has no block");
  }
  core_ = block_->getCoreArea();
  core_exists_ = !(core_.xMin() == 0 && core_.xMax() == 0 && core_.yMin() == 0
                   && core_.yMax() == 0);
  dbu_ = db_->getTech()->getDbUnitsPerMicron();

  // Apply sizing restrictions
  dbDoubleProperty* area_prop
      = dbDoubleProperty::find(block_, "limit_sizing_area");
  if (area_prop) {
    if (sizing_area_limit_
        && !rszFuzzyEqual(*sizing_area_limit_, area_prop->getValue())) {
      swappable_cells_cache_.clear();
    }
    sizing_area_limit_ = area_prop->getValue();
    default_sizing_area_limit_set_ = false;
  } else {
    if (default_sizing_area_limit_set_ && sizing_area_limit_) {
      dbDoubleProperty::create(
          block_, "limit_sizing_area", *sizing_area_limit_);
    } else {
      if (sizing_area_limit_) {
        swappable_cells_cache_.clear();
      }
      sizing_area_limit_.reset();
    }
  }
  dbDoubleProperty* leakage_prop
      = dbDoubleProperty::find(block_, "limit_sizing_leakage");
  if (leakage_prop) {
    if (sizing_leakage_limit_
        && !rszFuzzyEqual(*sizing_leakage_limit_, leakage_prop->getValue())) {
      swappable_cells_cache_.clear();
    }
    sizing_leakage_limit_ = leakage_prop->getValue();
    default_sizing_leakage_limit_set_ = false;
  } else {
    if (default_sizing_leakage_limit_set_ && sizing_leakage_limit_) {
      dbDoubleProperty::create(
          block_, "limit_sizing_leakage", *sizing_leakage_limit_);
    } else {
      if (sizing_leakage_limit_) {
        swappable_cells_cache_.clear();
      }
      sizing_leakage_limit_.reset();
    }
  }
  dbBoolProperty* site_prop = dbBoolProperty::find(block_, "keep_sizing_site");
  if (site_prop) {
    if (sizing_keep_site_ != site_prop->getValue()) {
      swappable_cells_cache_.clear();
    }
    sizing_keep_site_ = site_prop->getValue();
  } else {
    if (sizing_keep_site_ != false) {
      swappable_cells_cache_.clear();
    }
    sizing_keep_site_ = false;
  }
  dbBoolProperty* vt_prop = dbBoolProperty::find(block_, "keep_sizing_vt");
  if (vt_prop) {
    if (sizing_keep_vt_ != vt_prop->getValue()) {
      swappable_cells_cache_.clear();
    }
    sizing_keep_vt_ = vt_prop->getValue();
  } else {
    if (sizing_keep_vt_ != false) {
      swappable_cells_cache_.clear();
    }
    sizing_keep_vt_ = false;
  }

  dbDoubleProperty* cap_ratio_prop
      = dbDoubleProperty::find(block_, "early_sizing_cap_ratio");
  if (cap_ratio_prop) {
    sizing_cap_ratio_ = cap_ratio_prop->getValue();
  } else {
    sizing_cap_ratio_ = default_sizing_cap_ratio_;
  }

  dbDoubleProperty* buffer_cap_ratio_prop
      = dbDoubleProperty::find(block_, "early_buffer_sizing_cap_ratio");
  if (buffer_cap_ratio_prop) {
    buffer_sizing_cap_ratio_ = buffer_cap_ratio_prop->getValue();
  } else {
    buffer_sizing_cap_ratio_ = default_buffer_sizing_cap_ratio_;
  }

  dbBoolProperty* disable_pruning_prop
      = dbBoolProperty::find(block_, "disable_buffer_pruning");
  if (disable_pruning_prop) {
    if (disable_buffer_pruning_ != disable_pruning_prop->getValue()) {
      swappable_cells_cache_.clear();
      buffer_cells_.clear();
    }
    disable_buffer_pruning_ = disable_pruning_prop->getValue();
  }
  // Temporary WA: default is to disable buffer pruning
#ifdef TEMP
  else {
    if (disable_buffer_pruning_ != false) {
      swappable_cells_cache_.clear();
      buffer_cells_.clear();
    }
    disable_buffer_pruning_ = false;
  }
#endif
}

void Resizer::init()
{
  initDesignArea();
  sta_->ensureLevelized();
  graph_ = sta_->graph();
  swappable_cells_cache_.clear();
}

// remove all buffers if no buffers are specified
void Resizer::removeBuffers(sta::InstanceSeq insts)
{
  initBlock();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();
  IncrementalParasiticsGuard guard(this);

  if (insts.empty()) {
    // remove all the buffers
    for (dbInst* db_inst : block_->getInsts()) {
      Instance* buffer = db_network_->dbToSta(db_inst);
      if (unbuffer_move_->removeBufferIfPossible(buffer,
                                                 /* honor dont touch */ true)) {
      }
    }
  } else {
    // remove only select buffers specified by user
    InstanceSeq::Iterator inst_iter(insts);
    while (inst_iter.hasNext()) {
      Instance* buffer = const_cast<Instance*>(inst_iter.next());
      if (unbuffer_move_->removeBufferIfPossible(
              buffer, /* don't honor dont touch */ false)) {
      } else {
        logger_->warn(
            RSZ,
            97,
            "Instance {} cannot be removed because it is not a buffer, "
            "functions as a feedthrough port buffer, or is constrained",
            db_network_->name(buffer));
      }
    }
  }
  unbuffer_move_->commitMoves();
  level_drvr_vertices_valid_ = false;
  logger_->info(RSZ, 26, "Removed {} buffers.", unbuffer_move_->numMoves());
}

void Resizer::unbufferNet(Net* net)
{
  sta::InstanceSeq insts;
  std::vector<Net*> queue = {net};

  while (!queue.empty()) {
    Net* net_ = queue.back();
    sta::NetConnectedPinIterator* pin_iter
        = network_->connectedPinIterator(net_);
    queue.pop_back();

    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      const LibertyPort* port = network_->libertyPort(pin);
      if (port && port->libertyCell()->isBuffer()) {
        LibertyPort *in, *out;
        port->libertyCell()->bufferPorts(in, out);

        if (port == in) {
          const Instance* inst = network_->instance(pin);
          insts.emplace_back(inst);
          const Pin* out_pin = network_->findPin(inst, out);
          if (out_pin) {
            queue.push_back(network_->net(out_pin));
          }
        }
      }
    }

    delete pin_iter;
  }

  removeBuffers(insts);
}

void Resizer::ensureLevelDrvrVertices()
{
  if (!level_drvr_vertices_valid_) {
    level_drvr_vertices_.clear();
    VertexIterator vertex_iter(graph_);
    while (vertex_iter.hasNext()) {
      Vertex* vertex = vertex_iter.next();
      if (vertex->isDriver(network_)) {
        level_drvr_vertices_.emplace_back(vertex);
      }
    }
    sort(level_drvr_vertices_, VertexLevelLess(network_));
    level_drvr_vertices_valid_ = true;
  }
}

void Resizer::balanceBin(const vector<odb::dbInst*>& bin,
                         const std::set<odb::dbSite*>& base_sites)
{
  // Maps sites to the total width of all instances using that site
  map<odb::dbSite*, uint64_t> sites;
  uint64_t total_width = 0;
  for (auto inst : bin) {
    auto master = inst->getMaster();
    sites[master->getSite()] += master->getWidth();
    total_width += master->getWidth();
  }

  // Add empty base_sites
  for (odb::dbSite* site : base_sites) {
    if (sites.find(site) == sites.end()) {
      sites[site] = 0;
    }
  }

  const double imbalance_factor = 0.8;
  const double target_lower_width
      = imbalance_factor * total_width / sites.size();
  for (auto [site, width] : sites) {
    for (auto inst : bin) {
      if (width >= target_lower_width) {
        break;
      }
      if (inst->getMaster()->getSite() == site) {
        continue;
      }
      if (inst->getPlacementStatus().isFixed() || inst->isDoNotTouch()) {
        continue;
      }
      Instance* sta_inst = db_network_->dbToSta(inst);
      LibertyCell* cell = network_->libertyCell(sta_inst);
      dbMaster* master = db_network_->staToDb(cell);
      LibertyCellSeq swappable_cells = getSwappableCells(cell);
      for (LibertyCell* target_cell : swappable_cells) {
        dbMaster* target_master = db_network_->staToDb(target_cell);
        // Pick a cell that has the matching site, the same VT type
        // and equal or less drive resistance.  swappable_cells are
        // sorted in decreasing order of drive resistance.
        if (target_master->getSite() == site
            && cellVTType(target_master).first == cellVTType(master).first
            && sta::fuzzyLessEqual(cellDriveResistance(target_cell),
                                   cellDriveResistance(cell))) {
          inst->swapMaster(target_master);
          width += target_master->getWidth();
          break;
        }
      }
    }
  }
}

void Resizer::balanceRowUsage()
{
  initBlock();
  makeEquivCells();

  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  const int num_bins = 10;
  using Insts = vector<odb::dbInst*>;
  using InstGrid = boost::multi_array<Insts, 2>;
  InstGrid grid(boost::extents[num_bins][num_bins]);

  const int core_width = core_.dx();
  const int core_height = core_.dy();
  const int x_step = core_width / num_bins + 1;
  const int y_step = core_height / num_bins + 1;

  std::set<odb::dbSite*> base_sites;
  for (odb::dbRow* row : block_->getRows()) {
    odb::dbSite* site = row->getSite();
    if (site->hasRowPattern()) {
      continue;
    }
    base_sites.insert(site);
  }

  for (auto inst : block_->getInsts()) {
    auto master = inst->getMaster();
    auto site = master->getSite();
    // Ignore multi-height cells for now
    if (site && site->hasRowPattern()) {
      continue;
    }

    const Point origin = inst->getOrigin();
    const int x_bin = (origin.x() - core_.xMin()) / x_step;
    const int y_bin = (origin.y() - core_.yMin()) / y_step;
    grid[x_bin][y_bin].push_back(inst);
  }

  for (int x = 0; x < num_bins; ++x) {
    for (int y = 0; y < num_bins; ++y) {
      balanceBin(grid[x][y], base_sites);
    }
  }
}

////////////////////////////////////////////////////////////////

// Inspect the timing arcs on a buffer size to find the capacitance
// points on the piecewise linear delay model
static void populateBufferCapTestPoints(LibertyCell* cell,
                                        LibertyPort* in,
                                        LibertyPort* out,
                                        std::vector<float>& points)
{
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    const TimingRole* role = arc_set->role();
    if (role == TimingRole::combinational() && arc_set->from() == in
        && arc_set->to() == out) {
      for (TimingArc* arc : arc_set->arcs()) {
        auto model = dynamic_cast<sta::GateTableModel*>(arc->model());
        if (model) {
          auto dm = model->delayModel();
          for (const sta::TableAxis* axis :
               {dm->axis1(), dm->axis2(), dm->axis3()}) {
            if (axis
                && axis->variable()
                       == sta::TableAxisVariable::
                           total_output_net_capacitance) {
              points.insert(
                  points.end(), axis->values()->begin(), axis->values()->end());
            }
          }
        }
      }
    }
  }
}

bool Resizer::bufferSizeOutmatched(LibertyCell* worse,
                                   LibertyCell* better,
                                   const float max_drive_resist)
{
  LibertyPort *win, *wout, *bin, *bout;
  worse->bufferPorts(win, wout);
  better->bufferPorts(bin, bout);

  float extra_input_cap
      = std::max(bin->capacitance() - win->capacitance(), 0.0f);
  float delay_penalty = max_drive_resist * extra_input_cap;
  float wlimit = maxLoad(network_->cell(worse));

  std::vector<float> test_points;
  populateBufferCapTestPoints(worse, win, wout, test_points);
  populateBufferCapTestPoints(better, bin, bout, test_points);

  if (test_points.empty()) {
    return false;
  }

  std::sort(test_points.begin(), test_points.end());
  auto last = std::unique(test_points.begin(), test_points.end());
  test_points.erase(last, test_points.end());

  // Ignore cell timing above this C_load/C_in ratio as that is far off
  // from the target ratio we use in sizing (this cut-off helps prune the
  // list of candidate buffer sizes better)
  constexpr float cap_ratio_cutoff = 20.0f;

  for (auto p : test_points) {
    if ((wlimit == 0 || p <= wlimit)
        && p < std::min(bin->capacitance(), win->capacitance())
                   * cap_ratio_cutoff) {
      float bd = bufferDelay(better, p, tgt_slew_dcalc_ap_);
      float wd = bufferDelay(worse, p, tgt_slew_dcalc_ap_);
      if (bd + delay_penalty > wd) {
        debugPrint(logger_,
                   RSZ,
                   "gain_buffering",
                   3,
                   "{} is better than {} at {}: {} vs {} + {}",
                   worse->name(),
                   better->name(),
                   units_->capacitanceUnit()->asString(p),
                   units_->timeUnit()->asString(wd),
                   units_->timeUnit()->asString(bd),
                   units_->timeUnit()->asString(delay_penalty));
        return false;
      }
    }
  }

  // We haven't found a load amount (within bounds) for which `worse` wouldn't
  // be slower than `better`
  return true;
}

void Resizer::findFastBuffers()
{
  if (!buffer_fast_sizes_.empty()) {
    return;
  }

  float R_max = bufferDriveResistance(buffer_lowest_drive_);
  LibertyCellSeq sizes_by_inp_cap;
  sizes_by_inp_cap = buffer_cells_;
  sort(sizes_by_inp_cap,
       [](const LibertyCell* buffer1, const LibertyCell* buffer2) {
         LibertyPort *port1, *port2, *scratch;
         buffer1->bufferPorts(port1, scratch);
         buffer2->bufferPorts(port2, scratch);
         return port1->capacitance() < port2->capacitance();
       });

  LibertyCellSeq fast_buffers;
  for (auto size : sizes_by_inp_cap) {
    if (fast_buffers.empty()) {
      fast_buffers.push_back(size);
    } else {
      if (!bufferSizeOutmatched(size, fast_buffers.back(), R_max)) {
        while (!fast_buffers.empty()
               && bufferSizeOutmatched(fast_buffers.back(), size, R_max)) {
          debugPrint(logger_,
                     RSZ,
                     "resizer",
                     2,
                     "{} trumps {}",
                     size->name(),
                     fast_buffers.back()->name());
          fast_buffers.pop_back();
        }
        fast_buffers.push_back(size);
      } else {
        debugPrint(logger_,
                   RSZ,
                   "resizer",
                   2,
                   "{} trumps {}",
                   fast_buffers.back()->name(),
                   size->name());
      }
    }
  }

  debugPrint(logger_, RSZ, "resizer", 1, "pre-selected buffers:");
  for (auto size : fast_buffers) {
    debugPrint(logger_, RSZ, "resizer", 1, " - {}", size->name());
  }

  buffer_fast_sizes_ = {fast_buffers.begin(), fast_buffers.end()};
}

void Resizer::reportFastBufferSizes()
{
  resizePreamble();

  logger_->report("\nFast Buffer Report:");
  logger_->report("There are {} fast buffers", buffer_fast_sizes_.size());
  logger_->report("{:->80}", "");
  logger_->report(
      "Cell                                        Area  Input  Intrinsic "
      "Drive ");
  logger_->report(
      "                                                   Cap    Delay    Res");
  logger_->report("{:->80}", "");

  for (auto size : buffer_fast_sizes_) {
    LibertyPort *in, *out;
    size->bufferPorts(in, out);
    logger_->report("{:<41} {:>7.1f} {:>7.1e} {:>7.1e} {:>7.1f}",
                    size->name(),
                    size->area(),
                    in->capacitance(),
                    out->intrinsicDelay(sta_),
                    out->driveResistance());
  }
  logger_->report("{:->80}", "");
}

#define debugRDPrint1(format_str, ...) \
  debugPrint(logger_, utl::RSZ, "resizer", 1, format_str, ##__VA_ARGS__)
// debugPrint for replace_design level 2
#define debugRDPrint2(format_str, ...) \
  debugPrint(logger_, utl::RSZ, "resizer", 2, format_str, ##__VA_ARGS__)

void Resizer::findBuffers()
{
  if (disable_buffer_pruning_) {
    findBuffersNoPruning();
    return;
  }

  if (!buffer_cells_.empty()) {
    return;
  }

  LibertyCellSeq buffer_list;
  LibraryAnalysisData lib_data;
  getBufferList(buffer_list, lib_data);

  // Pick the right cell footprint to avoid delay cells
  std::string best_footprint;
  if (lib_data.cells_by_footprint.size() > 1) {
    for (const auto& [footprint_type, count] : lib_data.cells_by_footprint) {
      float ratio = (float) count / buffer_list.size();
      // Some PDK libs have a distinct footprint for each drive strength
      // Pick a footprint that dominates
      if (ratio > 0.5) {
        best_footprint = footprint_type;
        debugRDPrint2("findBuffers: Best footprint is {}", footprint_type);
        break;
      }
    }
  }

  // Pick the second most leaky VT for multiple VTs
  int best_vt_index = -1;
  int num_vt = lib_data.sorted_vt_categories.size();
  if (num_vt > 1) {
    const std::pair<VTCategory, VTLeakageStats> second_leakiest_vt
        = lib_data.sorted_vt_categories[num_vt - 2];
    best_vt_index = second_leakiest_vt.first.vt_index;
    debugRDPrint2("findBuffers: Best VT index is {} [{}] among {} VTs",
                  best_vt_index,
                  second_leakiest_vt.first.vt_name,
                  num_vt);
  } else if (num_vt == 1) {
    const std::pair<VTCategory, VTLeakageStats> only_vt
        = lib_data.sorted_vt_categories[0];
    best_vt_index = only_vt.first.vt_index;
    debugRDPrint2("findBuffers: Best VT index is {} among 1 VT", best_vt_index);
  } else {
    debugRDPrint2("findBuffers: Best VT index is {} among 0 VT", best_vt_index);
  }

  // There may be multiple cell sites like short, tall, short+tall, etc.
  // Pick two sites that are the most dominant.  Most likely, short and tall.
  std::vector<std::pair<odb::dbSite*, float>> site_list;
  site_list.reserve(lib_data.cells_by_site.size());
  for (const auto& [site_type, count] : lib_data.cells_by_site) {
    site_list.emplace_back(site_type, (float) count / buffer_list.size());
  }
  std::sort(site_list.begin(),
            site_list.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
  int num_best_sites = std::min(2, (int) lib_data.cells_by_site.size());
  for (int i = 0; i < num_best_sites; i++) {
    debugRDPrint2("findBuffers: {} is a dominant site",
                  site_list[i].first->getName());
  }

  LibertyCellSeq new_buffer_list;
  for (LibertyCell* buffer : buffer_list) {
    const char* footprint = buffer->footprint();
    odb::dbMaster* master = db_network_->staToDb(buffer);
    auto vt_type = cellVTType(master);
    bool footprint_matches
        = best_footprint.empty() || (footprint && best_footprint == footprint);
    bool vt_matches = best_vt_index == -1 || vt_type.first == best_vt_index;

    if (footprint_matches && vt_matches) {
      new_buffer_list.emplace_back(buffer);
      debugRDPrint2("findBuffers: Adding {} to new buffer list",
                    buffer->name());
    } else {
      debugRDPrint2(
          "findBuffers: {} is not added because VT {} and CF {} [VT index = "
          "{}, best VT index = {}]",
          buffer->name(),
          (vt_matches ? "matches" : "doesn't match"),
          (footprint_matches ? "matches" : "doesn't match"),
          vt_type.first,
          best_vt_index);
    }
  }
  debugRDPrint2("findBuffers: new buffer list has {} buffers",
                new_buffer_list.size());

  // The buffer list is already sorted in ascending order of drive resistance.
  // Divide buffers into 5 buckets based on drive resistance.
  // Each bucket is a proxy for drive strength group.
  // We want to pick 2 best buffers from each bucket using drive resistance *
  // c_in.  The lower the RC product, the better.
  const int num_buckets = 5;
  const int bucket_size = new_buffer_list.size() / num_buckets;
  const int remainder = new_buffer_list.size() % num_buckets;

  LibertyCellSeq final_buffer_list;
  for (int bucket = 0; bucket < num_buckets; bucket++) {
    // Calculate bucket boundaries
    int start_idx = bucket * bucket_size + std::min(bucket, remainder);
    int end_idx = start_idx + bucket_size + (bucket < remainder ? 1 : 0);

    // Create a vector for this bucket with drive_res * input_cap values
    std::vector<std::pair<LibertyCell*, float>> bucket_buffers;
    bucket_buffers.reserve(bucket_size);

    for (int i = start_idx; i < end_idx && i < new_buffer_list.size(); i++) {
      LibertyCell* buffer = new_buffer_list[i];
      LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      float metric = bufferDriveResistance(buffer) * input->capacitance();
      bucket_buffers.emplace_back(buffer, metric);
    }

    // Sort this bucket by drive_res * input_cap (ascending)
    std::sort(bucket_buffers.begin(),
              bucket_buffers.end(),
              [](const auto& a, const auto& b) {
                return a.second < b.second;  // Compare by R * C
              });

    // Select up to 2 best buffers from this bucket
    if (num_best_sites == 1) {
      int buffers_to_select
          = std::min(2, static_cast<int>(bucket_buffers.size()));
      for (int i = 0; i < buffers_to_select; i++) {
        final_buffer_list.emplace_back(bucket_buffers[i].first);
        debugRDPrint2(
            "findBuffers: Buffer {} with RC={:.2e} is selected for bucket "
            "{}",
            bucket_buffers[i].first->name(),
            bucket_buffers[i].second,
            bucket);
      }
    } else {
      // Try to choose one for each dominant site
      // It's possible that there are no buffers matching the sites in this
      // bucket
      for (int site_idx = 0; site_idx < 2 && site_idx < site_list.size();
           site_idx++) {
        for (const auto& pair : bucket_buffers) {
          odb::dbMaster* master = db_network_->staToDb(pair.first);
          if (master->getSite() == site_list[site_idx].first) {
            final_buffer_list.emplace_back(pair.first);
            debugRDPrint2(
                "findBuffers: Buffer {} with RC={:.2e} is selected for "
                "bucket {} and site "
                "{}",
                pair.first->name(),
                pair.second,
                bucket,
                master->getSite()->getName());
            break;
          }
        }
      }
    }
  }

  debugRDPrint2(
      "findBuffers: Selected {} buffers total from {} original buffers in "
      "{} buckets",
      final_buffer_list.size(),
      new_buffer_list.size(),
      num_buckets);

  // final_buffer_list now contains up to 10 buffers (2 from each of 5 buckets)
  buffer_cells_.assign(final_buffer_list.begin(), final_buffer_list.end());

  if (buffer_cells_.empty()) {
    logger_->error(RSZ, 22, "no buffers found.");
  } else {
    // find the buffer with the largest drive resistance
    buffer_lowest_drive_ = buffer_cells_.back();
  }
}

void Resizer::findBuffersNoPruning()
{
  if (buffer_cells_.empty()) {
    LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();

    while (lib_iter->hasNext()) {
      LibertyLibrary* lib = lib_iter->next();

      for (LibertyCell* buffer : *lib->buffers()) {
        if (exclude_clock_buffers_) {
          BufferUse buffer_use = sta_->getBufferUse(buffer);

          if (buffer_use == CLOCK) {
            continue;
          }
        }

        if (!dontUse(buffer) && isLinkCell(buffer)) {
          buffer_cells_.emplace_back(buffer);
        }
      }
    }

    delete lib_iter;

    if (buffer_cells_.empty()) {
      logger_->error(RSZ, 52, "no buffers found.");
    } else {
      sort(buffer_cells_,
           [this](const LibertyCell* buffer1, const LibertyCell* buffer2) {
             return bufferDriveResistance(buffer1)
                    > bufferDriveResistance(buffer2);
           });

      buffer_lowest_drive_ = buffer_cells_[0];
    }
  }
}

LibertyCell* Resizer::selectBufferCell(LibertyCell* user_buffer_cell)
{
  // Prefer user-specified buffer cell if provided.
  if (user_buffer_cell) {
    return user_buffer_cell;
  }

  // Otherwise, find the weakest buffer with the lowest drive resistance.
  findBuffers();  // updates buffer_lowest_drive_

  // No buffer?
  if (buffer_lowest_drive_ == nullptr) {
    logger_->error(RSZ, 41, "No buffers found.");
  }

  return buffer_lowest_drive_;
}

bool Resizer::isLinkCell(LibertyCell* cell) const
{
  return network_->findLibertyCell(cell->name()) == cell;
}

////////////////////////////////////////////////////////////////

void Resizer::bufferInputs(LibertyCell* buffer_cell, bool verbose)
{
  init();

  // Use buffer_cell. If it is null, find the buffer w/ lowest drive resistance.
  LibertyCell* selected_buffer_cell = selectBufferCell(buffer_cell);
  if (verbose) {
    logger_->info(RSZ,
                  29,
                  "Start input port buffering with {}.",
                  selected_buffer_cell->name());
  }

  sta_->ensureClkNetwork();
  inserted_buffer_count_ = 0;
  buffer_moved_into_core_ = false;

  {
    IncrementalParasiticsGuard guard(this);
    std::unique_ptr<InstancePinIterator> port_iter(
        network_->pinIterator(network_->topInstance()));
    while (port_iter->hasNext()) {
      Pin* pin = port_iter->next();
      Vertex* vertex = graph_->pinDrvrVertex(pin);
      Net* net = network_->net(network_->term(pin));

      if (network_->direction(pin)->isInput() && !dontTouch(net)
          && !vertex->isConstant()
          && !sta_->isClock(pin)
          // Hands off special nets.
          && !db_network_->isSpecial(net) && hasPins(net)) {
        // repair_design will resize to target slew.
        bufferInput(pin, selected_buffer_cell, verbose);
      }
    }
  }

  logger_->info(RSZ,
                27,
                "Inserted {} {} input buffers.",
                inserted_buffer_count_,
                selected_buffer_cell->name());

  if (inserted_buffer_count_ > 0) {
    level_drvr_vertices_valid_ = false;
  }
}

bool Resizer::hasPins(Net* net)
{
  NetPinIterator* pin_iter = db_network_->pinIterator(net);
  bool has_pins = pin_iter->hasNext();
  delete pin_iter;
  return has_pins;
}

void Resizer::getPins(Net* net, PinVector& pins) const
{
  auto pin_iter = network_->pinIterator(net);
  while (pin_iter->hasNext()) {
    pins.emplace_back(pin_iter->next());
  }
  delete pin_iter;
}

void Resizer::getPins(Instance* inst, PinVector& pins) const
{
  auto pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    pins.emplace_back(pin_iter->next());
  }
  delete pin_iter;
}

void Resizer::SwapNetNames(odb::dbITerm* iterm_to, odb::dbITerm* iterm_from)
{
  if (iterm_to && iterm_from) {
    //
    // The concept of this function is we are moving the name of the net
    // in the iterm_from to the iterm_to.  We preferentially use
    // the modnet name, if present.
    //
    dbNet* to_db_net = iterm_to->getNet();
    odb::dbModNet* to_mod_net = iterm_to->getModNet();

    odb::dbModNet* from_mod_net = iterm_from->getModNet();
    dbNet* from_db_net = iterm_from->getNet();

    std::string required_name
        = from_mod_net ? from_mod_net->getName() : from_db_net->getName();
    std::string to_name
        = to_mod_net ? to_mod_net->getName() : to_db_net->getName();

    if (from_mod_net && to_mod_net) {
      from_mod_net->rename(to_name.c_str());
      to_mod_net->rename(required_name.c_str());
    } else if (from_db_net && to_db_net) {
      to_db_net->swapNetNames(from_db_net);
    } else if (from_mod_net && to_db_net) {
      to_db_net->rename(required_name.c_str());
      from_mod_net->rename(to_name.c_str());
    } else if (to_mod_net && from_db_net) {
      to_mod_net->rename(required_name.c_str());
      from_db_net->rename(to_name.c_str());
    }
  }
}

/*
Make sure all the top pins are buffered
*/

Instance* Resizer::bufferInput(const Pin* top_pin,
                               LibertyCell* buffer_cell,
                               bool verbose)
{
  dbNet* top_pin_flat_net = db_network_->flatNet(top_pin);
  odb::dbModNet* top_pin_hier_net = db_network_->hierNet(top_pin);

  // Filter to see if we need to do anything..
  bool has_non_buffer = false;
  bool has_dont_touch = false;
  NetConnectedPinIterator* pin_iter
      = network_->connectedPinIterator(db_network_->dbToSta(top_pin_flat_net));
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    // Leave input port pin connected to top net
    if (pin != top_pin) {
      auto inst = network_->instance(pin);
      odb::dbInst* db_inst;
      odb::dbModInst* mod_inst;
      db_network_->staToDb(inst, db_inst, mod_inst);
      if (dontTouch(inst)) {
        has_dont_touch = true;
        // clang-format off
        logger_->warn(RSZ,
                      85,
                      "Input {} can't be buffered due to dont-touch fanout {}",
                      (top_pin_flat_net ? network_->name(
                           db_network_->dbToSta(top_pin_flat_net))
                                        : "no-input-net"),
                      network_->name(pin));
        // clang-format on
        break;
      }
      auto cell = network_->cell(inst);
      if (db_inst) {
        auto lib = network_->libertyCell(cell);
        if (lib && !lib->isBuffer()) {
          has_non_buffer = true;
        }
      } else {
        has_non_buffer = true;
      }
    }
  }
  delete pin_iter;
  // dont buffer, buffers
  if (has_dont_touch || !has_non_buffer) {
    if (verbose) {
      logger_->info(RSZ,
                    213,
                    "Skipping input port {} buffering.",
                    network_->name(top_pin));
    }
    return nullptr;
  }

  if (verbose) {
    logger_->info(
        RSZ, 214, "Buffering input port {}.", network_->name(top_pin));
  }

  // make the buffer and its output net.
  string buffer_name = makeUniqueInstName("input");
  Instance* parent = db_network_->topInstance();
  Net* buffer_out = makeUniqueNet();
  dbNet* buffer_out_flat_net = db_network_->flatNet(buffer_out);
  Point pin_loc = db_network_->location(top_pin);
  Instance* buffer
      = makeBuffer(buffer_cell, buffer_name.c_str(), parent, pin_loc);
  inserted_buffer_count_++;

  Pin* buffer_ip_pin = nullptr;
  Pin* buffer_op_pin = nullptr;
  getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);

  pin_iter
      = network_->connectedPinIterator(db_network_->dbToSta(top_pin_flat_net));
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin != top_pin) {
      odb::dbBTerm* dest_bterm;
      odb::dbModITerm* dest_moditerm;
      odb::dbITerm* dest_iterm;
      db_network_->staToDb(pin, dest_iterm, dest_bterm, dest_moditerm);
      odb::dbModNet* dest_modnet = db_network_->hierNet(pin);
      sta_->disconnectPin(const_cast<Pin*>(pin));
      if (dest_modnet) {
        if (dest_iterm) {
          dest_iterm->connect(dest_modnet);
        }
        if (dest_moditerm) {
          dest_moditerm->connect(dest_modnet);
        }
      }
      if (dest_iterm) {
        dest_iterm->connect(buffer_out_flat_net);
      } else if (dest_bterm) {
        dest_bterm->connect(buffer_out_flat_net);
      }
    }
  }
  delete pin_iter;

  db_network_->connectPin(buffer_ip_pin,
                          db_network_->dbToSta(top_pin_flat_net));
  db_network_->connectPin(buffer_op_pin,
                          db_network_->dbToSta(buffer_out_flat_net));
  //
  // we are going to push the top mod net into the core
  // so we rename it to avoid conflict with top level
  // name. We name it the same as the flat net used on
  // the buffer output.
  //

  if (top_pin_hier_net) {
    top_pin_hier_net->rename(buffer_out_flat_net->getName().c_str());
    db_network_->connectPin(buffer_op_pin,
                            db_network_->dbToSta(top_pin_hier_net));
  }

  //
  // Remove the top net connection to the mod net, if any
  // and make sure top pin connected to just the flat net.
  //
  if (top_pin_hier_net) {
    db_network_->disconnectPin(const_cast<Pin*>(top_pin));
    db_network_->connectPin(const_cast<Pin*>(top_pin),
                            db_network_->dbToSta(top_pin_flat_net));
  }

  return buffer;
}

void Resizer::bufferOutputs(LibertyCell* buffer_cell, bool verbose)
{
  init();

  // Use buffer_cell. If it is null, find the buffer w/ lowest drive resistance.
  LibertyCell* selected_buffer_cell = selectBufferCell(buffer_cell);
  if (verbose) {
    logger_->info(RSZ,
                  31,
                  "Start output port buffering with {}.",
                  selected_buffer_cell->name());
  }

  inserted_buffer_count_ = 0;
  buffer_moved_into_core_ = false;

  {
    IncrementalParasiticsGuard guard(this);
    std::unique_ptr<InstancePinIterator> port_iter(
        network_->pinIterator(network_->topInstance()));
    while (port_iter->hasNext()) {
      Pin* pin = port_iter->next();
      Vertex* vertex = graph_->pinLoadVertex(pin);
      Net* net = network_->net(network_->term(pin));
      if (network_->direction(pin)->isOutput() && net
          && !dontTouch(net)
          // Hands off special nets.
          && !db_network_->isSpecial(net)
          // DEF does not have tristate output types so we have look at the
          // drivers.
          && !hasTristateOrDontTouchDriver(net) && !vertex->isConstant()
          && hasPins(net)) {
        bufferOutput(pin, selected_buffer_cell, verbose);
      }
    }
  }

  logger_->info(RSZ,
                28,
                "Inserted {} {} output buffers.",
                inserted_buffer_count_,
                selected_buffer_cell->name());

  if (inserted_buffer_count_ > 0) {
    level_drvr_vertices_valid_ = false;
  }
}

bool Resizer::hasTristateOrDontTouchDriver(const Net* net)
{
  PinSet* drivers = network_->drivers(net);
  if (drivers) {
    for (const Pin* pin : *drivers) {
      if (isTristateDriver(pin)) {
        return true;
      }
      odb::dbITerm* iterm;
      odb::dbBTerm* bterm;
      odb::dbModITerm* moditerm;
      db_network_->staToDb(pin, iterm, bterm, moditerm);
      if (iterm && iterm->getInst()->isDoNotTouch()) {
        logger_->warn(RSZ,
                      84,
                      "Output {} can't be buffered due to dont-touch driver {}",
                      network_->name(net),
                      network_->name(pin));
        return true;
      }
    }
  }
  return false;
}

bool Resizer::isTristateDriver(const Pin* pin)
{
  // Note LEF macro PINs do not have a clue about tristates.
  LibertyPort* port = network_->libertyPort(pin);
  return port && port->direction()->isAnyTristate();
}

void Resizer::bufferOutput(const Pin* top_pin,
                           LibertyCell* buffer_cell,
                           bool verbose)
{
  if (verbose) {
    logger_->info(
        RSZ, 215, "Buffering output port {}.", network_->name(top_pin));
  }

  NetworkEdit* network = networkEdit();

  odb::dbITerm* top_pin_op_iterm;
  odb::dbBTerm* top_pin_op_bterm;
  odb::dbModITerm* top_pin_op_moditerm;

  db_network_->staToDb(
      top_pin, top_pin_op_iterm, top_pin_op_bterm, top_pin_op_moditerm);

  odb::dbNet* flat_op_net = top_pin_op_bterm->getNet();
  odb::dbModNet* hier_op_net = top_pin_op_bterm->getModNet();

  sta_->disconnectPin(const_cast<Pin*>(top_pin));

  LibertyPort *input, *output;
  assert(buffer_cell);
  buffer_cell->bufferPorts(input, output);

  string buffer_name = makeUniqueInstName("output");
  Net* buffer_out = makeUniqueNet();
  Instance* parent = network->topInstance();

  Point pin_loc = db_network_->location(top_pin);
  // buffer made in top level.
  Instance* buffer
      = makeBuffer(buffer_cell, buffer_name.c_str(), parent, pin_loc);
  inserted_buffer_count_++;

  // connect original input (hierarchical or flat) to buffer input
  // handle hierarchy
  Pin* buffer_ip_pin = nullptr;
  Pin* buffer_op_pin = nullptr;
  getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);

  // get the iterms. Note this are never null (makeBuffer properly instantiates
  // them and we know to always expect an iterm.). However, the api (flatPin)
  // truly checks to see if the pins could be moditerms & if they are returns
  // null, so coverity rationally reasons that the iterm could be null,
  // so we add some extra checking here. This is a consequence of
  //"hiding" the full api.
  //
  odb::dbITerm* buffer_op_pin_iterm = db_network_->flatPin(buffer_op_pin);
  odb::dbITerm* buffer_ip_pin_iterm = db_network_->flatPin(buffer_ip_pin);

  if (buffer_ip_pin_iterm && buffer_op_pin_iterm) {
    if (flat_op_net) {
      buffer_ip_pin_iterm->connect(flat_op_net);
    }
    if (hier_op_net) {
      buffer_ip_pin_iterm->connect(hier_op_net);
    }
    buffer_op_pin_iterm->connect(db_network_->staToDb(buffer_out));
    top_pin_op_bterm->connect(db_network_->staToDb(buffer_out));
    SwapNetNames(buffer_op_pin_iterm, buffer_ip_pin_iterm);
    // rename the mod net to match the flat net.
    if (buffer_ip_pin_iterm->getNet() && buffer_ip_pin_iterm->getModNet()) {
      buffer_ip_pin_iterm->getModNet()->rename(
          buffer_ip_pin_iterm->getNet()->getName().c_str());
    }
  }
}

////////////////////////////////////////////////////////////////

float Resizer::driveResistance(const Pin* drvr_pin)
{
  if (network_->isTopLevelPort(drvr_pin)) {
    InputDrive* drive = sdc_->findInputDrive(network_->port(drvr_pin));
    if (drive) {
      float max_res = 0;
      for (auto min_max : MinMax::range()) {
        for (auto rf : RiseFall::range()) {
          const LibertyCell* cell;
          const LibertyPort* from_port;
          float* from_slews;
          const LibertyPort* to_port;
          drive->driveCell(rf, min_max, cell, from_port, from_slews, to_port);
          if (to_port) {
            max_res = max(max_res, to_port->driveResistance());
          } else {
            float res;
            bool exists;
            drive->driveResistance(rf, min_max, res, exists);
            if (exists) {
              max_res = max(max_res, res);
            }
          }
        }
      }
      return max_res;
    }
  } else {
    LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    if (drvr_port) {
      return drvr_port->driveResistance();
    }
  }
  return 0.0;
}

float Resizer::bufferDriveResistance(const LibertyCell* buffer) const
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  return output->driveResistance();
}

// This should be exported by STA
float Resizer::cellDriveResistance(const LibertyCell* cell) const
{
  sta::LibertyCellPortBitIterator port_iter(cell);
  while (port_iter.hasNext()) {
    auto port = port_iter.next();
    if (port->direction()->isOutput()) {
      return port->driveResistance();
    }
  }
  return 0.0;
}

LibertyCell* Resizer::halfDrivingPowerCell(Instance* inst)
{
  return halfDrivingPowerCell(network_->libertyCell(inst));
}
LibertyCell* Resizer::halfDrivingPowerCell(LibertyCell* cell)
{
  return closestDriver(cell, getSwappableCells(cell), 0.5);
}

bool Resizer::isSingleOutputCombinational(Instance* inst) const
{
  dbInst* db_inst = db_network_->staToDb(inst);
  if (inst == network_->topInstance() || db_inst->isBlock()) {
    return false;
  }
  return isSingleOutputCombinational(network_->libertyCell(inst));
}

bool Resizer::isSingleOutputCombinational(LibertyCell* cell) const
{
  if (!cell) {
    return false;
  }
  auto output_pins = libraryOutputPins(cell);
  return (output_pins.size() == 1 && isCombinational(cell));
}

bool Resizer::isCombinational(LibertyCell* cell) const
{
  if (!cell) {
    return false;
  }
  return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro()
          && !cell->hasSequentials());
}

std::vector<sta::LibertyPort*> Resizer::libraryOutputPins(
    LibertyCell* cell) const
{
  auto pins = libraryPins(cell);
  for (auto it = pins.begin(); it != pins.end(); it++) {
    if (!((*it)->direction()->isAnyOutput())) {
      it = pins.erase(it);
      it--;
    }
  }
  return pins;
}

std::vector<sta::LibertyPort*> Resizer::libraryPins(Instance* inst) const
{
  return libraryPins(network_->libertyCell(inst));
}

std::vector<sta::LibertyPort*> Resizer::libraryPins(LibertyCell* cell) const
{
  std::vector<sta::LibertyPort*> pins;
  sta::LibertyCellPortIterator itr(cell);
  while (itr.hasNext()) {
    auto port = itr.next();
    pins.emplace_back(port);
  }
  return pins;
}

LibertyCell* Resizer::closestDriver(LibertyCell* cell,
                                    const LibertyCellSeq& candidates,
                                    float scale)
{
  LibertyCell* closest = nullptr;
  if (candidates.empty() || !isSingleOutputCombinational(cell)) {
    return nullptr;
  }
  const auto output_pin = libraryOutputPins(cell)[0];
  const auto current_limit = scale * maxLoad(output_pin->cell());
  auto diff = sta::INF;
  for (auto& cand : candidates) {
    if (dontUse(cand)) {
      continue;
    }
    auto limit = maxLoad(libraryOutputPins(cand)[0]->cell());
    if (limit == current_limit) {
      return cand;
    }
    auto new_diff = std::fabs(limit - current_limit);
    if (new_diff < diff) {
      diff = new_diff;
      closest = cand;
    }
  }
  return closest;
}

float Resizer::maxLoad(Cell* cell)
{
  LibertyCell* lib_cell = network_->libertyCell(cell);
  auto min_max = sta::MinMax::max();
  sta::LibertyCellPortIterator itr(lib_cell);
  while (itr.hasNext()) {
    LibertyPort* port = itr.next();
    if (port->direction()->isOutput()) {
      float limit, limit1;
      bool exists, exists1;
      const sta::Corner* corner = sta_->cmdCorner();
      Sdc* sdc = sta_->sdc();
      // Default to top ("design") limit.
      Cell* top_cell = network_->cell(network_->topInstance());
      sdc->capacitanceLimit(top_cell, min_max, limit, exists);
      sdc->capacitanceLimit(cell, min_max, limit1, exists1);

      if (exists1 && (!exists || min_max->compare(limit, limit1))) {
        limit = limit1;
        exists = true;
      }
      LibertyPort* corner_port = port->cornerPort(corner, min_max);
      corner_port->capacitanceLimit(min_max, limit1, exists1);
      if (!exists1 && port->direction()->isAnyOutput()) {
        corner_port->libertyLibrary()->defaultMaxCapacitance(limit1, exists1);
      }
      if (exists1 && (!exists || min_max->compare(limit, limit1))) {
        limit = limit1;
        exists = true;
      }
      if (exists) {
        return limit;
      }
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////

bool Resizer::hasFanout(Vertex* drvr)
{
  VertexOutEdgeIterator edge_iter(drvr, graph_);
  return edge_iter.hasNext();
}

static float targetLoadDist(float load_cap, float target_load)
{
  return abs(load_cap - target_load);
}

////////////////////////////////////////////////////////////////

void Resizer::resizeDrvrToTargetSlew(const Pin* drvr_pin)
{
  resizePreamble();
  resizeToTargetSlew(drvr_pin);
}

void Resizer::resizePreamble()
{
  init();
  ensureLevelDrvrVertices();
  sta_->ensureClkNetwork();
  makeEquivCells();
  checkLibertyForAllCorners();
  findBuffers();
  findTargetLoads();
  findFastBuffers();
}

// Convert static cell leakage to std::optional.
// For state-dependent leakage, compute the average
// across all the power states.  Cache the leakage for
// runtime.
std::optional<float> Resizer::cellLeakage(LibertyCell* cell)
{
  auto it = cell_leakage_cache_.find(cell);
  if (it != cell_leakage_cache_.end()) {
    return it->second;
  }

  float leakage = 0.0;
  bool exists;
  cell->leakagePower(leakage, exists);
  if (exists) {
    cell_leakage_cache_[cell] = leakage;
    return leakage;
  }

  // Compute average leakage across power conds for state-dependent leakage
  LeakagePowerSeq* leakages = cell->leakagePowers();
  if (!leakages || leakages->empty()) {
    cell_leakage_cache_[cell] = std::nullopt;
    return std::nullopt;
  }

  float total_leakage = 0.0;
  for (LeakagePower* leak : *leakages) {
    total_leakage += leak->power();
  }
  leakage = total_leakage / leakages->size();
  cell_leakage_cache_[cell] = leakage;
  return leakage;
}

// For debugging
void Resizer::reportEquivalentCells(LibertyCell* base_cell,
                                    bool match_cell_footprint,
                                    bool report_all_cells)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  resizePreamble();
  LibertyCellSeq equiv_cells;

  if (report_all_cells) {
    swappable_cells_cache_.clear();
    bool restrict = false;
    std::optional<double> no_limit = std::nullopt;
    utl::SetAndRestore relax_footprint(match_cell_footprint_, restrict);
    utl::SetAndRestore relax_sizing_area_limit(sizing_area_limit_, no_limit);
    utl::SetAndRestore relax_sizing_leakage_limit(sizing_leakage_limit_,
                                                  no_limit);
    utl::SetAndRestore relax_keep_site(sizing_keep_site_, restrict);
    utl::SetAndRestore relax_keep_vt(sizing_keep_vt_, restrict);
    equiv_cells = getSwappableCells(base_cell);
    swappable_cells_cache_.clear();  // SetAndRestore invalidates cache.
  } else {
    equiv_cells = getSwappableCells(base_cell);
  }

  // Identify cells that are excluded due to restrictions or cell_footprints
  std::unordered_set<LibertyCell*> excluded_cells;
  if (report_all_cells) {
    // All SetAndRestore variables are out of scope, so original restrictions
    // are in play
    LibertyCellSeq real_equiv_cells = getSwappableCells(base_cell);
    std::unordered_set<LibertyCell*> real_equiv_cells_set(
        real_equiv_cells.begin(), real_equiv_cells.end());
    for (LibertyCell* cell : equiv_cells) {
      if (real_equiv_cells_set.find(cell) == real_equiv_cells_set.end()) {
        excluded_cells.insert(cell);
      }
    }
  }

  // Sort equiv cells by ascending area and leakage
  // STA sorts them by drive resistance
  std::stable_sort(equiv_cells.begin(),
                   equiv_cells.end(),
                   [this](LibertyCell* a, LibertyCell* b) {
                     dbMaster* master_a = this->getDbNetwork()->staToDb(a);
                     dbMaster* master_b = this->getDbNetwork()->staToDb(b);
                     if (master_a && master_b) {
                       if (master_a->getArea() != master_b->getArea()) {
                         return master_a->getArea() < master_b->getArea();
                       }
                     }
                     std::optional<float> leakage_a = this->cellLeakage(a);
                     std::optional<float> leakage_b = this->cellLeakage(b);
                     // Compare leakages only if they are defined
                     if (leakage_a && leakage_b) {
                       return *leakage_a < *leakage_b;
                     }
                     // Cell 'a' has a priority as it has lower
                     // drive resistance than 'b' after STA sort
                     return leakage_a.has_value() && !leakage_b.has_value();
                   });

  logger_->report(
      "The following {} cells are equivalent to {}{}",
      equiv_cells.size(),
      base_cell->name(),
      (match_cell_footprint ? " with matching cell_footprint:" : ":"));
  odb::dbMaster* master = db_network_->staToDb(base_cell);
  double base_area = block_->dbuAreaToMicrons(master->getArea());
  std::optional<float> base_leakage = cellLeakage(base_cell);
  if (base_leakage) {
    logger_->report(
        "======================================================================"
        "=======");
    logger_->report(
        "          Cell                              Area   Area Leakage  "
        "Leakage  VT");
    logger_->report(
        "                                           (um^2)  Ratio  (W)     "
        "Ratio  Type");
    logger_->report(
        "======================================================================"
        "=======");
    for (LibertyCell* equiv_cell : equiv_cells) {
      std::string cell_name = equiv_cell->name();
      if (excluded_cells.find(equiv_cell) != excluded_cells.end()) {
        cell_name.insert(cell_name.begin(), '*');
      }
      odb::dbMaster* equiv_master = db_network_->staToDb(equiv_cell);
      double equiv_area = block_->dbuAreaToMicrons(equiv_master->getArea());
      std::optional<float> equiv_cell_leakage = cellLeakage(equiv_cell);
      if (equiv_cell_leakage) {
        logger_->report("{:<41} {:>7.3f} {:>5.2f} {:>8.2e} {:>5.2f}   {}",
                        cell_name,
                        equiv_area,
                        equiv_area / base_area,
                        *equiv_cell_leakage,
                        *equiv_cell_leakage / *base_leakage,
                        cellVTType(equiv_master).second);
      } else {
        logger_->report("{:<41} {:>7.3f} {:>5.2f}   {}",
                        cell_name,
                        equiv_area,
                        equiv_area / base_area,
                        cellVTType(equiv_master).second);
      }
    }
    logger_->report(
        "----------------------------------------------------------------------"
        "-------");
  } else {
    logger_->report(
        "==============================================================");
    logger_->report(
        "          Cell                              Area   Area    VT");
    logger_->report(
        "                                           (um^2)  Ratio  Type");
    logger_->report(
        "==============================================================");
    for (LibertyCell* equiv_cell : equiv_cells) {
      std::string cell_name = equiv_cell->name();
      if (excluded_cells.find(equiv_cell) != excluded_cells.end()) {
        cell_name.insert(cell_name.begin(), '*');
      }
      odb::dbMaster* equiv_master = db_network_->staToDb(equiv_cell);
      double equiv_area = block_->dbuAreaToMicrons(equiv_master->getArea());
      logger_->report("{:<41} {:>7.3f} {:>5.2f}   {}",
                      cell_name,
                      equiv_area,
                      equiv_area / base_area,
                      cellVTType(equiv_master).second);
    }
    logger_->report(
        "--------------------------------------------------------------");
  }
}

void Resizer::reportBuffers(bool filtered)
{
  resizePreamble();

  LibertyCellSeq buffer_list;
  LibraryAnalysisData lib_data;

  getBufferList(buffer_list, lib_data);
  if (buffer_list.empty()) {
    logger_->error(RSZ, 23, "No buffers are found from the loaded libraries.");
    return;
  }

  logger_->report("{:*>80}", "");
  logger_->report("Buffer Report:");
  logger_->report(
      "There are {} buffers that are not marked as dont-use, multi-voltage\n"
      "special cells{}",
      buffer_list.size(),
      (exclude_clock_buffers_ ? " or clock buffers" : ""));

  logger_->report("\nThere are {} VT types:",
                  lib_data.vt_leakage_by_category.size());
  logger_->report("VT type[index]: # buffers, ave leakage");
  for (const auto& [vt_category, vt_stats] : lib_data.sorted_vt_categories) {
    logger_->report("  {:<6} [{}]: {}, {:>7.1e}",
                    vt_category.vt_name,
                    vt_category.vt_index,
                    vt_stats.cell_count,
                    vt_stats.get_average_leakage());
  }

  logger_->report("\nThere are {} cell footprint types:",
                  lib_data.cells_by_footprint.size());
  for (const auto& [footprint_type, count] : lib_data.cells_by_footprint) {
    logger_->report("  {:<6}: {} [{:.2f}%]",
                    footprint_type,
                    count,
                    ((float) count) / buffer_list.size() * 100);
  }

  logger_->report("\nThere are {} cell site types:",
                  lib_data.cells_by_site.size());
  for (const auto& [site_type, count] : lib_data.cells_by_site) {
    logger_->report("  {:<6} [H={}]: {} [{:.2f}%]",
                    site_type->getName(),
                    site_type->getHeight(),
                    count,
                    ((float) count) / buffer_list.size() * 100);
  }

  logger_->report("{:->80}", "");
  logger_->report(
      "Cell                                        Drive Drive    Leak "
      "  Site Cell   VT");
  logger_->report(
      "                                            Res   Res*Cin       "
      "   Ht Footpt Type");
  logger_->report("{:->80}", "");

  for (LibertyCell* buffer : buffer_list) {
    float drive_res = bufferDriveResistance(buffer);
    LibertyPort *input, *output;
    buffer->bufferPorts(input, output);
    float c_in = input->capacitance();
    std::optional<float> cell_leak = cellLeakage(buffer);
    odb::dbMaster* master = db_network_->staToDb(buffer);

    logger_->report("{:<41} {:>7.1f} {:>7.1e} {:>7.1e} {:>3} {:<7} {:<}",
                    buffer->name(),
                    drive_res,
                    drive_res * c_in,
                    cell_leak.value_or(0.0f),
                    master->getSite()->getHeight(),
                    (buffer->footprint() ? buffer->footprint() : "N/A"),
                    cellVTType(master).second);
  }

  if (filtered) {
    findBuffers();
    logger_->report("\nFiltered Buffer Report:");
    logger_->report(
        "There are {} buffers after filtering based on threshold voltage,"
        "\ncell footprint, drive strength and cell site",
        buffer_cells_.size());
    logger_->report("{:->80}", "");
    logger_->report(
        "Cell                                        Drive Drive    Leak "
        "  Site Cell   VT");
    logger_->report(
        "                                            Res   Res*Cin       "
        "   Ht Footpt Type");
    logger_->report("{:->80}", "");

    for (LibertyCell* buffer : buffer_cells_) {
      float drive_res = bufferDriveResistance(buffer);
      LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      float c_in = input->capacitance();
      std::optional<float> cell_leak = cellLeakage(buffer);
      odb::dbMaster* master = db_network_->staToDb(buffer);

      logger_->report("{:<41} {:>7.1f} {:>7.1e} {:>7.1e} {:>3} {:<7} {:<}",
                      buffer->name(),
                      drive_res,
                      drive_res * c_in,
                      cell_leak.value_or(0.0f),
                      master->getSite()->getHeight(),
                      (buffer->footprint() ? buffer->footprint() : "N/A"),
                      cellVTType(master).second);
    }
  }

  logger_->report("{:*>80}", "");
}

void Resizer::getBufferList(LibertyCellSeq& buffer_list,
                            LibraryAnalysisData& lib_data)
{
  LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary* lib = lib_iter->next();
    for (LibertyCell* buffer : *lib->buffers()) {
      if (exclude_clock_buffers_) {
        BufferUse buffer_use = sta_->getBufferUse(buffer);
        if (buffer_use == CLOCK) {
          continue;
        }
      }
      if (!dontUse(buffer) && !buffer->alwaysOn() && !buffer->isIsolationCell()
          && !buffer->isLevelShifter() && isLinkCell(buffer)) {
        buffer_list.emplace_back(buffer);

        // Track cell footprint distribution
        if (buffer->footprint()) {
          lib_data.cells_by_footprint[buffer->footprint()]++;
        }

        // Track site distribution
        odb::dbMaster* master = db_network_->staToDb(buffer);
        lib_data.cells_by_site[master->getSite()]++;

        // Track VT category leakage data
        auto vt_type = cellVTType(master);
        VTCategory vt_category{vt_type.first, vt_type.second};

        // Get or create VT leakage stats for this category
        VTLeakageStats& vt_stats = lib_data.vt_leakage_by_category[vt_category];
        vt_stats.add_cell_leakage(cellLeakage(buffer));
      }
    }
  }
  delete lib_iter;

  if (!buffer_list.empty()) {
    // Sort buffer list by VT type, then by drive resistance
    sort(buffer_list,
         [this](const LibertyCell* buffer1, const LibertyCell* buffer2) {
           odb::dbMaster* master1 = db_network_->staToDb(buffer1);
           odb::dbMaster* master2 = db_network_->staToDb(buffer2);
           auto vt_type1 = cellVTType(master1);
           auto vt_type2 = cellVTType(master2);
           if (vt_type1 != vt_type2) {
             return vt_type1 < vt_type2;
           }
           return bufferDriveResistance(buffer1)
                  < bufferDriveResistance(buffer2);
         });

    // Sort VT categories by average leakage
    lib_data.sort_vt_categories();
  }
}

// Filter equivalent cells based on the following liberty attributes:
// - Footprint (Optional - Honored if enforced by user): Cells with the
//   same footprint have the same layout boundary.
// - User Function Class (Optional - Honored if found): Cells with the
//   same user_function_class are electrically compatible.
// - DontUse: Cells that are marked dont-use are not considered for
//   replacement.
// - Link Cell: Only link cells are considered for replacement.
// This function is cached for performance and reset if more cells are read.
LibertyCellSeq Resizer::getSwappableCells(LibertyCell* source_cell)
{
  if (swappable_cells_cache_.find(source_cell)
      != swappable_cells_cache_.end()) {
    return swappable_cells_cache_[source_cell];
  }

  dbMaster* master = db_network_->staToDb(source_cell);
  if (master == nullptr || !master->isCore()) {
    swappable_cells_cache_[source_cell] = {};
    return {};
  }

  LibertyCellSeq swappable_cells;
  LibertyCellSeq* equiv_cells = sta_->equivCells(source_cell);

  if (equiv_cells) {
    int64_t source_cell_area = master->getArea();
    std::optional<float> source_cell_leakage;
    if (sizing_leakage_limit_) {
      source_cell_leakage = cellLeakage(source_cell);
    }
    for (LibertyCell* equiv_cell : *equiv_cells) {
      if (dontUse(equiv_cell) || !isLinkCell(equiv_cell)) {
        continue;
      }

      dbMaster* equiv_cell_master = db_network_->staToDb(equiv_cell);
      if (!equiv_cell_master) {
        continue;
      }

      if (sizing_area_limit_.has_value() && (source_cell_area != 0)
          && (equiv_cell_master->getArea()
                  / static_cast<double>(source_cell_area)
              > sizing_area_limit_.value())) {
        continue;
      }

      if (sizing_leakage_limit_ && source_cell_leakage) {
        std::optional<float> equiv_cell_leakage = cellLeakage(equiv_cell);
        if (equiv_cell_leakage
            && (*equiv_cell_leakage / *source_cell_leakage
                > sizing_leakage_limit_.value())) {
          continue;
        }
      }

      if (sizing_keep_site_) {
        if (master->getSite() != equiv_cell_master->getSite()) {
          continue;
        }
      }

      if (sizing_keep_vt_) {
        if (cellVTType(master).first != cellVTType(equiv_cell_master).first) {
          continue;
        }
      }

      if (match_cell_footprint_) {
        const bool footprints_match = sta::stringEqIf(source_cell->footprint(),
                                                      equiv_cell->footprint());
        if (!footprints_match) {
          continue;
        }
      }

      if (source_cell->userFunctionClass()) {
        const bool user_function_classes_match = sta::stringEqIf(
            source_cell->userFunctionClass(), equiv_cell->userFunctionClass());
        if (!user_function_classes_match) {
          continue;
        }
      }

      swappable_cells.push_back(equiv_cell);
    }
  } else {
    swappable_cells.push_back(source_cell);
  }

  swappable_cells_cache_[source_cell] = swappable_cells;
  return swappable_cells;
}

void Resizer::checkLibertyForAllCorners()
{
  for (Corner* corner : *sta_->corners()) {
    int lib_ap_index = corner->libertyIndex(max_);
    LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      LibertyLibrary* lib = lib_iter->next();
      LibertyCellIterator cell_iter(lib);
      while (cell_iter.hasNext()) {
        LibertyCell* cell = cell_iter.next();
        if (isLinkCell(cell) && !dontUse(cell)) {
          LibertyCell* corner_cell = cell->cornerCell(lib_ap_index);
          if (!corner_cell) {
            logger_->warn(RSZ,
                          96,
                          "Cell {} is missing in {} and will be set dont-use",
                          cell->name(),
                          corner->name());
            setDontUse(cell, true);
            continue;
          }
        }
      }
    }
    delete lib_iter;
  }
}

void Resizer::setParasiticsSrc(ParasiticsSrc src)
{
  if (incremental_parasitics_enabled_) {
    logger_->error(RSZ,
                   108,
                   "cannot change parasitics source while incremental "
                   "parasitics enabled");
  }

  parasitics_src_ = src;
}

void Resizer::makeEquivCells()
{
  LibertyLibrarySeq libs;
  LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary* lib = lib_iter->next();
    // massive kludge until makeEquivCells is fixed to only incldue link cells
    LibertyCellIterator cell_iter(lib);
    if (cell_iter.hasNext()) {
      LibertyCell* cell = cell_iter.next();
      if (isLinkCell(cell)) {
        libs.emplace_back(lib);
      }
    }
  }
  delete lib_iter;
  sta_->makeEquivCells(&libs, nullptr);
}

// When there are multiple VT layers, create a composite name
// by removing conflicting characters.
std::string mergeVTLayerNames(const std::string& new_name,
                              const std::string& curr_name)
{
  std::string merged;
  size_t len = std::max(new_name.size(), curr_name.size());

  for (size_t i = 0; i < len; ++i) {
    char c1 = (i < new_name.size()) ? new_name[i] : '\0';
    char c2 = (i < curr_name.size()) ? curr_name[i] : '\0';

    if (c1 == c2) {
      merged.push_back(c1);  // Same character, keep it
    } else if (c1 == '\0') {
      merged.push_back(c2);  // Only c2 exists, add it
    } else if (c2 == '\0') {
      merged.push_back(c1);  // Only c1 exists, add it
    }
    // If c1 and c2 are different, we do not add anything
  }

  // If there is no overlap between names, append the names
  if (merged.empty()) {
    merged = new_name + curr_name;
  }
  return merged;
}

// Make new composite VT type name more compact by stripping out VT
// and trailing underscore
// LVT => L (no VT)
// VTL_ => L (no VT, no trailing underscore)
void compressVTLayerName(std::string& name)
{
  if (name.empty()) {
    return;
  }

  // Strip out VT from name
  size_t pos;
  while ((pos = name.find("VT")) != std::string::npos) {
    name.erase(pos, 2);
  }

  // Strip out trailing underscore
  if (!name.empty() && name.back() == '_') {
    name.pop_back();
  }
}

// VT layers are typically in LEF obstruction section.
// To categorize cell VT type, hash all obstruction VT layers.
// Cells beloning to the same VT category should have identical layer
// composition, resulting in the same hash value.  VT type is 0 if there are
// no OBS VT layers.
std::pair<int, std::string> Resizer::cellVTType(dbMaster* master)
{
  // Check if VT type is already computed
  auto it = vt_map_.find(master);
  if (it != vt_map_.end()) {
    return it->second;
  }

  dbSet<dbBox> obs = master->getObstructions();
  if (obs.empty()) {
    auto [new_it, _] = vt_map_.emplace(master, std::make_pair(0, "-"));
    return new_it->second;
  }

  std::unordered_set<std::string> unique_layers;  // count each layer only once
  size_t hash1 = 0;
  std::string new_layer_name;
  for (dbBox* bbox : obs) {
    dbTechLayer* layer = bbox->getTechLayer();
    if (layer == nullptr || layer->getType() != odb::dbTechLayerType::IMPLANT) {
      continue;
    }

    std::string curr_layer_name = layer->getName();
    if (unique_layers.insert(curr_layer_name).second) {
      debugPrint(logger_,
                 RSZ,
                 "equiv",
                 1,
                 "{} has OBS implant layer {}",
                 master->getName(),
                 curr_layer_name);
      size_t hash2 = boost::hash<std::string>()(curr_layer_name);
      boost::hash_combine(hash1, hash2);
      new_layer_name = mergeVTLayerNames(new_layer_name, curr_layer_name);
    }
  }

  if (hash1 == 0) {
    auto [new_it, _] = vt_map_.emplace(master, std::make_pair(0, "-"));
    return new_it->second;
  }

  if (vt_hash_map_.find(hash1) == vt_hash_map_.end()) {
    int vt_id = vt_hash_map_.size() + 1;
    vt_hash_map_[hash1] = vt_id;
  }

  compressVTLayerName(new_layer_name);
  auto [new_it, _] = vt_map_.emplace(
      master, std::make_pair(vt_hash_map_[hash1], new_layer_name));
  debugPrint(logger_,
             RSZ,
             "equiv",
             1,
             "{} has VT type {} {}",
             master->getName(),
             vt_map_[master].first,
             vt_map_[master].second);
  return new_it->second;
}

int Resizer::resizeToTargetSlew(const Pin* drvr_pin)
{
  Instance* inst = network_->instance(drvr_pin);
  LibertyCell* cell = network_->libertyCell(inst);
  if (!network_->isTopLevelPort(drvr_pin) && !dontTouch(inst) && cell
      && isLogicStdCell(inst)) {
    bool revisiting_inst = false;
    if (hasMultipleOutputs(inst)) {
      revisiting_inst = resized_multi_output_insts_.hasKey(inst);
      debugPrint(logger_,
                 RSZ,
                 "resize",
                 2,
                 "multiple outputs{}",
                 revisiting_inst ? " - revisit" : "");
      resized_multi_output_insts_.insert(inst);
    }
    ensureWireParasitic(drvr_pin);
    // Includes net parasitic capacitance.
    float load_cap = graph_delay_calc_->loadCap(drvr_pin, tgt_slew_dcalc_ap_);
    if (load_cap > 0.0) {
      LibertyCell* target_cell
          = findTargetCell(cell, load_cap, revisiting_inst);
      if (target_cell != cell) {
        debugPrint(logger_,
                   RSZ,
                   "resize",
                   2,
                   "{} {} -> {}",
                   sdc_network_->pathName(drvr_pin),
                   cell->name(),
                   target_cell->name());
        if (replaceCell(inst, target_cell, true) && !revisiting_inst) {
          return 1;
        }
      }
    }
  }
  return 0;
}

bool Resizer::getCin(const LibertyCell* cell, float& cin)
{
  sta::LibertyCellPortIterator port_iter(cell);
  int nports = 0;
  cin = 0;
  while (port_iter.hasNext()) {
    const LibertyPort* port = port_iter.next();
    if (port->direction() == sta::PortDirection::input()) {
      cin += port->capacitance();
      nports++;
    }
  }
  if (nports) {
    cin /= nports;
    return true;
  }
  return false;
}

int Resizer::resizeToCapRatio(const Pin* drvr_pin, bool upsize_only)
{
  Instance* inst = network_->instance(drvr_pin);
  LibertyCell* cell = inst ? network_->libertyCell(inst) : nullptr;
  if (!network_->isTopLevelPort(drvr_pin) && inst && !dontTouch(inst) && cell
      && isLogicStdCell(inst)) {
    float cin, load_cap;
    ensureWireParasitic(drvr_pin);

    // Includes net parasitic capacitance.
    load_cap = graph_delay_calc_->loadCap(drvr_pin, tgt_slew_dcalc_ap_);
    if (load_cap > 0.0 && getCin(cell, cin)) {
      float cap_ratio
          = cell->isBuffer() ? buffer_sizing_cap_ratio_ : sizing_cap_ratio_;

      LibertyCellSeq equiv_cells = getSwappableCells(cell);
      LibertyCell *best = nullptr, *highest_cin_cell = nullptr;
      float highest_cin = 0;
      for (LibertyCell* size : equiv_cells) {
        float size_cin;
        if ((!cell->isBuffer() || buffer_fast_sizes_.count(size))
            && getCin(size, size_cin)) {
          if (load_cap < size_cin * cap_ratio) {
            if (upsize_only && size == cell) {
              // The current size of the cell fits the criteria, apply no
              // sizing
              return 0;
            }

            if (!best || size->area() < best->area()) {
              best = size;
            }
          }

          if (size_cin > highest_cin) {
            highest_cin = size_cin;
            highest_cin_cell = size;
          }
        }
      }

      if (best) {
        if (best != cell) {
          replaceCell(inst, best, true);
          return 1;
        }
      } else if (highest_cin_cell) {
        replaceCell(inst, highest_cin_cell, true);
        return 1;
      }
    }
  }
  return 0;
}

bool Resizer::isLogicStdCell(const Instance* inst)
{
  return !db_network_->isTopInstance(inst)
         && db_network_->staToDb(inst)->getMaster()->getType()
                == odb::dbMasterType::CORE;
}

LibertyCell* Resizer::findTargetCell(LibertyCell* cell,
                                     float load_cap,
                                     bool revisiting_inst)
{
  LibertyCell* best_cell = cell;
  LibertyCellSeq swappable_cells = getSwappableCells(cell);
  if (!swappable_cells.empty()) {
    bool is_buf_inv = cell->isBuffer() || cell->isInverter();
    float target_load = (*target_load_map_)[cell];
    float best_load = target_load;
    float best_dist = targetLoadDist(load_cap, target_load);
    float best_delay
        = is_buf_inv ? bufferDelay(cell, load_cap, tgt_slew_dcalc_ap_) : 0.0;
    debugPrint(logger_,
               RSZ,
               "resize",
               3,
               "{} load cap {} dist={:.2e} delay={}",
               cell->name(),
               units_->capacitanceUnit()->asString(load_cap),
               best_dist,
               delayAsString(best_delay, sta_, 3));
    for (LibertyCell* target_cell : swappable_cells) {
      float target_load = (*target_load_map_)[target_cell];
      float delay = 0.0;
      if (is_buf_inv) {
        delay = bufferDelay(target_cell, load_cap, tgt_slew_dcalc_ap_);
      }
      float dist = targetLoadDist(load_cap, target_load);
      debugPrint(logger_,
                 RSZ,
                 "resize",
                 3,
                 " {} dist={:.2e} delay={}",
                 target_cell->name(),
                 dist,
                 delayAsString(delay, sta_, 3));
      if (is_buf_inv
              // Library may have "delay" buffers/inverters that are
              // functionally buffers/inverters but have additional
              // intrinsic delay. Accept worse target load matching if
              // delay is reduced to avoid using them.
              ? ((delay < best_delay && dist < best_dist * 1.1)
                 || (dist < best_dist && delay < best_delay * 1.1))
              : dist < best_dist
                    // If the instance has multiple outputs (generally a
                    // register Q/QN) only allow upsizing after the first pin
                    // is visited.
                    && (!revisiting_inst || target_load > best_load)) {
        best_cell = target_cell;
        best_dist = dist;
        best_load = target_load;
        best_delay = delay;
      }
    }
  }
  return best_cell;
}

void Resizer::eraseParasitics(const Net* net)
{
  parasitics_invalid_.erase(net);
}

// Replace LEF with LEF so ports stay aligned in instance.
bool Resizer::replaceCell(Instance* inst,
                          const LibertyCell* replacement,
                          const bool journal)
{
  const char* replacement_name = replacement->name();
  dbMaster* replacement_master = db_->findMaster(replacement_name);

  if (replacement_master) {
    dbInst* dinst = db_network_->staToDb(inst);
    dbMaster* master = dinst->getMaster();
    designAreaIncr(-area(master));
    Cell* replacement_cell1 = db_network_->dbToSta(replacement_master);
    sta_->replaceCell(inst, replacement_cell1);
    designAreaIncr(area(replacement_master));

    // Legalize the position of the instance in case it leaves the die
    if (parasitics_src_ == ParasiticsSrc::global_routing
        || parasitics_src_ == ParasiticsSrc::detailed_routing) {
      opendp_->legalCellPos(db_network_->staToDb(inst));
    }
    return true;
  }
  return false;
}

bool Resizer::hasMultipleOutputs(const Instance* inst)
{
  int output_count = 0;
  std::unique_ptr<InstancePinIterator> pin_iter(network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->direction(pin)->isAnyOutput() && network_->net(pin)) {
      output_count++;
      if (output_count > 1) {
        return true;
      }
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////

// API for timing driven placement.

void Resizer::resizeSlackPreamble()
{
  resizePreamble();
  // Save max_wire_length for multiple repairDesign calls.
  max_wire_length_ = findMaxWireLength1();
}

// Run repair_design to repair long wires and max slew, capacitance and fanout
// violations. Find the slacks, and then undo all changes to the netlist.
void Resizer::findResizeSlacks(bool run_journal_restore)
{
  IncrementalParasiticsGuard guard(this);
  if (run_journal_restore) {
    journalBegin();
  }
  ensureLevelDrvrVertices();
  estimateWireParasitics();
  int repaired_net_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repair_design_->repairDesign(max_wire_length_,
                               0.0,
                               0.0,
                               0.0,
                               false,
                               repaired_net_count,
                               slew_violations,
                               cap_violations,
                               fanout_violations,
                               length_violations);
  repair_design_->reportViolationCounters(false,
                                          slew_violations,
                                          cap_violations,
                                          fanout_violations,
                                          length_violations,
                                          repaired_net_count);
  fullyRebuffer(nullptr);
  ensureLevelDrvrVertices();

  findResizeSlacks1();
  if (run_journal_restore) {
    journalRestore();
    level_drvr_vertices_valid_ = false;
  }
}

void Resizer::findResizeSlacks1()
{
  // Use driver pin slacks rather than Sta::netSlack to save visiting
  // the net pins and min'ing the slack.
  net_slack_map_.clear();
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex* drvr = level_drvr_vertices_[i];
    Pin* drvr_pin = drvr->pin();
    Net* net = network_->isTopLevelPort(drvr_pin)
                   ? network_->net(network_->term(drvr_pin))
                   : network_->net(drvr_pin);
    if (net
        && !drvr->isConstant()
        // Hands off special nets.
        && !db_network_->isSpecial(net) && !sta_->isClock(drvr_pin)) {
      net_slack_map_[net] = sta_->vertexSlack(drvr, max_);
    }
  }
}

NetSeq Resizer::resizeWorstSlackNets()
{
  // Find the nets with the worst slack.
  NetSeq nets;
  for (auto pair : net_slack_map_) {
    nets.push_back(pair.first);
  }
  sort(nets, [this](const Net* net1, const Net* net2) {
    return resizeNetSlack(net1) < resizeNetSlack(net2);
  });

  int nworst_nets = std::ceil(nets.size() * worst_slack_nets_percent_ / 100.0);
  if (nets.size() > nworst_nets) {
    nets.resize(nworst_nets);
  }
  return nets;
}

std::optional<Slack> Resizer::resizeNetSlack(const Net* net)
{
  auto it = net_slack_map_.find(net);
  if (it == net_slack_map_.end()) {
    return {};
  }
  return it->second;
}

std::optional<Slack> Resizer::resizeNetSlack(const dbNet* db_net)
{
  const Net* net = db_network_->dbToSta(db_net);
  return resizeNetSlack(net);
}

////////////////////////////////////////////////////////////////

// API for logic resynthesis
PinSet Resizer::findFaninFanouts(PinSet& end_pins)
{
  // Abbreviated copyState
  sta_->ensureLevelized();
  graph_ = sta_->graph();

  VertexSet ends(graph_);
  for (const Pin* pin : end_pins) {
    Vertex* end = graph_->pinLoadVertex(pin);
    ends.insert(end);
  }
  PinSet fanin_fanout_pins(db_network_);
  VertexSet fanin_fanouts = findFaninFanouts(ends);
  for (Vertex* vertex : fanin_fanouts) {
    fanin_fanout_pins.insert(vertex->pin());
  }
  return fanin_fanout_pins;
}

VertexSet Resizer::findFaninFanouts(VertexSet& ends)
{
  // Search backwards from ends to fanin register outputs and input ports.
  VertexSet fanin_roots = findFaninRoots(ends);
  // Search forward from register outputs.
  VertexSet fanouts = findFanouts(fanin_roots);
  return fanouts;
}

// Find source pins for logic fanin of ends.
PinSet Resizer::findFanins(PinSet& end_pins)
{
  // Abbreviated copyState
  sta_->ensureLevelized();
  graph_ = sta_->graph();

  VertexSet ends(graph_);
  for (const Pin* pin : end_pins) {
    Vertex* end = graph_->pinLoadVertex(pin);
    ends.insert(end);
  }

  SearchPredNonReg2 pred(sta_);
  BfsBkwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex* vertex : ends) {
    iter.enqueueAdjacentVertices(vertex);
  }

  PinSet fanins(db_network_);
  while (iter.hasNext()) {
    Vertex* vertex = iter.next();
    if (isRegOutput(vertex) || network_->isTopLevelPort(vertex->pin())) {
      continue;
    }
    iter.enqueueAdjacentVertices(vertex);
    fanins.insert(vertex->pin());
  }
  return fanins;
}

// Find roots for logic fanin of ends.
VertexSet Resizer::findFaninRoots(VertexSet& ends)
{
  SearchPredNonReg2 pred(sta_);
  BfsBkwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex* vertex : ends) {
    iter.enqueueAdjacentVertices(vertex);
  }

  VertexSet roots(graph_);
  while (iter.hasNext()) {
    Vertex* vertex = iter.next();
    if (isRegOutput(vertex) || network_->isTopLevelPort(vertex->pin())) {
      roots.insert(vertex);
    } else {
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return roots;
}

bool Resizer::isRegOutput(Vertex* vertex)
{
  LibertyPort* port = network_->libertyPort(vertex->pin());
  if (port) {
    LibertyCell* cell = port->libertyCell();
    for (TimingArcSet* arc_set : cell->timingArcSets(nullptr, port)) {
      if (arc_set->role()->genericRole() == TimingRole::regClkToQ()) {
        return true;
      }
    }
  }
  return false;
}

VertexSet Resizer::findFanouts(VertexSet& reg_outs)
{
  VertexSet fanouts(graph_);
  sta::SearchPredNonLatch2 pred(sta_);
  BfsFwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex* reg_out : reg_outs) {
    iter.enqueueAdjacentVertices(reg_out);
  }

  while (iter.hasNext()) {
    Vertex* vertex = iter.next();
    if (!isRegister(vertex)) {
      fanouts.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return fanouts;
}

bool Resizer::isRegister(Vertex* vertex)
{
  LibertyPort* port = network_->libertyPort(vertex->pin());
  if (port) {
    LibertyCell* cell = port->libertyCell();
    return cell && cell->hasSequentials();
  }
  return false;
}

////////////////////////////////////////////////////////////////

double Resizer::area(Cell* cell)
{
  return area(db_network_->staToDb(cell));
}

double Resizer::area(dbMaster* master)
{
  if (!master->isCoreAutoPlaceable()) {
    return 0;
  }
  return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

double Resizer::dbuToMeters(int dist) const
{
  return dist / (dbu_ * 1e+6);
}

int Resizer::metersToDbu(double dist) const
{
  if (dist < 0) {
    logger_->error(
        RSZ, 86, "metersToDbu({}) cannot convert negative distances", dist);
  }
  // sta::INF is passed to this function in some cases. Protect against
  // overflow conditions.
  double distance = dist * dbu_ * 1e+6;
  return static_cast<int>(std::lround(distance)
                          & std::numeric_limits<int>::max());
}

void Resizer::setMaxUtilization(double max_utilization)
{
  max_area_ = coreArea() * max_utilization;
}

bool Resizer::overMaxArea()
{
  return max_area_ && fuzzyGreaterEqual(design_area_, max_area_);
}

void Resizer::setDontUse(LibertyCell* cell, bool dont_use)
{
  if (dont_use) {
    dont_use_.insert(cell);
  } else {
    dont_use_.erase(cell);
  }

  // Reset buffer set and swappable cells cache to ensure they honor dont_use_
  buffer_cells_.clear();
  buffer_fast_sizes_.clear();
  buffer_lowest_drive_ = nullptr;
  swappable_cells_cache_.clear();
}

void Resizer::resetDontUse()
{
  dont_use_.clear();

  // Reset buffer set and swappable cells cache to ensure they honor dont_use_
  buffer_cells_.clear();
  buffer_fast_sizes_.clear();
  buffer_lowest_drive_ = nullptr;
  swappable_cells_cache_.clear();

  // recopy in liberty cell dont uses
  copyDontUseFromLiberty();
}

bool Resizer::dontUse(const LibertyCell* cell)
{
  return dont_use_.hasKey(const_cast<LibertyCell*>(cell));
}

void Resizer::reportDontUse() const
{
  logger_->report("Don't Use Cells:");

  if (dont_use_.empty()) {
    logger_->report("  none");
  } else {
    for (auto* cell : dont_use_) {
      if (!isLinkCell(cell)) {
        continue;
      }
      logger_->report("  {}", cell->name());
    }
  }
}

void Resizer::setDontTouch(const Instance* inst, bool dont_touch)
{
  dbInst* db_inst = db_network_->staToDb(inst);
  db_inst->setDoNotTouch(dont_touch);
}

bool Resizer::dontTouch(const Instance* inst)
{
  dbInst* db_inst = db_network_->staToDb(inst);
  if (!db_inst) {
    return false;
  }
  return db_inst->isDoNotTouch();
}

void Resizer::setDontTouch(const Net* net, bool dont_touch)
{
  dbNet* db_net = db_network_->staToDb(net);
  db_net->setDoNotTouch(dont_touch);
}

bool Resizer::dontTouch(const Net* net)
{
  odb::dbNet* db_net = nullptr;
  odb::dbModNet* db_mod_net = nullptr;
  db_network_->staToDb(net, db_net, db_mod_net);
  if (db_net == nullptr) {
    return false;
  }
  return db_net->isDoNotTouch();
}

bool Resizer::dontTouch(const Pin* pin)
{
  return dontTouch(network_->instance(pin)) || dontTouch(network_->net(pin));
}

void Resizer::reportDontTouch()
{
  initBlock();

  std::set<odb::dbInst*> insts;
  std::set<odb::dbNet*> nets;

  for (auto* inst : block_->getInsts()) {
    if (inst->isDoNotTouch()) {
      insts.insert(inst);
    }
  }

  for (auto* net : block_->getNets()) {
    if (net->isDoNotTouch()) {
      nets.insert(net);
    }
  }

  logger_->report("Don't Touch Instances:");

  if (insts.empty()) {
    logger_->report("  none");
  } else {
    for (auto* inst : insts) {
      logger_->report("  {}", inst->getName());
    }
  }

  logger_->report("Don't Touch Nets:");
  if (nets.empty()) {
    logger_->report("  none");
  } else {
    for (auto* net : nets) {
      logger_->report("  {}", net->getName());
    }
  }
}

////////////////////////////////////////////////////////////////

// Find a target slew for the libraries and then
// a target load for each cell that gives the target slew.
void Resizer::findTargetLoads()
{
  if (target_load_map_ == nullptr) {
    // Find target slew across all buffers in the libraries.
    findBufferTargetSlews();

    target_load_map_ = std::make_unique<CellTargetLoadMap>();
    // Find target loads at the tgt_slew_corner.
    int lib_ap_index = tgt_slew_corner_->libertyIndex(max_);
    LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      LibertyLibrary* lib = lib_iter->next();
      LibertyCellIterator cell_iter(lib);
      while (cell_iter.hasNext()) {
        LibertyCell* cell = cell_iter.next();
        if (isLinkCell(cell) && !dontUse(cell)) {
          LibertyCell* corner_cell = cell->cornerCell(lib_ap_index);
          float tgt_load;
          bool exists;
          target_load_map_->findKey(corner_cell, tgt_load, exists);
          if (!exists) {
            tgt_load = findTargetLoad(corner_cell);
            (*target_load_map_)[corner_cell] = tgt_load;
          }
          // Map link cell to corner cell target load.
          if (cell != corner_cell) {
            (*target_load_map_)[cell] = tgt_load;
          }
        }
      }
    }
    delete lib_iter;
  }
}

float Resizer::targetLoadCap(LibertyCell* cell)
{
  float load_cap = 0.0;
  bool exists;
  target_load_map_->findKey(cell, load_cap, exists);
  if (!exists) {
    logger_->error(RSZ, 68, "missing target load cap.");
  }
  return load_cap;
}

float Resizer::findTargetLoad(LibertyCell* cell)
{
  float target_load_sum = 0.0;
  int arc_count = 0;
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    const TimingRole* role = arc_set->role();
    if (!role->isTimingCheck() && role != TimingRole::tristateDisable()
        && role != TimingRole::tristateEnable()
        && role != TimingRole::clockTreePathMin()
        && role != TimingRole::clockTreePathMax()) {
      for (TimingArc* arc : arc_set->arcs()) {
        int in_rf_index = arc->fromEdge()->asRiseFall()->index();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        float arc_target_load = findTargetLoad(
            cell, arc, tgt_slews_[in_rf_index], tgt_slews_[out_rf_index]);
        debugPrint(logger_,
                   RSZ,
                   "target_load",
                   3,
                   "{} {} -> {} {} target_load = {:.2e}",
                   cell->name(),
                   arc->from()->name(),
                   arc->to()->name(),
                   arc->toEdge()->to_string(),
                   arc_target_load);
        target_load_sum += arc_target_load;
        arc_count++;
      }
    }
  }
  float target_load = arc_count ? target_load_sum / arc_count : 0.0;
  debugPrint(logger_,
             RSZ,
             "target_load",
             2,
             "{} target_load = {:.2e}",
             cell->name(),
             target_load);
  return target_load;
}

// Find the load capacitance that will cause the output slew
// to be equal to out_slew.
float Resizer::findTargetLoad(LibertyCell* cell,
                              TimingArc* arc,
                              Slew in_slew,
                              Slew out_slew)
{
  GateTimingModel* model = dynamic_cast<GateTimingModel*>(arc->model());
  if (model) {
    // load_cap1 lower bound
    // load_cap2 upper bound
    double load_cap1 = 0.0;
    double load_cap2 = 1.0e-12;  // 1pF
    double tol = .01;            // 1%
    double diff1 = gateSlewDiff(cell, arc, model, in_slew, load_cap1, out_slew);
    if (diff1 > 0.0) {
      // Zero load cap out_slew is higher than the target.
      return 0.0;
    }
    double diff2 = gateSlewDiff(cell, arc, model, in_slew, load_cap2, out_slew);
    // binary search for diff = 0.
    while (abs(load_cap1 - load_cap2) > max(load_cap1, load_cap2) * tol) {
      if (diff2 < 0.0) {
        load_cap1 = load_cap2;
        load_cap2 *= 2;
        diff2 = gateSlewDiff(cell, arc, model, in_slew, load_cap2, out_slew);
      } else {
        double load_cap3 = (load_cap1 + load_cap2) / 2.0;
        const double diff3
            = gateSlewDiff(cell, arc, model, in_slew, load_cap3, out_slew);
        if (diff3 < 0.0) {
          load_cap1 = load_cap3;
        } else {
          load_cap2 = load_cap3;
          diff2 = diff3;
        }
      }
    }
    return load_cap1;
  }
  return 0.0;
}

// objective function
Slew Resizer::gateSlewDiff(LibertyCell* cell,
                           TimingArc* arc,
                           GateTimingModel* model,
                           Slew in_slew,
                           float load_cap,
                           Slew out_slew)

{
  const Pvt* pvt = tgt_slew_dcalc_ap_->operatingConditions();
  ArcDelay arc_delay;
  Slew arc_slew;
  model->gateDelay(pvt, in_slew, load_cap, false, arc_delay, arc_slew);
  return arc_slew - out_slew;
}

////////////////////////////////////////////////////////////////

Slew Resizer::targetSlew(const RiseFall* rf)
{
  return tgt_slews_[rf->index()];
}

// Find target slew across all buffers in the libraries.
void Resizer::findBufferTargetSlews()
{
  tgt_slews_ = {0.0};
  tgt_slew_corner_ = nullptr;

  for (Corner* corner : *sta_->corners()) {
    int lib_ap_index = corner->libertyIndex(max_);
    const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
    const Pvt* pvt = dcalc_ap->operatingConditions();
    // Average slews across buffers at corner.
    Slew slews[RiseFall::index_count]{0.0};
    int counts[RiseFall::index_count]{0};
    for (LibertyCell* buffer : buffer_cells_) {
      LibertyCell* corner_buffer = buffer->cornerCell(lib_ap_index);
      findBufferTargetSlews(corner_buffer, pvt, slews, counts);
    }
    Slew slew_rise
        = slews[RiseFall::riseIndex()] / counts[RiseFall::riseIndex()];
    Slew slew_fall
        = slews[RiseFall::fallIndex()] / counts[RiseFall::fallIndex()];
    // Use the target slews from the slowest corner,
    // and resize using that corner.
    if (slew_rise > tgt_slews_[RiseFall::riseIndex()]) {
      tgt_slews_[RiseFall::riseIndex()] = slew_rise;
      tgt_slews_[RiseFall::fallIndex()] = slew_fall;
      tgt_slew_corner_ = corner;
      tgt_slew_dcalc_ap_ = corner->findDcalcAnalysisPt(max_);
    }
  }

  debugPrint(logger_,
             RSZ,
             "target_load",
             1,
             "target slew corner {} = {}/{}",
             tgt_slew_corner_->name(),
             delayAsString(tgt_slews_[RiseFall::riseIndex()], sta_, 3),
             delayAsString(tgt_slews_[RiseFall::fallIndex()], sta_, 3));
}

void Resizer::findBufferTargetSlews(LibertyCell* buffer,
                                    const Pvt* pvt,
                                    // Return values.
                                    Slew slews[],
                                    int counts[])
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  for (TimingArcSet* arc_set : buffer->timingArcSets(input, output)) {
    for (TimingArc* arc : arc_set->arcs()) {
      GateTimingModel* model = dynamic_cast<GateTimingModel*>(arc->model());
      if (model != nullptr) {
        const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        const RiseFall* out_rf = arc->toEdge()->asRiseFall();
        float in_cap = input->capacitance(in_rf, max_);
        float load_cap = in_cap * tgt_slew_load_cap_factor;
        ArcDelay arc_delay;
        Slew arc_slew;
        model->gateDelay(pvt, 0.0, load_cap, false, arc_delay, arc_slew);
        model->gateDelay(pvt, arc_slew, load_cap, false, arc_delay, arc_slew);
        slews[out_rf->index()] += arc_slew;
        counts[out_rf->index()]++;
      }
    }
  }
}

////////////////////////////////////////////////////////////////

// Repair tie hi/low net driver fanout by duplicating the
// tie hi/low instances for every pin connected to tie hi/low instances.
void Resizer::repairTieFanout(LibertyPort* tie_port,
                              double separation,  // meters
                              bool verbose)
{
  initDesignArea();
  Instance* top_inst = network_->topInstance();
  LibertyCell* tie_cell = tie_port->libertyCell();
  InstanceSeq insts;
  findCellInstances(tie_cell, insts);
  int tie_count = 0;
  int separation_dbu = metersToDbu(separation);
  for (const Instance* inst : insts) {
    if (!dontTouch(inst)) {
      Pin* drvr_pin = network_->findPin(inst, tie_port);
      if (drvr_pin) {
        dbNet* db_net = db_network_->flatNet(drvr_pin);
        if (!db_net) {
          continue;
        }
        Net* net = db_network_->dbToSta(db_net);
        if (net && !dontTouch(net)) {
          NetConnectedPinIterator* pin_iter
              = network_->connectedPinIterator(net);
          bool keep_tie = false;
          while (pin_iter->hasNext()) {
            const Pin* load = pin_iter->next();
            if (!(db_network_->isFlat(load))) {
              continue;
            }

            Instance* load_inst = network_->instance(load);
            if (dontTouch(load_inst)) {
              keep_tie = true;
            } else if (load != drvr_pin) {
              // Make tie inst.
              Point tie_loc = tieLocation(load, separation_dbu);
              const char* inst_name = network_->name(load_inst);
              string tie_name = makeUniqueInstName(inst_name, true);
              Instance* tie
                  = makeInstance(tie_cell, tie_name.c_str(), top_inst, tie_loc);

              // Put the tie cell instance in the same module with the load
              // it drives.
              if (!network_->isTopInstance(load_inst)) {
                dbInst* load_inst_odb = db_network_->staToDb(load_inst);
                dbInst* tie_odb = db_network_->staToDb(tie);
                load_inst_odb->getModule()->addInst(tie_odb);
              }

              // Make tie output net.
              Net* load_net = makeUniqueNet();

              // Connect tie inst output.
              sta_->connectPin(tie, tie_port, load_net);

              // Connect load to tie output net.
              sta_->disconnectPin(const_cast<Pin*>(load));
              Port* load_port = network_->port(load);
              sta_->connectPin(load_inst, load_port, load_net);

              designAreaIncr(area(db_network_->cell(tie_cell)));
              tie_count++;
            }
          }
          delete pin_iter;

          if (keep_tie) {
            continue;
          }

          // Delete inst output net.
          Pin* tie_pin = network_->findPin(inst, tie_port);
          dbNet* tie_flat_net = db_network_->flatNet(tie_pin);
          Net* tie_net = db_network_->dbToSta(tie_flat_net);

          // network_->net(tie_pin);
          sta_->deleteNet(tie_net);
          parasitics_invalid_.erase(tie_net);
          // Delete the tie instance if no other ports are in use.
          // A tie cell can have both tie hi and low outputs.
          bool has_other_fanout = false;
          std::unique_ptr<InstancePinIterator> inst_pin_iter{
              network_->pinIterator(inst)};
          while (inst_pin_iter->hasNext()) {
            Pin* pin = inst_pin_iter->next();
            if (pin != drvr_pin) {
              // hier fix
              Net* net = db_network_->dbToSta(db_network_->flatNet(pin));
              if (net && !network_->isPower(net) && !network_->isGround(net)) {
                has_other_fanout = true;
                break;
              }
            }
          }
          if (!has_other_fanout) {
            sta_->deleteInstance(const_cast<Instance*>(inst));
          }
        }
      }
    }
  }

  if (tie_count > 0) {
    logger_->info(
        RSZ, 42, "Inserted {} tie {} instances.", tie_count, tie_cell->name());
    level_drvr_vertices_valid_ = false;
  }
}

void Resizer::findCellInstances(LibertyCell* cell,
                                // Return value.
                                InstanceSeq& insts)
{
  LeafInstanceIterator* inst_iter = network_->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    Instance* inst = inst_iter->next();
    if (network_->libertyCell(inst) == cell) {
      insts.emplace_back(inst);
    }
  }
  delete inst_iter;
}

// Place the tie instance on the side of the load pin.
Point Resizer::tieLocation(const Pin* load, int separation)
{
  Point load_loc = db_network_->location(load);
  int load_x = load_loc.getX();
  int load_y = load_loc.getY();
  int tie_x = load_x;
  int tie_y = load_y;
  if (!(network_->isTopLevelPort(load) || !db_network_->isFlat(load))) {
    dbInst* db_inst = db_network_->staToDb(network_->instance(load));
    dbBox* bbox = db_inst->getBBox();
    int left_dist = abs(load_x - bbox->xMin());
    int right_dist = abs(load_x - bbox->xMax());
    int bot_dist = abs(load_y - bbox->yMin());
    int top_dist = abs(load_y - bbox->yMax());
    if (left_dist < right_dist && left_dist < bot_dist
        && left_dist < top_dist) {
      // left
      tie_x -= separation;
    }
    if (right_dist < left_dist && right_dist < bot_dist
        && right_dist < top_dist) {
      // right
      tie_x += separation;
    }
    if (bot_dist < left_dist && bot_dist < right_dist && bot_dist < top_dist) {
      // bot
      tie_y -= separation;
    }
    if (top_dist < left_dist && top_dist < right_dist && top_dist < bot_dist) {
      // top
      tie_y += separation;
    }
  }
  return Point(tie_x, tie_y);
}

////////////////////////////////////////////////////////////////

void Resizer::reportLongWires(int count, int digits)
{
  initBlock();
  graph_ = sta_->ensureGraph();
  sta_->ensureClkNetwork();
  VertexSeq drvrs;
  findLongWires(drvrs);
  logger_->report("Driver    length delay");
  const Corner* corner = sta_->cmdCorner();
  double wire_res = wireSignalResistance(corner);
  double wire_cap = wireSignalCapacitance(corner);
  int i = 0;
  for (Vertex* drvr : drvrs) {
    Pin* drvr_pin = drvr->pin();
    double wire_length = dbuToMeters(maxLoadManhattenDistance(drvr));
    double steiner_length = dbuToMeters(findMaxSteinerDist(drvr, corner));
    double delay = (wire_length * wire_res) * (wire_length * wire_cap) * 0.5;
    logger_->report("{} manhtn {} steiner {} {}",
                    sdc_network_->pathName(drvr_pin),
                    units_->distanceUnit()->asString(wire_length, 1),
                    units_->distanceUnit()->asString(steiner_length, 1),
                    delayAsString(delay, sta_, digits));
    if (i == count) {
      break;
    }
    i++;
  }
}

using DrvrDist = std::pair<Vertex*, int>;

void Resizer::findLongWires(VertexSeq& drvrs)
{
  Vector<DrvrDist> drvr_dists;
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex* vertex = vertex_iter.next();
    if (vertex->isDriver(network_)) {
      Pin* pin = vertex->pin();
      // Hands off the clock nets.
      if (!sta_->isClock(pin) && !vertex->isConstant()
          && !vertex->isDisabledConstraint()) {
        drvr_dists.emplace_back(
            DrvrDist(vertex, maxLoadManhattenDistance(vertex)));
      }
    }
  }
  sort(drvr_dists, [](const DrvrDist& drvr_dist1, const DrvrDist& drvr_dist2) {
    return drvr_dist1.second > drvr_dist2.second;
  });
  drvrs.reserve(drvr_dists.size());
  for (DrvrDist& drvr_dist : drvr_dists) {
    drvrs.emplace_back(drvr_dist.first);
  }
}

// Find the maximum distance along steiner tree branches from
// the driver to loads (in dbu).
int Resizer::findMaxSteinerDist(Vertex* drvr, const Corner* corner)

{
  Pin* drvr_pin = drvr->pin();
  BufferedNetPtr bnet = makeBufferedNetSteiner(drvr_pin, corner);
  if (bnet) {
    return bnet->maxLoadWireLength();
  }
  return 0;
}

double Resizer::maxLoadManhattenDistance(const Net* net)
{
  NetPinIterator* pin_iter = network_->pinIterator(net);
  int max_dist = 0;
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->isDriver(pin)) {
      Vertex* drvr = graph_->pinDrvrVertex(pin);
      if (drvr) {
        int dist = maxLoadManhattenDistance(drvr);
        max_dist = max(max_dist, dist);
      }
    }
  }
  delete pin_iter;
  return dbuToMeters(max_dist);
}

int Resizer::maxLoadManhattenDistance(Vertex* drvr)
{
  int max_dist = 0;
  Point drvr_loc = db_network_->location(drvr->pin());
  VertexOutEdgeIterator edge_iter(drvr, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    Vertex* load = edge->to(graph_);
    Point load_loc = db_network_->location(load->pin());
    int dist = Point::manhattanDistance(drvr_loc, load_loc);
    max_dist = max(max_dist, dist);
  }
  return max_dist;
}

////////////////////////////////////////////////////////////////

NetSeq* Resizer::findFloatingNets()
{
  NetSeq* floating_nets = new NetSeq;
  NetIterator* net_iter = network_->netIterator(network_->topInstance());
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    PinSeq loads;
    PinSeq drvrs;
    PinSet visited_drvrs(db_network_);
    FindNetDrvrLoads visitor(nullptr, visited_drvrs, loads, drvrs, network_);
    network_->visitConnectedPins(net, visitor);
    if (drvrs.empty() && !loads.empty()) {
      floating_nets->emplace_back(net);
    }
  }
  delete net_iter;
  sort(floating_nets, sta::NetPathNameLess(network_));
  return floating_nets;
}

PinSet* Resizer::findFloatingPins()
{
  PinSet* floating_pins = new PinSet(network_);

  // Find instances with inputs without a net
  LeafInstanceIterator* leaf_iter = network_->leafInstanceIterator();
  while (leaf_iter->hasNext()) {
    const Instance* inst = leaf_iter->next();
    InstancePinIterator* pin_iter = network_->pinIterator(inst);
    while (pin_iter->hasNext()) {
      Pin* pin = pin_iter->next();
      if (network_->direction(pin) != sta::PortDirection::input()) {
        continue;
      }
      if (network_->net(pin) != nullptr) {
        continue;
      }
      floating_pins->insert(pin);
    }
    delete pin_iter;
  }
  delete leaf_iter;

  return floating_pins;
}

NetSeq* Resizer::findOverdrivenNets(bool include_parallel_driven)
{
  NetSeq* overdriven_nets = new NetSeq;
  std::unique_ptr<NetIterator> net_iter(
      network_->netIterator(network_->topInstance()));
  while (net_iter->hasNext()) {
    Net* net = net_iter->next();
    PinSeq loads;
    PinSeq drvrs;
    PinSet visited_drvrs(db_network_);
    FindNetDrvrLoads visitor(nullptr, visited_drvrs, loads, drvrs, network_);
    network_->visitConnectedPins(net, visitor);
    if (drvrs.size() > 1) {
      bool all_tristate = true;
      for (const Pin* drvr : drvrs) {
        if (!isTristateDriver(drvr)) {
          all_tristate = false;
        }
      }

      if (all_tristate) {
        continue;
      }

      if (!include_parallel_driven) {
        bool allowed = true;
        bool has_buffers = false;
        bool has_inverters = false;
        std::set<sta::Net*> input_nets;

        for (const Pin* drvr : drvrs) {
          const Instance* inst = network_->instance(drvr);
          const LibertyCell* cell = network_->libertyCell(inst);
          if (cell == nullptr) {
            allowed = false;
            break;
          }
          if (cell->isBuffer()) {
            has_buffers = true;
          }
          if (cell->isInverter()) {
            has_inverters = true;
          }

          std::unique_ptr<InstancePinIterator> inst_pin_iter(
              network_->pinIterator(inst));
          while (inst_pin_iter->hasNext()) {
            Pin* inst_pin = inst_pin_iter->next();
            sta::PortDirection* dir = network_->direction(inst_pin);
            if (dir->isAnyInput()) {
              input_nets.insert(network_->net(inst_pin));
            }
          }
        }

        if (has_inverters && has_buffers) {
          allowed = false;
        }

        if (input_nets.size() > 1) {
          allowed = false;
        }

        if (allowed) {
          continue;
        }
      }
      overdriven_nets->emplace_back(net);
    }
  }
  sort(overdriven_nets, sta::NetPathNameLess(network_));
  return overdriven_nets;
}

////////////////////////////////////////////////////////////////

// TODO:
//----
// when making a unique net name search within the scope of the
// containing module only (parent scope module)which is passed in.
// This requires scoping nets in the module in hierarchical mode
//(as was done with dbInsts) and will require changing the
// method: dbNetwork::name).
// Currently all nets are scoped within a dbBlock.
//

string Resizer::makeUniqueNetName(Instance* parent_scope)
{
  string node_name;
  bool prefix_name = false;
  if (parent_scope && parent_scope != network_->topInstance()) {
    prefix_name = true;
  }
  Instance* top_inst = prefix_name ? parent_scope : network_->topInstance();
  do {
    if (prefix_name) {
      std::string parent_name = network_->name(parent_scope);
      node_name = fmt::format("{}/net{}", parent_name, unique_net_index_++);
    } else {
      node_name = fmt::format("net{}", unique_net_index_++);
    }
  } while (network_->findNet(top_inst, node_name.c_str())
           //
           // in hierarchical mode we check the uniqueness globally.
           // TODO:change scoping of nets so we never
           // have to do this, as it is obviously slow.
           //
           || (db_network_->hasHierarchy()
               && db_network_->findNetAllScopes(node_name.c_str())));
  return node_name;
}

Net* Resizer::makeUniqueNet()
{
  string net_name = makeUniqueNetName();
  Instance* parent = db_network_->topInstance();
  return db_network_->makeNet(net_name.c_str(), parent);
}

string Resizer::makeUniqueInstName(const char* base_name)
{
  return makeUniqueInstName(base_name, false);
}

string Resizer::makeUniqueInstName(const char* base_name, bool underscore)
{
  string inst_name;
  do {
    // sta::stringPrint can lead to string overflow and fatal
    if (underscore) {
      inst_name = fmt::format("{}_{}", base_name, unique_inst_index_++);
    } else {
      inst_name = fmt::format("{}{}", base_name, unique_inst_index_++);
    }
    //
    // NOTE: TODO: The scoping should be within
    // the dbModule scope for the instance, not the whole network.
    // dbInsts are already scoped within a dbModule
    // To get the dbModule for a dbInst used inst -> getModule
    // then search within that scope. That way the instance name
    // does not have to be some massive string like root/X/Y/U1.
    //
  } while (network_->findInstance(inst_name.c_str()));
  return inst_name;
}

float Resizer::portFanoutLoad(LibertyPort* port) const
{
  float fanout_load;
  bool exists;
  port->fanoutLoad(fanout_load, exists);
  if (!exists) {
    LibertyLibrary* lib = port->libertyLibrary();
    lib->defaultFanoutLoad(fanout_load, exists);
  }
  if (exists) {
    return fanout_load;
  }
  return 0.0;
}

float Resizer::bufferDelay(LibertyCell* buffer_cell,
                           const RiseFall* rf,
                           float load_cap,
                           const DcalcAnalysisPt* dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return gate_delays[rf->index()];
}

float Resizer::bufferDelay(LibertyCell* buffer_cell,
                           float load_cap,
                           const DcalcAnalysisPt* dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return max(gate_delays[RiseFall::riseIndex()],
             gate_delays[RiseFall::fallIndex()]);
}

void Resizer::bufferDelays(LibertyCell* buffer_cell,
                           float load_cap,
                           const DcalcAnalysisPt* dcalc_ap,
                           // Return values.
                           ArcDelay delays[RiseFall::index_count],
                           Slew slews[RiseFall::index_count])
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  gateDelays(output, load_cap, dcalc_ap, delays, slews);
}

// Rise/fall delays across all timing arcs into drvr_port.
// Uses target slew for input slew.
void Resizer::gateDelays(const LibertyPort* drvr_port,
                         const float load_cap,
                         const DcalcAnalysisPt* dcalc_ap,
                         // Return values.
                         ArcDelay delays[RiseFall::index_count],
                         Slew slews[RiseFall::index_count])
{
  for (int rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = -INF;
    slews[rf_index] = -INF;
  }
  LibertyCell* cell = drvr_port->libertyCell();
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (TimingArc* arc : arc_set->arcs()) {
        const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        // use annotated slews if available
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
        const Slew& drvr_slew = dcalc_result.drvrSlew();
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        slews[out_rf_index] = max(slews[out_rf_index], drvr_slew);
      }
    }
  }
}

// Rise/fall delays across all timing arcs into drvr_port.
// Takes input slews and load cap
void Resizer::gateDelays(const LibertyPort* drvr_port,
                         const float load_cap,
                         const Slew in_slews[RiseFall::index_count],
                         const DcalcAnalysisPt* dcalc_ap,
                         // Return values.
                         ArcDelay delays[RiseFall::index_count],
                         Slew out_slews[RiseFall::index_count])
{
  for (int rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = -INF;
    out_slews[rf_index] = -INF;
  }
  LibertyCell* cell = drvr_port->libertyCell();
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (TimingArc* arc : arc_set->arcs()) {
        const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        LoadPinIndexMap load_pin_index_map(network_);
        ArcDcalcResult dcalc_result
            = arc_delay_calc_->gateDelay(nullptr,
                                         arc,
                                         in_slews[in_rf->index()],
                                         load_cap,
                                         nullptr,
                                         load_pin_index_map,
                                         dcalc_ap);

        const ArcDelay& gate_delay = dcalc_result.gateDelay();
        const Slew& drvr_slew = dcalc_result.drvrSlew();
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        out_slews[out_rf_index] = max(out_slews[out_rf_index], drvr_slew);
      }
    }
  }
}

ArcDelay Resizer::gateDelay(const LibertyPort* drvr_port,
                            const RiseFall* rf,
                            const float load_cap,
                            const DcalcAnalysisPt* dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  return delays[rf->index()];
}

ArcDelay Resizer::gateDelay(const LibertyPort* drvr_port,
                            const float load_cap,
                            const DcalcAnalysisPt* dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  return max(delays[RiseFall::riseIndex()], delays[RiseFall::fallIndex()]);
}

////////////////////////////////////////////////////////////////

double Resizer::findMaxWireLength(bool issue_error)
{
  init();
  checkLibertyForAllCorners();
  findBuffers();
  findTargetLoads();
  return findMaxWireLength1(issue_error);
}

double Resizer::findMaxWireLength1(bool issue_error)
{
  std::optional<double> max_length;
  for (const Corner* corner : *sta_->corners()) {
    if (wireSignalResistance(corner) <= 0.0) {
      if (issue_error) {
        logger_->warn(RSZ,
                      88,
                      "Corner: {} has no wire signal resistance value.",
                      corner->name());
      }
      continue;
    }

    // buffer_cells_ is required to be non-empty.
    for (LibertyCell* buffer_cell : buffer_cells_) {
      const double buffer_length = findMaxWireLength(buffer_cell, corner);
      max_length = min(max_length.value_or(INF), buffer_length);
      debugPrint(logger_,
                 RSZ,
                 "max_wire_length",
                 1,
                 "Buffer {} has max_wire_length {}",
                 buffer_cell->name(),
                 units_->distanceUnit()->asString(buffer_length));
    }
  }

  if (!max_length.has_value()) {
    if (issue_error) {
      logger_->error(RSZ,
                     89,
                     "Could not find a resistance value for any corner. Cannot "
                     "evaluate max wire length for buffer. Check over your "
                     "`set_wire_rc` configuration");
    } else {
      max_length = -std::numeric_limits<double>::infinity();
    }
  }

  return max_length.value();
}

// Find the max wire length before it is faster to split the wire
// in half with a buffer (in meters).
double Resizer::findMaxWireLength(LibertyCell* buffer_cell,
                                  const Corner* corner)
{
  initBlock();
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  return findMaxWireLength(drvr_port, corner);
}

double Resizer::findMaxWireLength(LibertyPort* drvr_port, const Corner* corner)
{
  LibertyCell* cell = drvr_port->libertyCell();
  if (db_network_->staToDb(cell) == nullptr) {
    logger_->error(RSZ, 70, "no LEF cell for {}.", cell->name());
  }
  // Make a (hierarchical) block to use as a scratchpad.
  dbBlock* block
      = dbBlock::create(block_, "wire_delay", block_->getTech(), '/');
  std::unique_ptr<dbSta> sta = sta_->makeBlockSta(block);

  const double drvr_r = drvr_port->driveResistance();
  // wire_length_low - lower bound
  // wire_length_high - upper bound
  double wire_length_low = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  double wire_length_high = drvr_r / wireSignalResistance(corner);
  const double tol = .01;  // 1%
  double diff_ub = splitWireDelayDiff(wire_length_high, cell, sta);
  // binary search for diff = 0.
  while (abs(wire_length_low - wire_length_high)
         > max(wire_length_low, wire_length_high) * tol) {
    if (diff_ub < 0.0) {  // unbuffered < 2 * buffered
      wire_length_low = wire_length_high;
      wire_length_high *= 2;
      diff_ub = splitWireDelayDiff(wire_length_high, cell, sta);
    } else {  // unbuffered > 2 * buffered
      const double wire_length_mid = (wire_length_low + wire_length_high) / 2.0;
      const double diff_mid = splitWireDelayDiff(wire_length_mid, cell, sta);
      if (diff_mid < 0.0) {
        wire_length_low = wire_length_mid;
      } else {
        wire_length_high = wire_length_mid;
        diff_ub = diff_mid;
      }
    }
  }
  dbBlock::destroy(block);
  return wire_length_low;
}

// objective function
double Resizer::splitWireDelayDiff(double wire_length,
                                   LibertyCell* buffer_cell,
                                   std::unique_ptr<dbSta>& sta)
{
  Delay delay1, delay2;
  Slew slew1, slew2;
  bufferWireDelay(buffer_cell, wire_length, sta, delay1, slew1);
  bufferWireDelay(buffer_cell, wire_length / 2, sta, delay2, slew2);
  return delay1 - delay2 * 2;
}

// For tcl accessor.
void Resizer::bufferWireDelay(LibertyCell* buffer_cell,
                              double wire_length,  // meters
                              // Return values.
                              Delay& delay,
                              Slew& slew)
{
  // Make a (hierarchical) block to use as a scratchpad.
  dbBlock* block
      = dbBlock::create(block_, "wire_delay", block_->getTech(), '/');
  std::unique_ptr<dbSta> sta = sta_->makeBlockSta(block);
  bufferWireDelay(buffer_cell, wire_length, sta, delay, slew);
  dbBlock::destroy(block);
}

void Resizer::bufferWireDelay(LibertyCell* buffer_cell,
                              double wire_length,  // meters
                              std::unique_ptr<dbSta>& sta,
                              // Return values.
                              Delay& delay,
                              Slew& slew)
{
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  cellWireDelay(drvr_port, load_port, wire_length, sta, delay, slew);
}

// Cell delay plus wire delay.
// Use target slew for input slew.
// drvr_port and load_port do not have to be the same liberty cell.
void Resizer::cellWireDelay(LibertyPort* drvr_port,
                            LibertyPort* load_port,
                            double wire_length,  // meters
                            std::unique_ptr<dbSta>& sta,
                            // Return values.
                            Delay& delay,
                            Slew& slew)
{
  Parasitics* parasitics = sta->parasitics();
  Network* network = sta->network();
  ArcDelayCalc* arc_delay_calc = sta->arcDelayCalc();
  Corners* corners = sta->corners();
  corners->copy(sta_->corners());
  sta->sdc()->makeCornersAfter(corners);

  Instance* top_inst = network->topInstance();
  // Tmp net for parasitics to live on.
  Net* net = sta->makeNet("wire", top_inst);
  LibertyCell* drvr_cell = drvr_port->libertyCell();
  LibertyCell* load_cell = load_port->libertyCell();
  Instance* drvr = sta->makeInstance("drvr", drvr_cell, top_inst);
  Instance* load = sta->makeInstance("load", load_cell, top_inst);
  sta->connectPin(drvr, drvr_port, net);
  sta->connectPin(load, load_port, net);
  Pin* drvr_pin = network->findPin(drvr, drvr_port);
  Pin* load_pin = network->findPin(load, load_port);

  // Max rise/fall delays.
  delay = -INF;
  slew = -INF;

  LoadPinIndexMap load_pin_index_map(network_);
  load_pin_index_map[load_pin] = 0;
  for (Corner* corner : *corners) {
    const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
    makeWireParasitic(net, drvr_pin, load_pin, wire_length, corner, parasitics);

    for (TimingArcSet* arc_set : drvr_cell->timingArcSets()) {
      if (arc_set->to() == drvr_port) {
        for (TimingArc* arc : arc_set->arcs()) {
          const RiseFall* in_rf = arc->fromEdge()->asRiseFall();
          const RiseFall* drvr_rf = arc->toEdge()->asRiseFall();
          double in_slew = tgt_slews_[in_rf->index()];
          Parasitic* drvr_parasitic
              = arc_delay_calc->findParasitic(drvr_pin, drvr_rf, dcalc_ap);
          float load_cap = parasitics_->capacitance(drvr_parasitic);
          ArcDcalcResult dcalc_result
              = arc_delay_calc->gateDelay(drvr_pin,
                                          arc,
                                          in_slew,
                                          load_cap,
                                          drvr_parasitic,
                                          load_pin_index_map,
                                          dcalc_ap);
          ArcDelay gate_delay = dcalc_result.gateDelay();
          // Only one load pin, so load_idx is 0.
          ArcDelay wire_delay = dcalc_result.wireDelay(0);
          ArcDelay load_slew = dcalc_result.loadSlew(0);
          delay = max(delay, gate_delay + wire_delay);
          slew = max(slew, load_slew);
        }
      }
    }
    arc_delay_calc->finishDrvrPin();
    parasitics->deleteParasitics(net, dcalc_ap->parasiticAnalysisPt());
  }

  // Cleanup the temporaries.
  sta->deleteInstance(drvr);
  sta->deleteInstance(load);
  sta->deleteNet(net);
}

void Resizer::makeWireParasitic(Net* net,
                                Pin* drvr_pin,
                                Pin* load_pin,
                                double wire_length,  // meters
                                const Corner* corner,
                                Parasitics* parasitics)
{
  const ParasiticAnalysisPt* parasitics_ap
      = corner->findParasiticAnalysisPt(max_);
  Parasitic* parasitic
      = parasitics->makeParasiticNetwork(net, false, parasitics_ap);
  ParasiticNode* n1
      = parasitics->ensureParasiticNode(parasitic, drvr_pin, network_);
  ParasiticNode* n2
      = parasitics->ensureParasiticNode(parasitic, load_pin, network_);
  double wire_cap = wire_length * wireSignalCapacitance(corner);
  double wire_res = wire_length * wireSignalResistance(corner);
  parasitics->incrCap(n1, wire_cap / 2.0);
  parasitics->makeResistor(parasitic, 1, wire_res, n1, n2);
  parasitics->incrCap(n2, wire_cap / 2.0);
}

////////////////////////////////////////////////////////////////

double Resizer::designArea()
{
  initBlock();
  initDesignArea();
  return design_area_;
}

void Resizer::designAreaIncr(float delta)
{
  design_area_ += delta;
}

double Resizer::computeDesignArea()
{
  double design_area = 0.0;
  for (dbInst* inst : block_->getInsts()) {
    dbMaster* master = inst->getMaster();
    // Don't count fillers otherwise you'll always get 100% utilization
    if (!master->isFiller()
        && master->getType() != odb::dbMasterType::CORE_WELLTAP
        && !master->isEndCap()) {
      design_area += area(master);
    }
  }
  return design_area;
}

void Resizer::initDesignArea()
{
  initBlock();
  design_area_ = computeDesignArea();
}

bool Resizer::isFuncOneZero(const Pin* drvr_pin)
{
  LibertyPort* port = network_->libertyPort(drvr_pin);
  if (port) {
    FuncExpr* func = port->function();
    return func
           && (func->op() == FuncExpr::op_zero
               || func->op() == FuncExpr::op_one);
  }
  return false;
}

////////////////////////////////////////////////////////////////

void Resizer::repairDesign(double max_wire_length,
                           double slew_margin,
                           double cap_margin,
                           double buffer_gain,
                           bool match_cell_footprint,
                           bool verbose)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing
      || parasitics_src_ == ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  repair_design_->repairDesign(
      max_wire_length, slew_margin, cap_margin, buffer_gain, verbose);
}

int Resizer::repairDesignBufferCount() const
{
  return repair_design_->insertedBufferCount();
}

void Resizer::repairNet(Net* net,
                        double max_wire_length,
                        double slew_margin,
                        double cap_margin)
{
  resizePreamble();
  repair_design_->repairNet(net, max_wire_length, slew_margin, cap_margin);
}

void Resizer::repairClkNets(double max_wire_length)
{
  resizePreamble();
  utl::SetAndRestore set_buffers(buffer_cells_, clk_buffers_);

  repair_design_->repairClkNets(max_wire_length);
}

////////////////////////////////////////////////////////////////

// Find inverters in the clock network and clone them next to the
// each register they drive.
void Resizer::repairClkInverters()
{
  initDesignArea();
  sta_->ensureLevelized();
  graph_ = sta_->graph();
  for (const Instance* inv : findClkInverters()) {
    if (!dontTouch(inv)) {
      cloneClkInverter(const_cast<Instance*>(inv));
    }
  }
}

InstanceSeq Resizer::findClkInverters()
{
  InstanceSeq clk_inverters;
  ClkArrivalSearchPred srch_pred(this);
  BfsFwdIterator bfs(BfsIndex::other, &srch_pred, this);
  for (Clock* clk : sdc_->clks()) {
    for (const Pin* pin : clk->leafPins()) {
      Vertex* vertex = graph_->pinDrvrVertex(pin);
      bfs.enqueue(vertex);
    }
  }
  while (bfs.hasNext()) {
    Vertex* vertex = bfs.next();
    const Pin* pin = vertex->pin();
    Instance* inst = network_->instance(pin);
    LibertyCell* lib_cell = network_->libertyCell(inst);
    if (vertex->isDriver(network_) && lib_cell && lib_cell->isInverter()) {
      clk_inverters.emplace_back(inst);
      debugPrint(logger_,
                 RSZ,
                 "repair_clk_inverters",
                 2,
                 "inverter {}",
                 network_->pathName(inst));
    }
    if (!vertex->isRegClk()) {
      bfs.enqueueAdjacentVertices(vertex);
    }
  }
  return clk_inverters;
}

void Resizer::cloneClkInverter(Instance* inv)
{
  LibertyCell* inv_cell = network_->libertyCell(inv);
  LibertyPort *in_port, *out_port;
  inv_cell->bufferPorts(in_port, out_port);
  Pin* in_pin = network_->findPin(inv, in_port);
  Pin* out_pin = network_->findPin(inv, out_port);
  Net* in_net = network_->net(in_pin);
  dbNet* in_net_db = db_network_->staToDb(in_net);
  Net* out_net = network_->isTopLevelPort(out_pin)
                     ? network_->net(network_->term(out_pin))
                     : network_->net(out_pin);
  if (out_net) {
    const char* inv_name = network_->name(inv);
    Instance* top_inst = network_->topInstance();
    NetConnectedPinIterator* load_iter = network_->pinIterator(out_net);
    while (load_iter->hasNext()) {
      const Pin* load_pin = load_iter->next();
      if (load_pin != out_pin) {
        string clone_name = makeUniqueInstName(inv_name, true);
        Point clone_loc = db_network_->location(load_pin);
        Instance* clone
            = makeInstance(inv_cell, clone_name.c_str(), top_inst, clone_loc);
        journalMakeBuffer(clone);

        Net* clone_out_net = makeUniqueNet();
        dbNet* clone_out_net_db = db_network_->staToDb(clone_out_net);
        clone_out_net_db->setSigType(in_net_db->getSigType());

        Instance* load = network_->instance(load_pin);
        sta_->connectPin(clone, in_port, in_net);
        sta_->connectPin(clone, out_port, clone_out_net);

        // Connect load to clone
        sta_->disconnectPin(const_cast<Pin*>(load_pin));
        Port* load_port = network_->port(load_pin);
        sta_->connectPin(load, load_port, clone_out_net);
      }
    }
    delete load_iter;

    bool has_term = false;
    NetTermIterator* term_iter = network_->termIterator(out_net);
    while (term_iter->hasNext()) {
      has_term = true;
      break;
    }
    delete term_iter;

    if (!has_term) {
      // Delete inv
      sta_->disconnectPin(in_pin);
      sta_->disconnectPin(out_pin);
      sta_->deleteNet(out_net);
      parasitics_invalid_.erase(out_net);
      sta_->deleteInstance(inv);
    }
  }
}

////////////////////////////////////////////////////////////////

bool Resizer::repairSetup(double setup_margin,
                          double repair_tns_end_percent,
                          int max_passes,
                          int max_repairs_per_pass,
                          bool match_cell_footprint,
                          bool verbose,
                          const std::vector<MoveType>& sequence,
                          bool skip_pin_swap,
                          bool skip_gate_cloning,
                          bool skip_size_down,
                          bool skip_buffering,
                          bool skip_buffer_removal,
                          bool skip_last_gasp)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing
      || parasitics_src_ == ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  return repair_setup_->repairSetup(setup_margin,
                                    repair_tns_end_percent,
                                    max_passes,
                                    max_repairs_per_pass,
                                    verbose,
                                    sequence,
                                    skip_pin_swap,
                                    skip_gate_cloning,
                                    skip_size_down,
                                    skip_buffering,
                                    skip_buffer_removal,
                                    skip_last_gasp);
}

void Resizer::reportSwappablePins()
{
  resizePreamble();
  swap_pins_move_->reportSwappablePins();
}

void Resizer::repairSetup(const Pin* end_pin)
{
  resizePreamble();
  repair_setup_->repairSetup(end_pin);
}

void Resizer::rebufferNet(const Pin* drvr_pin)
{
  resizePreamble();
  buffer_move_->rebufferNet(drvr_pin);
}

////////////////////////////////////////////////////////////////

bool Resizer::repairHold(
    double setup_margin,
    double hold_margin,
    bool allow_setup_violations,
    // Max buffer count as percent of design instance count.
    float max_buffer_percent,
    int max_passes,
    bool match_cell_footprint,
    bool verbose)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  // Some technologies such as nangate45 don't have delay cells. Hence,
  // until we have a better approach, it's better to consider clock buffers
  // for hold violation repairing as these buffers' delay may be slighty
  // higher and we'll need fewer insertions.
  // Obs: We need to clear the buffer list for the preamble to select
  // buffers again excluding the clock ones.
  utl::SetAndRestore set_exclude_clk_buffers(exclude_clock_buffers_, false);
  utl::SetAndRestore set_buffers(buffer_cells_, LibertyCellSeq());

  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing
      || parasitics_src_ == ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  return repair_hold_->repairHold(setup_margin,
                                  hold_margin,
                                  allow_setup_violations,
                                  max_buffer_percent,
                                  max_passes,
                                  verbose);
}

void Resizer::repairHold(const Pin* end_pin,
                         double setup_margin,
                         double hold_margin,
                         bool allow_setup_violations,
                         float max_buffer_percent,
                         int max_passes)
{
  // See comment on the method above.
  utl::SetAndRestore set_exclude_clk_buffers(exclude_clock_buffers_, false);
  utl::SetAndRestore set_buffers(buffer_cells_, LibertyCellSeq());

  resizePreamble();
  repair_hold_->repairHold(end_pin,
                           setup_margin,
                           hold_margin,
                           allow_setup_violations,
                           max_buffer_percent,
                           max_passes);
}

int Resizer::holdBufferCount() const
{
  return repair_hold_->holdBufferCount();
}

////////////////////////////////////////////////////////////////
bool Resizer::recoverPower(float recover_power_percent,
                           bool match_cell_footprint,
                           bool verbose)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing
      || parasitics_src_ == ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  return recover_power_->recoverPower(recover_power_percent, verbose);
}
////////////////////////////////////////////////////////////////
void Resizer::swapArithModules(int path_count,
                               const std::string& target,
                               float slack_margin)
{
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing
      || parasitics_src_ == ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  swap_arith_modules_->replaceArithModules(path_count, target, slack_margin);
}
////////////////////////////////////////////////////////////////
// Journal to roll back changes
void Resizer::journalBegin()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  odb::dbDatabase::beginEco(block_);

  buffer_move_->undoMoves();
  size_down_move_->undoMoves();
  size_up_move_->undoMoves();
  clone_move_->undoMoves();
  split_load_move_->undoMoves();
  swap_pins_move_->undoMoves();
  unbuffer_move_->undoMoves();
}

void Resizer::journalEnd()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal end");
  if (!odb::dbDatabase::ecoEmpty(block_)) {
    updateParasitics();
    sta_->findRequireds();
  }
  odb::dbDatabase::endEco(block_);

  int move_count_ = 0;
  move_count_ += size_up_move_->numPendingMoves();
  move_count_ += size_down_move_->numPendingMoves();
  move_count_ += buffer_move_->numPendingMoves();
  move_count_ += clone_move_->numPendingMoves();
  move_count_ += swap_pins_move_->numPendingMoves();
  move_count_ += unbuffer_move_->numPendingMoves();

  debugPrint(
      logger_,
      RSZ,
      "opt_moves",
      2,
      "COMMIT {} moves: up {} down {} buffer {} clone {} swap {} unbuf {}",
      move_count_,
      size_up_move_->numPendingMoves(),
      size_down_move_->numPendingMoves(),
      buffer_move_->numPendingMoves(),
      clone_move_->numPendingMoves(),
      swap_pins_move_->numPendingMoves(),
      unbuffer_move_->numPendingMoves());

  accepted_move_count_ += move_count_;

  buffer_move_->commitMoves();
  size_up_move_->commitMoves();
  size_down_move_->commitMoves();
  clone_move_->commitMoves();
  split_load_move_->commitMoves();
  swap_pins_move_->commitMoves();
  unbuffer_move_->commitMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             1,
             "TOTAL {} moves (acc {} rej {}):  up {} down {} buffer {} clone "
             "{} swap {} unbuf {}",
             accepted_move_count_ + rejected_move_count_,
             accepted_move_count_,
             rejected_move_count_,
             size_up_move_->numCommittedMoves(),
             size_down_move_->numCommittedMoves(),
             buffer_move_->numCommittedMoves(),
             clone_move_->numCommittedMoves(),
             swap_pins_move_->numCommittedMoves(),
             unbuffer_move_->numCommittedMoves());
}

void Resizer::journalMakeBuffer(Instance* buffer)
{
  debugPrint(logger_,
             RSZ,
             "journal",
             1,
             "journal make_buffer {}",
             network_->pathName(buffer));
}

// This restores previously saved checkpoint by
// using odb undoEco.  Parasitics on relevant nets
// are invalidated and parasitics are updated.
// STA findRequireds() is performed also.
void Resizer::journalRestore()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal restore starts >>>");
  init();

  if (odb::dbDatabase::ecoEmpty(block_)) {
    odb::dbDatabase::endEco(block_);
    debugPrint(logger_,
               RSZ,
               "journal",
               1,
               "journal restore ends due to empty ECO >>>");
    return;
  }

  // Odb callbacks invalidate parasitics
  odb::dbDatabase::endEco(block_);
  odb::dbDatabase::undoEco(block_);

  updateParasitics();
  sta_->findRequireds();

  // Update transform counts
  debugPrint(
      logger_,
      RSZ,
      "journal",
      1,
      "Undid {} sizing {} buffering {} cloning {} swaps {} buf removal",
      size_up_move_->numPendingMoves() + size_down_move_->numPendingMoves(),
      buffer_move_->numPendingMoves(),
      clone_move_->numPendingMoves(),
      swap_pins_move_->numPendingMoves(),
      unbuffer_move_->numPendingMoves());

  int move_count_ = 0;
  move_count_ += size_down_move_->numPendingMoves();
  move_count_ += size_up_move_->numPendingMoves();
  move_count_ += buffer_move_->numPendingMoves();
  move_count_ += clone_move_->numPendingMoves();
  move_count_ += swap_pins_move_->numPendingMoves();
  move_count_ += unbuffer_move_->numPendingMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             2,
             "UNDO {} moves: up {} down {} buffer {} clone {} swap {} unbuf {}",
             move_count_,
             size_up_move_->numPendingMoves(),
             size_down_move_->numPendingMoves(),
             buffer_move_->numPendingMoves(),
             clone_move_->numPendingMoves(),
             swap_pins_move_->numPendingMoves(),
             unbuffer_move_->numPendingMoves());

  rejected_move_count_ += move_count_;

  size_down_move_->undoMoves();
  size_up_move_->undoMoves();
  buffer_move_->undoMoves();
  clone_move_->undoMoves();
  split_load_move_->undoMoves();
  swap_pins_move_->undoMoves();
  unbuffer_move_->undoMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             1,
             "TOTAL {} moves (acc {} rej {}):  up {} down {} buffer {} clone "
             "{} swap {} unbuf {}",
             accepted_move_count_ + rejected_move_count_,
             accepted_move_count_,
             rejected_move_count_,
             size_up_move_->numCommittedMoves(),
             size_down_move_->numCommittedMoves(),
             buffer_move_->numCommittedMoves(),
             clone_move_->numCommittedMoves(),
             swap_pins_move_->numCommittedMoves(),
             unbuffer_move_->numCommittedMoves());

  debugPrint(logger_, RSZ, "journal", 1, "journal restore ends <<<");
}

////////////////////////////////////////////////////////////////
void Resizer::journalBeginTest()
{
  journalBegin();
}

void Resizer::journalRestoreTest()
{
  int resize_count_old = size_up_move_->numMoves();
  int inserted_buffer_count_old = buffer_move_->numMoves();
  int cloned_gate_count_old = clone_move_->numMoves();
  int swap_pin_count_old = swap_pins_move_->numMoves();
  int removed_buffer_count_old = unbuffer_move_->numMoves();

  journalRestore();

  logger_->report(
      "journalRestoreTest restored {} sizing, {} buffering, {} "
      "cloning, {} pin swaps, {} buffer removal",
      resize_count_old - size_up_move_->numMoves(),
      inserted_buffer_count_old - buffer_move_->numMoves(),
      cloned_gate_count_old - clone_move_->numMoves(),
      swap_pin_count_old - swap_pins_move_->numMoves(),
      removed_buffer_count_old - unbuffer_move_->numMoves());
}

void Resizer::getBufferPins(Instance* buffer, Pin*& ip, Pin*& op)
{
  ip = nullptr;
  op = nullptr;
  auto pin_iter
      = std::unique_ptr<InstancePinIterator>(network_->pinIterator(buffer));
  while (pin_iter->hasNext()) {
    Pin* pin = pin_iter->next();
    sta::PortDirection* dir = network_->direction(pin);
    if (dir->isAnyOutput()) {
      op = pin;
    }
    if (dir->isAnyInput()) {
      ip = pin;
    }
  }
}

////////////////////////////////////////////////////////////////
Instance* Resizer::makeBuffer(LibertyCell* cell,
                              const char* name,
                              Instance* parent,
                              const Point& loc)
{
  Instance* inst = makeInstance(cell, name, parent, loc);
  journalMakeBuffer(inst);
  return inst;
}

Instance* Resizer::makeInstance(LibertyCell* cell,
                                const char* name,
                                Instance* parent,
                                const Point& loc)
{
  debugPrint(logger_, RSZ, "make_instance", 1, "make instance {}", name);
  Instance* inst = db_network_->makeInstance(cell, name, parent);
  dbInst* db_inst = db_network_->staToDb(inst);
  db_inst->setSourceType(odb::dbSourceType::TIMING);
  setLocation(db_inst, loc);
  // Legalize the position of the instance in case it leaves the die
  if (parasitics_src_ == ParasiticsSrc::global_routing
      || parasitics_src_ == ParasiticsSrc::detailed_routing) {
    opendp_->legalCellPos(db_inst);
  }
  designAreaIncr(area(db_inst->getMaster()));
  return inst;
}

void Resizer::setLocation(dbInst* db_inst, const Point& pt)
{
  int x = pt.x();
  int y = pt.y();
  // Stay inside the lines.
  if (core_exists_) {
    dbMaster* master = db_inst->getMaster();
    int width = master->getWidth();
    if (x < core_.xMin()) {
      x = core_.xMin();
      buffer_moved_into_core_ = true;
    } else if (x > core_.xMax() - width) {
      // Make sure the instance is entirely inside core.
      x = core_.xMax() - width;
      buffer_moved_into_core_ = true;
    }

    int height = master->getHeight();
    if (y < core_.yMin()) {
      y = core_.yMin();
      buffer_moved_into_core_ = true;
    } else if (y > core_.yMax() - height) {
      y = core_.yMax() - height;
      buffer_moved_into_core_ = true;
    }
  }

  db_inst->setPlacementStatus(dbPlacementStatus::PLACED);
  db_inst->setLocation(x, y);
}

float Resizer::portCapacitance(LibertyPort* input, const Corner* corner) const
{
  const DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(max_);
  int lib_ap = dcalc_ap->libertyIndex();
  LibertyPort* corner_input = input->cornerPort(lib_ap);
  return corner_input->capacitance();
}

float Resizer::bufferSlew(LibertyCell* buffer_cell,
                          float load_cap,
                          const DcalcAnalysisPt* dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return max(slews[RiseFall::riseIndex()], slews[RiseFall::fallIndex()]);
}

float Resizer::maxInputSlew(const LibertyPort* input,
                            const Corner* corner) const
{
  float limit;
  bool exists;
  sta_->findSlewLimit(input, corner, MinMax::max(), limit, exists);
  if (!exists || limit == 0.0) {
    // Fixup for nangate45: This library doesn't specify any max transition on
    // input pins which indirectly causes issues for the resizer when
    // repairing driver pin transitions.
    //
    // To address, if there's no max tran on the port directly, use the
    // library default (the default only applies to output pins per the
    // Liberty spec, as a workaround we apply it to input pins too).
    input->libertyLibrary()->defaultMaxSlew(limit, exists);
    if (!exists) {
      limit = INF;
    }
  }
  return limit;
}

void Resizer::checkLoadSlews(const Pin* drvr_pin,
                             double slew_margin,
                             // Return values.
                             Slew& slew,
                             float& limit,
                             float& slack,
                             const Corner*& corner)
{
  slack = INF;
  limit = INF;
  PinConnectedPinIterator* pin_iter = network_->connectedPinIterator(drvr_pin);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin != drvr_pin) {
      const Corner* corner1;
      const RiseFall* tr1;
      Slew slew1;
      float limit1, slack1;
      sta_->checkSlew(
          pin, nullptr, max_, false, corner1, tr1, slew1, limit1, slack1);
      if (!corner1) {
        // Fixup for nangate45: see comment in maxInputSlew
        if (!corner1) {
          LibertyPort* port = network_->libertyPort(pin);
          if (port) {
            bool exists;
            port->libertyLibrary()->defaultMaxSlew(limit1, exists);
            if (exists) {
              slew1 = 0.0;
              corner1 = tgt_slew_corner_;
              for (const RiseFall* rf : RiseFall::range()) {
                const DcalcAnalysisPt* dcalc_ap
                    = corner1->findDcalcAnalysisPt(max_);
                const Vertex* vertex = graph_->pinLoadVertex(pin);
                Slew slew2 = sta_->graph()->slew(vertex, rf, dcalc_ap->index());
                if (slew2 > slew1) {
                  slew1 = slew2;
                }
              }
            }
          }
        }
      }
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

void Resizer::warnBufferMovedIntoCore()
{
  if (buffer_moved_into_core_) {
    logger_->warn(RSZ, 77, "some buffers were moved inside the core.");
  }
}

void Resizer::setDebugPin(const Pin* pin)
{
  debug_pin_ = pin;
}

void Resizer::setWorstSlackNetsPercent(float percent)
{
  worst_slack_nets_percent_ = percent;
}

void Resizer::annotateInputSlews(Instance* inst,
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

void Resizer::resetInputSlews()
{
  input_slew_map_.clear();
}

void Resizer::eliminateDeadLogic(bool clean_nets)
{
  std::vector<const Instance*> queue;
  std::set<const Instance*> kept_instances;

  auto keepInst = [&](const Instance* inst) {
    if (!kept_instances.count(inst)) {
      kept_instances.insert(inst);
      queue.push_back(inst);
    }
  };

  auto keepPinDriver = [&](Pin* pin) {
    PinSet* drivers = network_->drivers(pin);
    if (drivers) {
      for (const Pin* drvr_pin : *drivers) {
        if (auto inst = network_->instance(drvr_pin)) {
          keepInst(inst);
        }
      }
    }
  };

  auto top_inst = network_->topInstance();
  if (top_inst) {
    auto iter = network_->pinIterator(top_inst);
    while (iter->hasNext()) {
      Pin* pin = iter->next();
      Net* net = network_->net(network_->term(pin));
      PinSet* drivers = network_->drivers(net);
      if (drivers) {
        for (const Pin* drvr_pin : *drivers) {
          if (auto inst = network_->instance(drvr_pin)) {
            keepInst(inst);
          }
        }
      }
    }
    delete iter;
  }

  for (auto inst : network_->leafInstances()) {
    if (!isLogicStdCell(inst) || dontTouch(inst)) {
      keepInst(inst);
    } else {
      auto iter = network_->pinIterator(inst);
      while (iter->hasNext()) {
        Pin* pin = iter->next();
        Net* net = network_->net(pin);
        if (net && dontTouch(net)) {
          keepInst(inst);
        }
      }
      delete iter;
    }
  }

  while (!queue.empty()) {
    const Instance* inst = queue.back();
    queue.pop_back();
    auto iter = network_->pinIterator(inst);
    while (iter->hasNext()) {
      keepPinDriver(iter->next());
    }
    delete iter;
  }

  int remove_inst_count = 0, remove_net_count = 0;
  for (auto inst : network_->leafInstances()) {
    if (!kept_instances.count(inst)) {
      sta_->deleteInstance((Instance*) inst);
      remove_inst_count++;
    }
  }

  if (clean_nets) {
    auto nets = db_network_->block()->getNets();
    for (auto it = nets.begin(); it != nets.end();) {
      if (!dontTouch(db_network_->dbToSta(*it)) && !it->isSpecial()
          && it->getITerms().empty() && it->getBTerms().empty()) {
        it = dbNet::destroy(it);
        remove_net_count++;
      } else {
        it++;
      }
    }
  }

  logger_->report("Removed {} unused instances and {} unused nets.",
                  remove_inst_count,
                  remove_net_count);
}

void Resizer::postReadLiberty()
{
  copyDontUseFromLiberty();
  swappable_cells_cache_.clear();
}

void Resizer::copyDontUseFromLiberty()
{
  std::unique_ptr<LibertyLibraryIterator> itr(
      db_network_->libertyLibraryIterator());

  while (itr->hasNext()) {
    sta::LibertyLibrary* lib = itr->next();

    std::unique_ptr<ConcreteLibraryCellIterator> cells(lib->cellIterator());

    while (cells->hasNext()) {
      LibertyCell* lib_cell = cells->next()->libertyCell();
      if (lib_cell == nullptr) {
        continue;
      }

      if (lib_cell->dontUse()) {
        setDontUse(lib_cell, true);
      }
    }
  }
}

void Resizer::fullyRebuffer(Pin* user_pin)
{
  resizePreamble();
  rebuffer_->fullyRebuffer(user_pin);
}

void Resizer::setDebugGraphics(std::shared_ptr<ResizerObserver> graphics)
{
  repair_design_->setDebugGraphics(graphics);
  graphics_ = std::move(graphics);
}

MoveType Resizer::parseMove(const std::string& s)
{
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower == "buffer") {
    return rsz::MoveType::BUFFER;
  }
  if (lower == "unbuffer") {
    return rsz::MoveType::UNBUFFER;
  }
  if (lower == "swap") {
    return rsz::MoveType::SWAP;
  }
  if (lower == "size") {
    return rsz::MoveType::SIZE;
  }
  if (lower == "sizeup") {
    return rsz::MoveType::SIZEUP;
  }
  if (lower == "sizedown") {
    return rsz::MoveType::SIZEDOWN;
  }
  if (lower == "clone") {
    return rsz::MoveType::CLONE;
  }
  if (lower == "split") {
    return rsz::MoveType::SPLIT;
  }
  throw std::invalid_argument("Invalid move type: " + s);
}

std::vector<rsz::MoveType> Resizer::parseMoveSequence(
    const std::string& sequence)
{
  std::vector<rsz::MoveType> result;
  std::stringstream ss(sequence);
  std::string item;
  while (std::getline(ss, item, ',')) {
    result.push_back(parseMove(item));
  }
  return result;
}

IncrementalParasiticsGuard::IncrementalParasiticsGuard(Resizer* resizer)
    : resizer_(resizer), need_unregister_(false)
{
  // check to allow reentrancy
  if (!resizer_->incremental_parasitics_enabled_) {
    if (!resizer_->block_) {
      resizer_->logger_->error(
          RSZ, 101, "incremental parasitics require an initialized block");
    }

    if (!resizer_->parasitics_invalid_.empty()) {
      resizer_->logger_->error(RSZ, 104, "inconsistent parasitics state");
    }

    switch (resizer_->parasitics_src_) {
      case ParasiticsSrc::placement:
        break;
      case ParasiticsSrc::global_routing:
      case ParasiticsSrc::detailed_routing:
        // TODO: add IncrementalDRoute
        resizer_->incr_groute_
            = new IncrementalGRoute(resizer_->global_router_, resizer_->block_);
        // Don't print verbose messages for incremental routing
        resizer_->global_router_->setVerbose(false);
        break;
      case ParasiticsSrc::none:
        break;
    }

    resizer_->incremental_parasitics_enabled_ = true;
    resizer_->db_cbk_->addOwner(resizer_->block_);
    need_unregister_ = true;
  }
}

void IncrementalParasiticsGuard::update()
{
  resizer_->updateParasitics();
}

IncrementalParasiticsGuard::~IncrementalParasiticsGuard()
{
  if (need_unregister_) {
    resizer_->db_cbk_->removeOwner();
    resizer_->updateParasitics();

    switch (resizer_->parasitics_src_) {
      case ParasiticsSrc::placement:
        break;
      case ParasiticsSrc::global_routing:
      case ParasiticsSrc::detailed_routing:
        // TODO: add IncrementalDRoute
        delete resizer_->incr_groute_;
        resizer_->incr_groute_ = nullptr;
        break;
      case ParasiticsSrc::none:
        break;
    }

    resizer_->incremental_parasitics_enabled_ = false;
  }
}

}  // namespace rsz
