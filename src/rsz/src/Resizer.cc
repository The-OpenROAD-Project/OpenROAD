// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rsz/Resizer.hh"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BufferMove.hh"
#include "BufferedNet.hh"
#include "CloneMove.hh"
#include "ConcreteSwapArithModules.hh"
#include "PreChecks.hh"
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
#include "VTSwapMove.hh"
#include "boost/functional/hash.hpp"
#include "boost/multi_array.hpp"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Bfs.hh"
#include "sta/Clock.hh"
#include "sta/ConcreteLibrary.hh"
#include "sta/ContainerHelpers.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/LeakagePower.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/NetworkCmp.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchPred.hh"
#include "sta/StringSeq.hh"
#include "sta/StringUtil.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"
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

Resizer::Resizer(utl::Logger* logger,
                 odb::dbDatabase* db,
                 dbSta* sta,
                 stt::SteinerTreeBuilder* stt_builder,
                 grt::GlobalRouter* global_router,
                 dpl::Opendp* opendp,
                 est::EstimateParasitics* estimate_parasitics)
    : swap_arith_modules_(std::make_unique<ConcreteSwapArithModules>(this)),
      tgt_slews_{0.0, 0.0}
{
  opendp_ = opendp;
  logger_ = logger;
  db_ = db;
  block_ = nullptr;
  dbStaState::init(sta);
  stt_builder_ = stt_builder;
  global_router_ = global_router;
  estimate_parasitics_ = estimate_parasitics;
  db_network_ = sta->getDbNetwork();
  resized_multi_output_insts_ = sta::InstanceSet(db_network_);
  db_cbk_ = std::make_unique<OdbCallBack>(this, network_, db_network_);

  db_network_->addObserver(this);

  size_up_move_ = std::make_unique<SizeUpMove>(this);
  size_up_match_move_ = std::make_unique<SizeUpMatchMove>(this);
  size_down_move_ = std::make_unique<SizeDownMove>(this);
  buffer_move_ = std::make_unique<BufferMove>(this);
  clone_move_ = std::make_unique<CloneMove>(this);
  swap_pins_move_ = std::make_unique<SwapPinsMove>(this);
  vt_swap_speed_move_ = std::make_unique<VTSwapSpeedMove>(this);
  unbuffer_move_ = std::make_unique<UnbufferMove>(this);
  split_load_move_ = std::make_unique<SplitLoadMove>(this);

  recover_power_ = std::make_unique<RecoverPower>(this);
  repair_design_ = std::make_unique<RepairDesign>(this);
  repair_setup_ = std::make_unique<RepairSetup>(this);
  repair_hold_ = std::make_unique<RepairHold>(this);
  rebuffer_ = std::make_unique<Rebuffer>(this);
}

Resizer::~Resizer() = default;

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
  VertexLevelLess(const sta::Network* network);
  bool operator()(const sta::Vertex* vertex1, const sta::Vertex* vertex2) const;

 protected:
  const sta::Network* network_;
};

VertexLevelLess::VertexLevelLess(const sta::Network* network)
    : network_(network)
{
}

bool VertexLevelLess::operator()(const sta::Vertex* vertex1,
                                 const sta::Vertex* vertex2) const
{
  sta::Level level1 = vertex1->level();
  sta::Level level2 = vertex2->level();
  return (level1 < level2)
         || (level1 == level2
             // Break ties for stable results.
             && sta::stringLess(network_->pathName(vertex1->pin()),
                                network_->pathName(vertex2->pin())));
}

sta::VertexSeq Resizer::orderedLoadPinVertices()
{
  sta::VertexSeq loads;
  sta::VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    sta::PortDirection* dir = network_->direction(vertex->pin());
    bool top_level = network_->isTopLevelPort(vertex->pin());
    if (!top_level && dir->isAnyInput()) {
      loads.emplace_back(vertex);
    }
  }
  sort(loads, VertexLevelLess(network_));
  return loads;
}

////////////////////////////////////////////////////////////////
constexpr static double double_equal_tolerance
    = std::numeric_limits<double>::epsilon() * 10;
static bool rszFuzzyEqual(double v1, double v2)
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
  } else {
    if (disable_buffer_pruning_ != false) {
      swappable_cells_cache_.clear();
      buffer_cells_.clear();
    }
    disable_buffer_pruning_ = false;
  }
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
  // Unlike Resizer::bufferInputs(), init() call is not needed here.
  // init() call performs STA levelization, but removeBuffers() does not need
  // timing information. So initBlock(), a light version of init(), is
  // sufficient.
  initBlock();
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);

  if (insts.empty()) {
    // remove all the buffers
    for (dbInst* db_inst : block_->getInsts()) {
      sta::Instance* buffer = db_network_->dbToSta(db_inst);
      if (unbuffer_move_->removeBufferIfPossible(buffer,
                                                 /* honor dont touch */ true)) {
      }
    }
  } else {
    // remove only select buffers specified by user
    for (const sta::Instance* buffer : insts) {
      if (unbuffer_move_->removeBufferIfPossible(
              const_cast<sta::Instance*>(buffer),
              /* don't honor dont touch */ false)) {
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
  estimate_parasitics_->updateParasitics();
  level_drvr_vertices_valid_ = false;
  logger_->info(RSZ, 26, "Removed {} buffers.", unbuffer_move_->numMoves());
}

void Resizer::unbufferNet(sta::Net* net)
{
  sta::InstanceSeq insts;
  std::vector<sta::Net*> queue = {net};

  while (!queue.empty()) {
    sta::Net* net_ = queue.back();
    sta::NetConnectedPinIterator* pin_iter
        = network_->connectedPinIterator(net_);
    queue.pop_back();

    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      const sta::LibertyPort* port = network_->libertyPort(pin);
      if (port && port->libertyCell()->isBuffer()) {
        sta::LibertyPort *in, *out;
        port->libertyCell()->bufferPorts(in, out);

        if (port == in) {
          const sta::Instance* inst = network_->instance(pin);
          insts.emplace_back(inst);
          const sta::Pin* out_pin = network_->findPin(inst, out);
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
    sta::VertexIterator vertex_iter(graph_);
    while (vertex_iter.hasNext()) {
      sta::Vertex* vertex = vertex_iter.next();
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
      sta::Instance* sta_inst = db_network_->dbToSta(inst);
      sta::LibertyCell* cell = network_->libertyCell(sta_inst);
      dbMaster* master = db_network_->staToDb(cell);
      if (master == nullptr) {
        continue;
      }
      sta::LibertyCellSeq swappable_cells = getSwappableCells(cell);
      for (sta::LibertyCell* target_cell : swappable_cells) {
        dbMaster* target_master = db_network_->staToDb(target_cell);
        if (target_master == nullptr) {
          continue;
        }
        // Pick a cell that has the matching site, the same VT type
        // and equal or less drive resistance.  swappable_cells are
        // sorted in decreasing order of drive resistance.
        if (target_master->getSite() == site
            && cellVTType(target_master).vt_index == cellVTType(master).vt_index
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

    const odb::Point origin = inst->getOrigin();
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
static void populateBufferCapTestPoints(sta::LibertyCell* cell,
                                        sta::LibertyPort* in,
                                        sta::LibertyPort* out,
                                        std::vector<float>& points)
{
  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    const sta::TimingRole* role = arc_set->role();
    if (role == sta::TimingRole::combinational() && arc_set->from() == in
        && arc_set->to() == out) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        auto model = dynamic_cast<sta::GateTableModel*>(arc->model());
        if (model) {
          auto dm = model->delayModel();
          for (const sta::TableAxis* axis :
               {dm->axis1(), dm->axis2(), dm->axis3()}) {
            if (axis
                && axis->variable()
                       == sta::TableAxisVariable::
                           total_output_net_capacitance) {
              auto& values = axis->values();
              points.insert(points.end(), values.begin(), values.end());
            }
          }
        }
      }
    }
  }
}

bool Resizer::bufferSizeOutmatched(sta::LibertyCell* worse,
                                   sta::LibertyCell* better,
                                   const float max_drive_resist)
{
  sta::LibertyPort *win, *wout, *bin, *bout;
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

  utl::sort_and_unique(test_points);

  // Ignore cell timing above this C_load/C_in ratio as that is far off
  // from the target ratio we use in sizing (this cut-off helps prune the
  // list of candidate buffer sizes better)
  constexpr float cap_ratio_cutoff = 20.0f;

  for (auto p : test_points) {
    if ((wlimit == 0 || p <= wlimit)
        && p < std::min(bin->capacitance(), win->capacitance())
                   * cap_ratio_cutoff) {
      float bd = bufferDelay(better, p, tgt_slew_corner_, max_);
      float wd = bufferDelay(worse, p, tgt_slew_corner_, max_);
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
  sta::LibertyCellSeq sizes_by_inp_cap;
  sizes_by_inp_cap = buffer_cells_;
  sort(sizes_by_inp_cap,
       [](const sta::LibertyCell* buffer1, const sta::LibertyCell* buffer2) {
         sta::LibertyPort *port1, *port2, *scratch;
         buffer1->bufferPorts(port1, scratch);
         buffer2->bufferPorts(port2, scratch);
         return port1->capacitance() < port2->capacitance();
       });

  sta::LibertyCellSeq fast_buffers;
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

  // Sort fast buffers by capacitance and then by name.
  std::vector<sta::LibertyCell*> buffers{buffer_fast_sizes_.begin(),
                                         buffer_fast_sizes_.end()};
  std::ranges::sort(
      buffers, [=](const sta::LibertyCell* a, const sta::LibertyCell* b) {
        sta::LibertyPort* scratch;
        sta::LibertyPort* in_a;
        sta::LibertyPort* in_b;

        a->bufferPorts(in_a, scratch);
        b->bufferPorts(in_b, scratch);

        return std::make_pair(in_a->capacitance(), std::string_view(a->name()))
               < std::make_pair(in_b->capacitance(),
                                std::string_view(b->name()));
      });

  logger_->report("\nFast Buffer Report:");
  logger_->report("There are {} fast buffers", buffers.size());
  logger_->report("{:->80}", "");
  logger_->report(
      "Cell                                        Area  Input  Intrinsic "
      "Drive ");
  logger_->report(
      "                                                   Cap    Delay    Res");
  logger_->report("{:->80}", "");

  for (auto size : buffers) {
    sta::LibertyPort *in, *out;
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

  sta::LibertyCellSeq buffer_list;
  getBufferList(buffer_list);

  // Pick the right cell footprint to avoid delay cells
  std::string best_footprint;
  if (lib_data_->cells_by_footprint.size() > 1) {
    for (const auto& [footprint_type, count] : lib_data_->cells_by_footprint) {
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
  int num_vt = lib_data_->sorted_vt_categories.size();
  if (num_vt > 1) {
    const std::pair<VTCategory, VTLeakageStats> second_leakiest_vt
        = lib_data_->sorted_vt_categories[num_vt - 2];
    best_vt_index = second_leakiest_vt.first.vt_index;
    debugRDPrint2("findBuffers: Best VT index is {} [{}] among {} VTs",
                  best_vt_index,
                  second_leakiest_vt.first.vt_name,
                  num_vt);
  } else if (num_vt == 1) {
    const std::pair<VTCategory, VTLeakageStats> only_vt
        = lib_data_->sorted_vt_categories[0];
    best_vt_index = only_vt.first.vt_index;
    debugRDPrint2("findBuffers: Best VT index is {} among 1 VT", best_vt_index);
  } else {
    debugRDPrint2("findBuffers: Best VT index is {} among 0 VT", best_vt_index);
  }

  // There may be multiple cell sites like short, tall, short+tall, etc.
  // Pick two sites that are the most dominant.  Most likely, short and tall.
  std::vector<std::pair<odb::dbSite*, float>> site_list;
  site_list.reserve(lib_data_->cells_by_site.size());
  for (const auto& [site_type, count] : lib_data_->cells_by_site) {
    site_list.emplace_back(site_type, (float) count / buffer_list.size());
  }
  std::ranges::sort(site_list, [](const auto& a, const auto& b) {
    return a.second > b.second;
  });
  int num_best_sites = std::min(2, (int) lib_data_->cells_by_site.size());
  for (int i = 0; i < num_best_sites; i++) {
    debugRDPrint2("findBuffers: {} is a dominant site",
                  site_list[i].first->getName());
  }

  sta::LibertyCellSeq new_buffer_list;
  for (sta::LibertyCell* buffer : buffer_list) {
    const char* footprint = buffer->footprint();
    odb::dbMaster* master = db_network_->staToDb(buffer);
    if (master == nullptr) {
      continue;
    }
    auto vt_type = cellVTType(master);
    bool footprint_matches
        = best_footprint.empty() || (footprint && best_footprint == footprint);
    bool vt_matches = best_vt_index == -1 || vt_type.vt_index == best_vt_index;

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
          vt_type.vt_index,
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

  sta::LibertyCellSeq final_buffer_list;
  for (int bucket = 0; bucket < num_buckets; bucket++) {
    // Calculate bucket boundaries
    int start_idx = bucket * bucket_size + std::min(bucket, remainder);
    int end_idx = start_idx + bucket_size + (bucket < remainder ? 1 : 0);

    // Create a vector for this bucket with drive_res * input_cap values
    std::vector<std::pair<sta::LibertyCell*, float>> bucket_buffers;
    bucket_buffers.reserve(bucket_size);

    for (int i = start_idx; i < end_idx && i < new_buffer_list.size(); i++) {
      sta::LibertyCell* buffer = new_buffer_list[i];
      sta::LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      float metric = bufferDriveResistance(buffer) * input->capacitance();
      bucket_buffers.emplace_back(buffer, metric);
    }

    // Sort this bucket by drive_res * input_cap (ascending)
    std::ranges::sort(bucket_buffers, [](const auto& a, const auto& b) {
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
    sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();

    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();

      for (sta::LibertyCell* buffer : *lib->buffers()) {
        if (exclude_clock_buffers_) {
          BufferUse buffer_use = getBufferUse(buffer);

          if (buffer_use == BufferUse::CLOCK) {
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
           [this](const sta::LibertyCell* buffer1,
                  const sta::LibertyCell* buffer2) {
             return bufferDriveResistance(buffer1)
                    > bufferDriveResistance(buffer2);
           });

      buffer_lowest_drive_ = buffer_cells_[0];
    }
  }
}

sta::LibertyCell* Resizer::selectBufferCell(sta::LibertyCell* user_buffer_cell)
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

bool Resizer::isLinkCell(sta::LibertyCell* cell) const
{
  return network_->findLibertyCell(cell->name()) == cell;
}

////////////////////////////////////////////////////////////////

void Resizer::bufferInputs(sta::LibertyCell* buffer_cell, bool verbose)
{
  init();

  // Use buffer_cell. If it is null, find the buffer w/ lowest drive resistance.
  sta::LibertyCell* selected_buffer_cell = selectBufferCell(buffer_cell);
  if (verbose) {
    logger_->info(RSZ,
                  29,
                  "Start input port buffering with {}.",
                  selected_buffer_cell->name());
  }

  sta_->ensureClkNetwork(sta_->cmdMode());
  inserted_buffer_count_ = 0;
  buffer_moved_into_core_ = false;

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    std::unique_ptr<sta::InstancePinIterator> port_iter(
        network_->pinIterator(network_->topInstance()));
    while (port_iter->hasNext()) {
      sta::Pin* pin = port_iter->next();
      sta::Net* net = network_->net(network_->term(pin));

      if (network_->direction(pin)->isInput() && !dontTouch(net)
          && !sta_->isConstant(pin, sta_->cmdMode())
          && !sta_->isClock(pin, sta_->cmdMode())
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

bool Resizer::hasPins(sta::Net* net)
{
  sta::NetPinIterator* pin_iter = db_network_->pinIterator(net);
  bool has_pins = pin_iter->hasNext();
  delete pin_iter;
  return has_pins;
}

void Resizer::getPins(sta::Net* net, PinVector& pins) const
{
  auto pin_iter = network_->pinIterator(net);
  while (pin_iter->hasNext()) {
    pins.emplace_back(pin_iter->next());
  }
  delete pin_iter;
}

void Resizer::getPins(sta::Instance* inst, PinVector& pins) const
{
  auto pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    pins.emplace_back(pin_iter->next());
  }
  delete pin_iter;
}

/*
Make sure all the top pins are buffered
*/
sta::Instance* Resizer::bufferInput(const sta::Pin* top_pin,
                                    sta::LibertyCell* buffer_cell,
                                    bool verbose)
{
  odb::dbNet* top_pin_flat_net = db_network_->flatNet(top_pin);

  // Filter to see if we need to do anything..
  bool has_non_buffer = false;
  bool has_dont_touch = false;
  sta::NetConnectedPinIterator* pin_iter
      = network_->connectedPinIterator(db_network_->dbToSta(top_pin_flat_net));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
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

  sta::Instance* buffer = nullptr;
  sta::Net* target_net = db_network_->dbToSta(top_pin_flat_net);
  buffer = insertBufferAfterDriver(target_net, buffer_cell, nullptr, "input");

  return buffer;
}

void Resizer::bufferOutputs(sta::LibertyCell* buffer_cell, bool verbose)
{
  init();

  // Use buffer_cell. If it is null, find the buffer w/ lowest drive resistance.
  sta::LibertyCell* selected_buffer_cell = selectBufferCell(buffer_cell);
  if (verbose) {
    logger_->info(RSZ,
                  31,
                  "Start output port buffering with {}.",
                  selected_buffer_cell->name());
  }

  inserted_buffer_count_ = 0;
  buffer_moved_into_core_ = false;

  {
    est::IncrementalParasiticsGuard guard(estimate_parasitics_);
    std::unique_ptr<sta::InstancePinIterator> port_iter(
        network_->pinIterator(network_->topInstance()));
    while (port_iter->hasNext()) {
      sta::Pin* pin = port_iter->next();
      sta::Net* net = network_->net(network_->term(pin));
      if (network_->direction(pin)->isOutput() && net
          && !dontTouch(net)
          // Hands off special nets.
          && !db_network_->isSpecial(net)
          // DEF does not have tristate output types so we have look at the
          // drivers.
          && !hasTristateOrDontTouchDriver(net)
          && !sta_->isConstant(pin, sta_->cmdMode()) && hasPins(net)) {
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

bool Resizer::hasTristateOrDontTouchDriver(const sta::Net* net)
{
  sta::PinSet* drivers = network_->drivers(net);
  if (drivers) {
    for (const sta::Pin* pin : *drivers) {
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

bool Resizer::isTristateDriver(const sta::Pin* pin) const
{
  // Note LEF macro PINs do not have a clue about tristates.
  sta::LibertyPort* port = network_->libertyPort(pin);
  return port && port->direction()->isAnyTristate();
}

void Resizer::bufferOutput(const sta::Pin* top_pin,
                           sta::LibertyCell* buffer_cell,
                           bool verbose)
{
  if (verbose) {
    logger_->info(
        RSZ, 215, "Buffering output port {}.", network_->name(top_pin));
  }

  insertBufferBeforeLoad(
      const_cast<sta::Pin*>(top_pin), buffer_cell, nullptr, "output");
}

////////////////////////////////////////////////////////////////

float Resizer::driveResistance(const sta::Pin* drvr_pin)
{
  if (network_->isTopLevelPort(drvr_pin)) {
    sta::InputDrive* drive
        = sta_->cmdMode()->sdc()->findInputDrive(network_->port(drvr_pin));
    if (drive) {
      float max_res = 0;
      for (auto min_max : sta::MinMax::range()) {
        for (auto rf : sta::RiseFall::range()) {
          const sta::LibertyCell* cell;
          const sta::LibertyPort* from_port;
          float* from_slews;
          const sta::LibertyPort* to_port;
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
    sta::LibertyPort* drvr_port = network_->libertyPort(drvr_pin);
    if (drvr_port) {
      return drvr_port->driveResistance();
    }
  }
  return 0.0;
}

float Resizer::bufferDriveResistance(const sta::LibertyCell* buffer) const
{
  sta::LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  return output->driveResistance();
}

// This should be exported by STA
float Resizer::cellDriveResistance(const sta::LibertyCell* cell) const
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

sta::LibertyCell* Resizer::halfDrivingPowerCell(sta::Instance* inst)
{
  return halfDrivingPowerCell(network_->libertyCell(inst));
}
sta::LibertyCell* Resizer::halfDrivingPowerCell(sta::LibertyCell* cell)
{
  return closestDriver(cell, getSwappableCells(cell), 0.5);
}

bool Resizer::isSingleOutputCombinational(sta::Instance* inst) const
{
  dbInst* db_inst = db_network_->staToDb(inst);
  if (inst == network_->topInstance() || db_inst->isBlock()) {
    return false;
  }
  return isSingleOutputCombinational(network_->libertyCell(inst));
}

bool Resizer::isSingleOutputCombinational(sta::LibertyCell* cell) const
{
  if (!cell) {
    return false;
  }
  auto output_pins = libraryOutputPins(cell);
  return (output_pins.size() == 1 && isCombinational(cell));
}

bool Resizer::isCombinational(sta::LibertyCell* cell) const
{
  if (!cell) {
    return false;
  }
  return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro()
          && !cell->hasSequentials());
}

std::vector<sta::LibertyPort*> Resizer::libraryOutputPins(
    sta::LibertyCell* cell) const
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

std::vector<sta::LibertyPort*> Resizer::libraryPins(sta::Instance* inst) const
{
  return libraryPins(network_->libertyCell(inst));
}

std::vector<sta::LibertyPort*> Resizer::libraryPins(
    sta::LibertyCell* cell) const
{
  std::vector<sta::LibertyPort*> pins;
  sta::LibertyCellPortIterator itr(cell);
  while (itr.hasNext()) {
    auto port = itr.next();
    if (!port->isPwrGnd()) {
      pins.emplace_back(port);
    }
  }
  return pins;
}

sta::LibertyCell* Resizer::closestDriver(sta::LibertyCell* cell,
                                         const sta::LibertyCellSeq& candidates,
                                         float scale)
{
  sta::LibertyCell* closest = nullptr;
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

float Resizer::maxLoad(sta::Cell* cell)
{
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  auto min_max = sta::MinMax::max();
  sta::LibertyCellPortIterator itr(lib_cell);
  while (itr.hasNext()) {
    sta::LibertyPort* port = itr.next();
    if (port->direction()->isOutput()) {
      float limit, limit1;
      bool exists, exists1;
      const sta::Scene* corner = sta_->cmdScene();
      sta::Sdc* sdc = corner->sdc();
      // Default to top ("design") limit.
      sta::Cell* top_cell = network_->cell(network_->topInstance());
      sdc->capacitanceLimit(top_cell, min_max, limit, exists);
      sdc->capacitanceLimit(cell, min_max, limit1, exists1);

      if (exists1 && (!exists || min_max->compare(limit, limit1))) {
        limit = limit1;
        exists = true;
      }
      sta::LibertyPort* corner_port = port->scenePort(corner, min_max);
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

bool Resizer::hasFanout(sta::Vertex* drvr)
{
  sta::VertexOutEdgeIterator edge_iter(drvr, graph_);
  return edge_iter.hasNext();
}

bool Resizer::hasFanout(sta::Pin* drvr)
{
  if (network_->isDriver(drvr) == false) {
    return false;
  }

  sta::Vertex* vertex = graph_->pinDrvrVertex(drvr);
  return hasFanout(vertex);
}

static float targetLoadDist(float load_cap, float target_load)
{
  return abs(load_cap - target_load);
}

////////////////////////////////////////////////////////////////

void Resizer::resizeDrvrToTargetSlew(const sta::Pin* drvr_pin)
{
  resizePreamble();
  resizeToTargetSlew(drvr_pin);
}

void Resizer::resizePreamble()
{
  init();
  ensureLevelDrvrVertices();
  for (auto mode : sta_->modes()) {
    sta_->ensureClkNetwork(mode);
  }
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
std::optional<float> Resizer::cellLeakage(sta::LibertyCell* cell)
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
  const sta::LeakagePowerSeq& leakages = cell->leakagePowers();
  if (leakages.empty()) {
    cell_leakage_cache_[cell] = std::nullopt;
    return std::nullopt;
  }

  float total_leakage = 0.0;
  for (const sta::LeakagePower& leak : leakages) {
    total_leakage += leak.power();
  }
  leakage = total_leakage / leakages.size();
  cell_leakage_cache_[cell] = leakage;
  return leakage;
}

// For debugging
void Resizer::reportEquivalentCells(sta::LibertyCell* base_cell,
                                    bool match_cell_footprint,
                                    bool report_all_cells,
                                    bool report_vt_equiv)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  resizePreamble();
  sta::LibertyCellSeq equiv_cells;

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
  } else if (report_vt_equiv) {
    equiv_cells = getVTEquivCells(base_cell);
  } else {
    equiv_cells = getSwappableCells(base_cell);
  }

  // Identify cells that are excluded due to restrictions or cell_footprints
  std::unordered_set<sta::LibertyCell*> excluded_cells;
  if (report_all_cells) {
    // All SetAndRestore variables are out of scope, so original restrictions
    // are in play
    sta::LibertyCellSeq real_equiv_cells = getSwappableCells(base_cell);
    std::unordered_set<sta::LibertyCell*> real_equiv_cells_set(
        real_equiv_cells.begin(), real_equiv_cells.end());
    for (sta::LibertyCell* cell : equiv_cells) {
      if (real_equiv_cells_set.find(cell) == real_equiv_cells_set.end()) {
        excluded_cells.insert(cell);
      }
    }
  }

  // Sort equiv cells by ascending area and leakage
  // STA sorts them by drive resistance
  std::ranges::stable_sort(
      equiv_cells, [this](sta::LibertyCell* a, sta::LibertyCell* b) {
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
  double base_area = 0.0;
  if (master) {
    base_area = block_->dbuAreaToMicrons(master->getArea());
  }
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
    for (sta::LibertyCell* equiv_cell : equiv_cells) {
      std::string cell_name = equiv_cell->name();
      if (excluded_cells.find(equiv_cell) != excluded_cells.end()) {
        cell_name.insert(cell_name.begin(), '*');
      }
      odb::dbMaster* equiv_master = db_network_->staToDb(equiv_cell);
      if (equiv_master == nullptr) {
        continue;
      }
      double equiv_area = block_->dbuAreaToMicrons(equiv_master->getArea());
      double area_ratio = (base_area != 0.0) ? equiv_area / base_area : 0.0;
      std::optional<float> equiv_cell_leakage = cellLeakage(equiv_cell);
      if (equiv_cell_leakage) {
        logger_->report("{:<41} {:>7.3f} {:>5.2f} {:>8.2e} {:>5.2f}   {}",
                        cell_name,
                        equiv_area,
                        area_ratio,
                        *equiv_cell_leakage,
                        *equiv_cell_leakage / *base_leakage,
                        cellVTType(equiv_master).vt_name);
      } else {
        logger_->report("{:<41} {:>7.3f} {:>5.2f}   {}",
                        cell_name,
                        equiv_area,
                        area_ratio,
                        cellVTType(equiv_master).vt_name);
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
    for (sta::LibertyCell* equiv_cell : equiv_cells) {
      std::string cell_name = equiv_cell->name();
      if (excluded_cells.find(equiv_cell) != excluded_cells.end()) {
        cell_name.insert(cell_name.begin(), '*');
      }
      odb::dbMaster* equiv_master = db_network_->staToDb(equiv_cell);
      if (equiv_master == nullptr) {
        continue;
      }
      double equiv_area = block_->dbuAreaToMicrons(equiv_master->getArea());
      double area_ratio = (base_area != 0.0) ? equiv_area / base_area : 0.0;
      logger_->report("{:<41} {:>7.3f} {:>5.2f}   {}",
                      cell_name,
                      equiv_area,
                      area_ratio,
                      cellVTType(equiv_master).vt_name);
    }
    logger_->report(
        "--------------------------------------------------------------");
  }
}

void Resizer::reportBuffers(bool filtered)
{
  resizePreamble();

  sta::LibertyCellSeq buffer_list;

  getBufferList(buffer_list);
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
                  lib_data_->vt_leakage_by_category.size());
  logger_->report("VT type[index]: # buffers, ave leakage");
  for (const auto& [vt_category, vt_stats] : lib_data_->sorted_vt_categories) {
    logger_->report("  {:<6} [{}]: {}, {:>7.1e}",
                    vt_category.vt_name,
                    vt_category.vt_index,
                    vt_stats.cell_count,
                    vt_stats.get_average_leakage());
  }

  logger_->report("\nThere are {} cell footprint types:",
                  lib_data_->cells_by_footprint.size());
  for (const auto& [footprint_type, count] : lib_data_->cells_by_footprint) {
    logger_->report("  {:<6}: {} [{:.2f}%]",
                    footprint_type,
                    count,
                    ((float) count) / buffer_list.size() * 100);
  }

  logger_->report("\nThere are {} cell site types:",
                  lib_data_->cells_by_site.size());
  for (const auto& [site_type, count] : lib_data_->cells_by_site) {
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

  for (sta::LibertyCell* buffer : buffer_list) {
    float drive_res = bufferDriveResistance(buffer);
    sta::LibertyPort *input, *output;
    buffer->bufferPorts(input, output);
    float c_in = input->capacitance();
    std::optional<float> cell_leak = cellLeakage(buffer);
    odb::dbMaster* master = db_network_->staToDb(buffer);
    if (master == nullptr) {
      continue;
    }

    logger_->report("{:<41} {:>7.1f} {:>7.1e} {:>7.1e} {:>3} {:<7} {:<}",
                    buffer->name(),
                    drive_res,
                    drive_res * c_in,
                    cell_leak.value_or(0.0f),
                    master->getSite()->getHeight(),
                    (buffer->footprint() ? buffer->footprint() : "N/A"),
                    cellVTType(master).vt_name);
  }

  if (filtered) {
    findBuffers();
    logger_->report("\nFiltered Buffer Report:");
    if (disable_buffer_pruning_) {
      logger_->report(
          "All {} buffers are available because buffer pruning has been "
          "disabled",
          buffer_cells_.size());
    } else {
      logger_->report(
          "There are {} buffers after filtering based on threshold voltage,"
          "\ncell footprint, drive strength and cell site",
          buffer_cells_.size());
    }
    logger_->report("{:->80}", "");
    logger_->report(
        "Cell                                        Drive Drive    Leak "
        "  Site Cell   VT");
    logger_->report(
        "                                            Res   Res*Cin       "
        "   Ht Footpt Type");
    logger_->report("{:->80}", "");

    for (sta::LibertyCell* buffer : buffer_cells_) {
      float drive_res = bufferDriveResistance(buffer);
      sta::LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      float c_in = input->capacitance();
      std::optional<float> cell_leak = cellLeakage(buffer);
      odb::dbMaster* master = db_network_->staToDb(buffer);
      if (master == nullptr) {
        continue;
      }

      logger_->report("{:<41} {:>7.1f} {:>7.1e} {:>7.1e} {:>3} {:<7} {:<}",
                      buffer->name(),
                      drive_res,
                      drive_res * c_in,
                      cell_leak.value_or(0.0f),
                      master->getSite()->getHeight(),
                      (buffer->footprint() ? buffer->footprint() : "N/A"),
                      cellVTType(master).vt_name);
    }
  }

  sta::LibertyCell* hold_buffer = repair_hold_->reportHoldBuffer();
  logger_->report("\nHold Buffer Report:");
  logger_->report("{} is the buffer chosen for hold fixing",
                  (hold_buffer ? hold_buffer->name() : "-"));
  logger_->report("{:*>80}", "");
}

void Resizer::getBufferList(sta::LibertyCellSeq& buffer_list)
{
  if (!lib_data_) {
    lib_data_ = std::make_unique<LibraryAnalysisData>();
  } else {
    lib_data_->vt_leakage_by_category.clear();
    lib_data_->cells_by_footprint.clear();
    lib_data_->cells_by_site.clear();
    lib_data_->sorted_vt_categories.clear();
  }

  sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    for (sta::LibertyCell* buffer : *lib->buffers()) {
      if (exclude_clock_buffers_) {
        BufferUse buffer_use = getBufferUse(buffer);
        if (buffer_use == BufferUse::CLOCK) {
          continue;
        }
      }
      if (!dontUse(buffer) && !buffer->alwaysOn() && !buffer->isIsolationCell()
          && !buffer->isLevelShifter() && isLinkCell(buffer)) {
        odb::dbMaster* master = db_network_->staToDb(buffer);
        if (!master) {
          continue;
        }

        // Track site distribution
        lib_data_->cells_by_site[master->getSite()]++;

        // Track cell footprint distribution
        if (buffer->footprint()) {
          lib_data_->cells_by_footprint[buffer->footprint()]++;
        }

        // Track VT category leakage data
        VTCategory vt_category = cellVTType(master);

        // Get or create VT leakage stats for this category
        VTLeakageStats& vt_stats
            = lib_data_->vt_leakage_by_category[vt_category];
        vt_stats.add_cell_leakage(cellLeakage(buffer));

        buffer_list.emplace_back(buffer);
      }
    }
  }
  delete lib_iter;

  if (!buffer_list.empty()) {
    // Sort buffer list by VT type, then by drive resistance
    sort(buffer_list,
         [this](const sta::LibertyCell* buffer1,
                const sta::LibertyCell* buffer2) {
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
    lib_data_->sort_vt_categories();
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
sta::LibertyCellSeq Resizer::getSwappableCells(sta::LibertyCell* source_cell)
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

  sta::LibertyCellSeq swappable_cells;
  sta::LibertyCellSeq* equiv_cells = sta_->equivCells(source_cell);

  if (equiv_cells) {
    int64_t source_cell_area = master->getArea();
    std::optional<float> source_cell_leakage;
    if (sizing_leakage_limit_) {
      source_cell_leakage = cellLeakage(source_cell);
    }
    for (sta::LibertyCell* equiv_cell : *equiv_cells) {
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
        if (cellVTType(master).vt_index
            != cellVTType(equiv_cell_master).vt_index) {
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

size_t getCommonLength(const std::string& string1, const std::string& string2)
{
  size_t common_len = 0;
  size_t len_limit = std::min(string1.length(), string2.length());

  while (common_len < len_limit && string1[common_len] == string2[common_len]) {
    common_len++;
  }

  return common_len;
}

// Get VT swappable cells including the source cell itself
// Use cache to store equivalent cell list as follows:
// BUF_X1_RVT  : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
// BUF_X1_LVT  : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
// BUF_X1_SLVT : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
// If there are multiple cells in a VT category,
// keep only one cell that most closely matches the source cell name
sta::LibertyCellSeq Resizer::getVTEquivCells(sta::LibertyCell* source_cell)
{
  auto cache_it = vt_equiv_cells_cache_.find(source_cell);
  if (cache_it != vt_equiv_cells_cache_.end()) {
    return cache_it->second;
  }

  if (!lib_data_) {
    sta::LibertyCellSeq buffer_list;
    getBufferList(buffer_list);
    // We just need library data
    (void) buffer_list;
  }

  if (lib_data_->sorted_vt_categories.size() < 2) {
    vt_equiv_cells_cache_[source_cell] = sta::LibertyCellSeq();
    return vt_equiv_cells_cache_[source_cell];
  }

  sta::LibertyCellSeq* equiv_cells = sta_->equivCells(source_cell);
  dbMaster* source_cell_master = db_network_->staToDb(source_cell);
  if (equiv_cells == nullptr || source_cell_master == nullptr) {
    vt_equiv_cells_cache_[source_cell] = sta::LibertyCellSeq();
    return vt_equiv_cells_cache_[source_cell];
  }

  sta::LibertyCellSeq vt_equiv_cells;
  int64_t source_cell_area = source_cell_master->getArea();

  // VT equiv cell should have the same area, footprint, site and function class
  // but different VT type
  for (sta::LibertyCell* equiv_cell : *equiv_cells) {
    if (equiv_cell == source_cell) {
      vt_equiv_cells.emplace_back(equiv_cell);
      continue;
    }

    if (dontUse(equiv_cell) || !isLinkCell(equiv_cell)) {
      continue;
    }

    dbMaster* equiv_cell_master = db_network_->staToDb(equiv_cell);
    if (!equiv_cell_master) {
      continue;
    }

    if (cellVTType(equiv_cell_master) == cellVTType(source_cell_master)) {
      continue;
    }

    if (!sta::fuzzyEqual(equiv_cell_master->getArea(), source_cell_area)) {
      continue;
    }

    if (equiv_cell_master->getSite() != source_cell_master->getSite()) {
      continue;
    }

    if (!sta::stringEqIf(source_cell->footprint(), equiv_cell->footprint())) {
      continue;
    }

    if (source_cell->userFunctionClass()
        && !sta::stringEqIf(source_cell->userFunctionClass(),
                            equiv_cell->userFunctionClass())) {
      continue;
    }

    vt_equiv_cells.emplace_back(equiv_cell);
  }

  // Sort the list in ascending order of leakage
  std::ranges::stable_sort(
      vt_equiv_cells, [this](sta::LibertyCell* cell1, sta::LibertyCell* cell2) {
        std::optional<float> leak1 = this->cellLeakage(cell1);
        std::optional<float> leak2 = this->cellLeakage(cell2);
        // Treat missing leakage as 0
        return leak1.value_or(0.0) < leak2.value_or(0.0);
      });

  // Pick only one cell from each VT category
  const std::string source_name = source_cell->name();
  for (auto it = vt_equiv_cells.begin(); it != vt_equiv_cells.end();) {
    if (std::next(it) != vt_equiv_cells.end()) {
      sta::LibertyCell* curr_cell = *it;
      sta::LibertyCell* next_cell = *std::next(it);
      dbMaster* curr_master = db_network_->staToDb(curr_cell);
      dbMaster* next_master = db_network_->staToDb(next_cell);
      if (curr_master && next_master
          && (cellVTType(curr_master) == cellVTType(next_master))) {
        const std::string curr_name = curr_cell->name();
        const std::string next_name = next_cell->name();
        size_t curr_common_len = getCommonLength(curr_name, source_name);
        size_t next_common_len = getCommonLength(next_name, source_name);
        if (curr_common_len > next_common_len) {
          vt_equiv_cells.erase(std::next(it));
        } else {
          it = vt_equiv_cells.erase(it);
        }
      } else {
        ++it;
      }
    } else {
      ++it;
    }
  }

  // Map all equivalent cells to the same list
  // BUF_X1_RVT  : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
  // BUF_X1_LVT  : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
  // BUF_X1_SLVT : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
  vt_equiv_cells_cache_[source_cell] = std::move(vt_equiv_cells);
  const sta::LibertyCellSeq& vt_equiv_cells_new
      = vt_equiv_cells_cache_[source_cell];
  for (sta::LibertyCell* equiv_cell : vt_equiv_cells_new) {
    vt_equiv_cells_cache_[equiv_cell] = vt_equiv_cells_new;
  }
  return vt_equiv_cells_cache_[source_cell];
}

void Resizer::checkLibertyForAllCorners()
{
  for (sta::Scene* corner : sta_->scenes()) {
    int lib_ap_index = corner->libertyIndex(max_);
    sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      sta::LibertyCellIterator cell_iter(lib);
      while (cell_iter.hasNext()) {
        sta::LibertyCell* cell = cell_iter.next();
        if (isLinkCell(cell) && !dontUse(cell)) {
          sta::LibertyCell* corner_cell = cell->sceneCell(lib_ap_index);
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

void Resizer::makeEquivCells()
{
  if (!equiv_cells_made_) {
    sta::LibertyLibrarySeq libs;
    sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      // massive kludge until makeEquivCells is fixed to only incldue link cells
      sta::LibertyCellIterator cell_iter(lib);
      if (cell_iter.hasNext()) {
        sta::LibertyCell* cell = cell_iter.next();
        if (isLinkCell(cell)) {
          libs.emplace_back(lib);
        }
      }
    }
    delete lib_iter;
    sta_->makeEquivCells(&libs, nullptr);
    equiv_cells_made_ = true;
  }
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
VTCategory Resizer::cellVTType(dbMaster* master)
{
  // Check if VT type is already computed
  auto it = vt_map_.find(master);
  if (it != vt_map_.end()) {
    return it->second;
  }

  dbSet<dbBox> obs = master->getObstructions();
  if (obs.empty()) {
    VTCategory vt_cat{0, "-"};
    auto [new_it, _] = vt_map_.emplace(master, vt_cat);
    return new_it->second;
  }

  std::unordered_set<std::string> unique_layers;  // count each layer only once
  std::vector<std::string> layer_names;
  std::string new_layer_name;
  for (dbBox* bbox : obs) {
    odb::dbTechLayer* layer = bbox->getTechLayer();
    if (layer == nullptr || layer->getType() != odb::dbTechLayerType::IMPLANT) {
      continue;
    }

    std::string curr_layer_name = layer->getName();
    if (unique_layers.insert(curr_layer_name).second) {
      layer_names.emplace_back(curr_layer_name);
    }
  }

  // Sort to make hash order-independent
  std::ranges::sort(layer_names);

  // Now hash in sorted order
  size_t hash1 = 0;
  for (const auto& name : layer_names) {
    debugPrint(logger_,
               RSZ,
               "equiv",
               1,
               "{} has OBS implant layer {}",
               master->getName(),
               name);
    size_t hash2 = boost::hash<std::string>()(name);
    boost::hash_combine(hash1, hash2);
    new_layer_name = mergeVTLayerNames(new_layer_name, name);
  }

  if (hash1 == 0) {
    VTCategory vt_cat{0, "-"};
    auto [new_it, _] = vt_map_.emplace(master, vt_cat);
    return new_it->second;
  }

  if (vt_hash_map_.find(hash1) == vt_hash_map_.end()) {
    int vt_id = vt_hash_map_.size() + 1;
    vt_hash_map_[hash1] = vt_id;
  }

  compressVTLayerName(new_layer_name);
  VTCategory vt_cat{vt_hash_map_[hash1], std::move(new_layer_name)};
  const auto& [new_it, _] = vt_map_.emplace(master, std::move(vt_cat));
  debugPrint(logger_,
             RSZ,
             "equiv",
             1,
             "{} has VT type {} {}",
             master->getName(),
             vt_map_[master].vt_index,
             vt_map_[master].vt_name);
  return new_it->second;
}

int Resizer::resizeToTargetSlew(const sta::Pin* drvr_pin)
{
  sta::Instance* inst = network_->instance(drvr_pin);
  sta::LibertyCell* cell = network_->libertyCell(inst);
  if (!network_->isTopLevelPort(drvr_pin) && !dontTouch(inst) && cell
      && isLogicStdCell(inst)) {
    bool revisiting_inst = false;
    if (hasMultipleOutputs(inst)) {
      revisiting_inst = resized_multi_output_insts_.contains(inst);
      debugPrint(logger_,
                 RSZ,
                 "resize",
                 2,
                 "multiple outputs{}",
                 revisiting_inst ? " - revisit" : "");
      resized_multi_output_insts_.insert(inst);
    }
    estimate_parasitics_->ensureWireParasitic(drvr_pin);
    // Includes net parasitic capacitance.
    float load_cap
        = graph_delay_calc_->loadCap(drvr_pin, tgt_slew_corner_, max_);
    if (load_cap > 0.0) {
      sta::LibertyCell* target_cell
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

bool Resizer::getCin(const sta::LibertyCell* cell, float& cin)
{
  sta::LibertyCellPortIterator port_iter(cell);
  int nports = 0;
  cin = 0;
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
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

int Resizer::resizeToCapRatio(const sta::Pin* drvr_pin, bool upsize_only)
{
  sta::Instance* inst = network_->instance(drvr_pin);
  sta::LibertyCell* cell = inst ? network_->libertyCell(inst) : nullptr;
  if (!network_->isTopLevelPort(drvr_pin) && inst && !dontTouch(inst) && cell
      && isLogicStdCell(inst)) {
    float cin, load_cap;
    estimate_parasitics_->ensureWireParasitic(drvr_pin);

    // Includes net parasitic capacitance.
    load_cap = graph_delay_calc_->loadCap(drvr_pin, tgt_slew_corner_, max_);
    if (load_cap > 0.0 && getCin(cell, cin)) {
      float cap_ratio
          = cell->isBuffer() ? buffer_sizing_cap_ratio_ : sizing_cap_ratio_;

      sta::LibertyCellSeq equiv_cells = getSwappableCells(cell);
      sta::LibertyCell *best = nullptr, *highest_cin_cell = nullptr;
      float highest_cin = 0;
      for (sta::LibertyCell* size : equiv_cells) {
        float size_cin;
        if ((!cell->isBuffer() || buffer_fast_sizes_.contains(size))
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

bool Resizer::isLogicStdCell(const sta::Instance* inst)
{
  return !db_network_->isTopInstance(inst)
         && db_network_->staToDb(inst)->getMaster()->getType()
                == odb::dbMasterType::CORE;
}

sta::LibertyCell* Resizer::findTargetCell(sta::LibertyCell* cell,
                                          float load_cap,
                                          bool revisiting_inst)
{
  sta::LibertyCell* best_cell = cell;
  sta::LibertyCellSeq swappable_cells = getSwappableCells(cell);
  if (!swappable_cells.empty()) {
    bool is_buf_inv = cell->isBuffer() || cell->isInverter();
    float target_load = (*target_load_map_)[cell];
    float best_load = target_load;
    float best_dist = targetLoadDist(load_cap, target_load);
    float best_delay = is_buf_inv
                           ? bufferDelay(cell, load_cap, tgt_slew_corner_, max_)
                           : 0.0;
    debugPrint(logger_,
               RSZ,
               "resize",
               3,
               "{} load cap {} dist={:.2e} delay={}",
               cell->name(),
               units_->capacitanceUnit()->asString(load_cap),
               best_dist,
               delayAsString(best_delay, sta_, 3));
    for (sta::LibertyCell* target_cell : swappable_cells) {
      float target_load = (*target_load_map_)[target_cell];
      float delay = 0.0;
      if (is_buf_inv) {
        delay = bufferDelay(target_cell, load_cap, tgt_slew_corner_, max_);
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

// Replace LEF with LEF so ports stay aligned in instance.
bool Resizer::replaceCell(sta::Instance* inst,
                          const sta::LibertyCell* replacement,
                          const bool journal)
{
  const char* replacement_name = replacement->name();
  dbMaster* replacement_master = db_->findMaster(replacement_name);

  if (replacement_master) {
    dbInst* dinst = db_network_->staToDb(inst);
    dbMaster* master = dinst->getMaster();
    designAreaIncr(-area(master));
    sta::Cell* replacement_cell1 = db_network_->dbToSta(replacement_master);
    sta_->replaceCell(inst, replacement_cell1);
    designAreaIncr(area(replacement_master));

    // Legalize the position of the instance in case it leaves the die
    if (estimate_parasitics_->getParasiticsSrc()
            == est::ParasiticsSrc::global_routing
        || estimate_parasitics_->getParasiticsSrc()
               == est::ParasiticsSrc::detailed_routing) {
      opendp_->legalCellPos(db_network_->staToDb(inst));
    }
    return true;
  }
  return false;
}

bool Resizer::hasMultipleOutputs(const sta::Instance* inst)
{
  int output_count = 0;
  std::unique_ptr<sta::InstancePinIterator> pin_iter(
      network_->pinIterator(inst));
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
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
  initBlock();

  est::ParasiticsSrc parasitics_src = global_router_->haveRoutes()
                                          ? est::ParasiticsSrc::global_routing
                                          : est::ParasiticsSrc::placement;
  estimate_parasitics_->setParasiticsSrc(parasitics_src);
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  if (run_journal_restore) {
    journalBegin();
  }
  ensureLevelDrvrVertices();
  estimate_parasitics_->estimateParasitics(parasitics_src);
  int repaired_net_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;

  // Start incremental global routing if global routing parasitics are being
  // used.
  if (parasitics_src == est::ParasiticsSrc::global_routing) {
    global_router_->startIncremental();
  }

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

  // End incremental global routing if global routing parasitics were used.
  if (parasitics_src == est::ParasiticsSrc::global_routing) {
    global_router_->endIncremental();
  } else {
    // Fully rebuffer doesn't work with global routing parasitics.
    // TODO: fix the function to understand the parasitics from the global
    // routing.
    fullyRebuffer(nullptr);
  }
  ensureLevelDrvrVertices();

  findResizeSlacks1();
  if (run_journal_restore) {
    db_cbk_->addOwner(block_);
    journalRestore();
    db_cbk_->removeOwner();
    level_drvr_vertices_valid_ = false;
  }
}

void Resizer::findResizeSlacks1()
{
  // Use driver pin slacks rather than Sta::netSlack to save visiting
  // the net pins and min'ing the slack.
  net_slack_map_.clear();
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    sta::Vertex* drvr = level_drvr_vertices_[i];
    sta::Pin* drvr_pin = drvr->pin();
    sta::Net* net = db_network_->dbToSta(db_network_->flatNet(drvr_pin));
    if (net
        && !sta_->isConstant(drvr_pin, sta_->cmdMode())
        // Hands off special nets.
        && !db_network_->isSpecial(net)
        && !sta_->isClock(drvr_pin, sta_->cmdMode())) {
      net_slack_map_[net] = sta_->slack(drvr, max_);
    }
  }
}

sta::NetSeq Resizer::resizeWorstSlackNets()
{
  // Find the nets with the worst slack.
  sta::NetSeq nets;
  for (auto pair : net_slack_map_) {
    nets.push_back(pair.first);
  }
  // We are manually breaking ties to enforce stability, so use std::sort since
  // there is no need to pay the cost for stable sorting here
  std::ranges::sort(nets, [this](const sta::Net* net1, const sta::Net* net2) {
    auto slack1 = resizeNetSlack(net1);
    auto slack2 = resizeNetSlack(net2);
    if (slack1 != slack2) {
      return slack1 < slack2;
    }

    // In the event of a tie use ID to keep the sort stable.
    auto dbnet1 = db_network_->staToDb(net1);
    auto dbnet2 = db_network_->staToDb(net2);
    return dbnet1->getId() < dbnet2->getId();
  });

  int nworst_nets = std::ceil(nets.size() * worst_slack_nets_percent_ / 100.0);
  if (nets.size() > nworst_nets) {
    nets.resize(nworst_nets);
  }
  return nets;
}

std::optional<sta::Slack> Resizer::resizeNetSlack(const sta::Net* net)
{
  auto it = net_slack_map_.find(net);
  if (it == net_slack_map_.end()) {
    return {};
  }
  return it->second;
}

std::optional<sta::Slack> Resizer::resizeNetSlack(const odb::dbNet* db_net)
{
  const sta::Net* net = db_network_->dbToSta(db_net);
  return resizeNetSlack(net);
}

////////////////////////////////////////////////////////////////

// API for logic resynthesis
sta::PinSet Resizer::findFaninFanouts(sta::PinSet& end_pins)
{
  // Abbreviated copyState
  sta_->ensureLevelized();
  graph_ = sta_->graph();

  sta::VertexSet ends(graph_);
  for (const sta::Pin* pin : end_pins) {
    sta::Vertex* end = graph_->pinLoadVertex(pin);
    ends.insert(end);
  }
  sta::PinSet fanin_fanout_pins(db_network_);
  sta::VertexSet fanin_fanouts = findFaninFanouts(ends);
  for (sta::Vertex* vertex : fanin_fanouts) {
    fanin_fanout_pins.insert(vertex->pin());
  }
  return fanin_fanout_pins;
}

sta::VertexSet Resizer::findFaninFanouts(sta::VertexSet& ends)
{
  // Search backwards from ends to fanin register outputs and input ports.
  sta::VertexSet fanin_roots = findFaninRoots(ends);
  // Search forward from register outputs.
  sta::VertexSet fanouts = findFanouts(fanin_roots);
  return fanouts;
}

// STA's "search predicate" used to find combinational logic in the fanin
// or fanout cone of given vertices. It determines which edges/vertices should
// be traversed.
class SearchPredCombLogic : public sta::SearchPred1
{
 public:
  SearchPredCombLogic(sta::StaState* sta) : sta::SearchPred1(sta) {}

  bool searchThru(sta::Edge* edge, const sta::Mode* mode) const override
  {
    const sta::TimingRole* role = edge->role();
    return sta::SearchPred1::searchThru(edge, mode)
           && role->genericRole() != sta::TimingRole::regClkToQ()
           && role->genericRole() != sta::TimingRole::latchDtoQ();
  }
};

// Find source pins for logic fanin of ends.
sta::PinSet Resizer::findFanins(sta::PinSet& end_pins)
{
  // Abbreviated copyState
  sta_->ensureLevelized();
  graph_ = sta_->graph();

  sta::VertexSet ends(graph_);
  for (const sta::Pin* pin : end_pins) {
    sta::Vertex* end = graph_->pinLoadVertex(pin);
    ends.insert(end);
  }

  SearchPredCombLogic pred(sta_);
  sta::BfsBkwdIterator iter(sta::BfsIndex::other, &pred, this);
  for (sta::Vertex* vertex : ends) {
    iter.enqueueAdjacentVertices(vertex);
  }

  sta::PinSet fanins(db_network_);
  while (iter.hasNext()) {
    sta::Vertex* vertex = iter.next();
    if (isRegOutput(vertex) || network_->isTopLevelPort(vertex->pin())) {
      continue;
    }
    iter.enqueueAdjacentVertices(vertex);
    fanins.insert(vertex->pin());
  }
  return fanins;
}

// Find roots for logic fanin of ends.
sta::VertexSet Resizer::findFaninRoots(sta::VertexSet& ends)
{
  SearchPredCombLogic pred(sta_);
  sta::BfsBkwdIterator iter(sta::BfsIndex::other, &pred, this);
  for (sta::Vertex* vertex : ends) {
    iter.enqueueAdjacentVertices(vertex);
  }

  sta::VertexSet roots(graph_);
  while (iter.hasNext()) {
    sta::Vertex* vertex = iter.next();
    if (isRegOutput(vertex) || network_->isTopLevelPort(vertex->pin())) {
      roots.insert(vertex);
    } else {
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return roots;
}

bool Resizer::isRegOutput(sta::Vertex* vertex)
{
  sta::LibertyPort* port = network_->libertyPort(vertex->pin());
  if (port) {
    sta::LibertyCell* cell = port->libertyCell();
    for (sta::TimingArcSet* arc_set : cell->timingArcSets(nullptr, port)) {
      if (arc_set->role()->genericRole() == sta::TimingRole::regClkToQ()) {
        return true;
      }
    }
  }
  return false;
}

sta::VertexSet Resizer::findFanouts(sta::VertexSet& reg_outs)
{
  sta::VertexSet fanouts(graph_);
  SearchPredCombLogic pred(sta_);
  sta::BfsFwdIterator iter(sta::BfsIndex::other, &pred, this);
  for (sta::Vertex* reg_out : reg_outs) {
    iter.enqueueAdjacentVertices(reg_out);
  }

  while (iter.hasNext()) {
    sta::Vertex* vertex = iter.next();
    if (!isRegister(vertex)) {
      fanouts.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return fanouts;
}

bool Resizer::isRegister(sta::Vertex* vertex)
{
  sta::LibertyPort* port = network_->libertyPort(vertex->pin());
  if (port) {
    sta::LibertyCell* cell = port->libertyCell();
    return cell && cell->hasSequentials();
  }
  return false;
}

////////////////////////////////////////////////////////////////

double Resizer::area(sta::Cell* cell)
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
  return max_area_ && sta::fuzzyGreaterEqual(design_area_, max_area_);
}

void Resizer::setDontUse(sta::LibertyCell* cell, bool dont_use)
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

bool Resizer::dontUse(const sta::LibertyCell* cell)
{
  return dont_use_.contains(const_cast<sta::LibertyCell*>(cell));
}

void Resizer::reportDontUse() const
{
  logger_->report("Don't Use Cells:");

  if (dont_use_.empty()) {
    logger_->report("  none");
  } else {
    std::vector<sta::LibertyCell*> sorted_cells(dont_use_.begin(),
                                                dont_use_.end());
    std::ranges::sort(sorted_cells, utl::natural_compare, [](auto* cell) {
      return std::string_view(cell->name());
    });
    for (auto* cell : sorted_cells) {
      if (!isLinkCell(cell)) {
        continue;
      }
      logger_->report("  {}", cell->name());
    }
  }
}

void Resizer::setDontTouch(const sta::Instance* inst, bool dont_touch)
{
  dbInst* db_inst = db_network_->staToDb(inst);
  db_inst->setDoNotTouch(dont_touch);
}

bool Resizer::dontTouch(const sta::Instance* inst) const
{
  dbInst* db_inst = db_network_->staToDb(inst);
  if (!db_inst) {
    return false;
  }
  return db_inst->isDoNotTouch();
}

void Resizer::setDontTouch(const sta::Net* net, bool dont_touch)
{
  odb::dbNet* db_net = db_network_->staToDb(net);
  db_net->setDoNotTouch(dont_touch);
}

bool Resizer::dontTouch(const sta::Net* net) const
{
  odb::dbNet* db_net = nullptr;
  odb::dbModNet* db_mod_net = nullptr;
  db_network_->staToDb(net, db_net, db_mod_net);
  if (db_net == nullptr) {
    return false;
  }
  return db_net->isDoNotTouch();
}

bool Resizer::dontTouch(const sta::Pin* pin) const
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

    if (tgt_slew_corner_ == nullptr) {
      return;
    }

    target_load_map_ = std::make_unique<CellTargetLoadMap>();
    // Find target loads at the tgt_slew_corner.
    int lib_ap_index = tgt_slew_corner_->libertyIndex(max_);
    sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      sta::LibertyLibrary* lib = lib_iter->next();
      sta::LibertyCellIterator cell_iter(lib);
      while (cell_iter.hasNext()) {
        sta::LibertyCell* cell = cell_iter.next();
        if (isLinkCell(cell) && !dontUse(cell)) {
          sta::LibertyCell* corner_cell = cell->sceneCell(lib_ap_index);
          float tgt_load;
          auto it = target_load_map_->find(corner_cell);
          if (it == target_load_map_->end()) {
            tgt_load = findTargetLoad(corner_cell);
            (*target_load_map_)[corner_cell] = tgt_load;
          } else {
            tgt_load = it->second;
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

float Resizer::targetLoadCap(sta::LibertyCell* cell)
{
  float load_cap = 0.0;
  auto it = target_load_map_->find(cell);
  if (it != target_load_map_->end()) {
    load_cap = it->second;
  } else {
    logger_->error(RSZ, 68, "missing target load cap.");
  }
  return load_cap;
}

float Resizer::findTargetLoad(sta::LibertyCell* cell)
{
  float target_load_sum = 0.0;
  int arc_count = 0;
  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    const sta::TimingRole* role = arc_set->role();
    if (!role->isTimingCheck() && role != sta::TimingRole::tristateDisable()
        && role != sta::TimingRole::tristateEnable()
        && role != sta::TimingRole::clockTreePathMin()
        && role != sta::TimingRole::clockTreePathMax()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
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
float Resizer::findTargetLoad(sta::LibertyCell* cell,
                              sta::TimingArc* arc,
                              sta::Slew in_slew,
                              sta::Slew out_slew)
{
  sta::GateTimingModel* model
      = dynamic_cast<sta::GateTimingModel*>(arc->model());
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
sta::Slew Resizer::gateSlewDiff(sta::LibertyCell* cell,
                                sta::TimingArc* arc,
                                sta::GateTimingModel* model,
                                sta::Slew in_slew,
                                float load_cap,
                                sta::Slew out_slew)

{
  const sta::Pvt* pvt = sta_->cmdMode()->sdc()->operatingConditions(max_);
  sta::ArcDelay arc_delay;
  sta::Slew arc_slew;
  model->gateDelay(pvt, in_slew, load_cap, false, arc_delay, arc_slew);
  return arc_slew - out_slew;
}

////////////////////////////////////////////////////////////////

sta::Slew Resizer::targetSlew(const sta::RiseFall* rf)
{
  return tgt_slews_[rf->index()];
}

// Find target slew across all buffers in the libraries.
void Resizer::findBufferTargetSlews()
{
  tgt_slews_ = {0.0};
  tgt_slew_corner_ = nullptr;

  for (sta::Scene* corner : sta_->scenes()) {
    int lib_ap_index = corner->libertyIndex(max_);
    const sta::Pvt* pvt = sta_->cmdMode()->sdc()->operatingConditions(max_);
    // Average slews across buffers at corner.
    sta::Slew slews[sta::RiseFall::index_count]{0.0};
    int counts[sta::RiseFall::index_count]{0};
    for (sta::LibertyCell* buffer : buffer_cells_) {
      sta::LibertyCell* corner_buffer = buffer->sceneCell(lib_ap_index);
      findBufferTargetSlews(corner_buffer, pvt, slews, counts);
    }
    sta::Slew slew_rise = slews[sta::RiseFall::riseIndex()]
                          / counts[sta::RiseFall::riseIndex()];
    sta::Slew slew_fall = slews[sta::RiseFall::fallIndex()]
                          / counts[sta::RiseFall::fallIndex()];
    // Use the target slews from the slowest corner,
    // and resize using that corner.
    if (slew_rise > tgt_slews_[sta::RiseFall::riseIndex()]) {
      tgt_slews_[sta::RiseFall::riseIndex()] = slew_rise;
      tgt_slews_[sta::RiseFall::fallIndex()] = slew_fall;
      tgt_slew_corner_ = corner;
    }
  }

  debugPrint(logger_,
             RSZ,
             "target_load",
             1,
             "target slew corner {} = {}/{}",
             tgt_slew_corner_->name(),
             delayAsString(tgt_slews_[sta::RiseFall::riseIndex()], sta_, 3),
             delayAsString(tgt_slews_[sta::RiseFall::fallIndex()], sta_, 3));
}

void Resizer::findBufferTargetSlews(sta::LibertyCell* buffer,
                                    const sta::Pvt* pvt,
                                    // Return values.
                                    sta::Slew slews[],
                                    int counts[])
{
  sta::LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  for (sta::TimingArcSet* arc_set : buffer->timingArcSets(input, output)) {
    for (sta::TimingArc* arc : arc_set->arcs()) {
      sta::GateTimingModel* model
          = dynamic_cast<sta::GateTimingModel*>(arc->model());
      if (model != nullptr) {
        const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        const sta::RiseFall* out_rf = arc->toEdge()->asRiseFall();
        float in_cap = input->capacitance(in_rf, max_);
        float load_cap = in_cap * tgt_slew_load_cap_factor;
        sta::ArcDelay arc_delay;
        sta::Slew arc_slew;
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
void Resizer::repairTieFanout(sta::LibertyPort* tie_port,
                              double separation,  // meters
                              bool verbose)
{
  int tie_count = 0;
  int separation_dbu = metersToDbu(separation);
  sta::LibertyCell* tie_cell = tie_port->libertyCell();

  initDesignArea();

  sta::InstanceSeq insts;
  findCellInstances(tie_cell, insts);

  for (const sta::Instance* tie_inst : insts) {
    if (dontTouch(tie_inst)) {
      continue;
    }

    sta::Pin* drvr_pin = network_->findPin(tie_inst, tie_port);
    if (drvr_pin == nullptr) {
      continue;
    }

    odb::dbNet* drvr_db_net = db_network_->flatNet(drvr_pin);
    if (!drvr_db_net) {
      continue;
    }

    sta::Net* drvr_net = db_network_->dbToSta(drvr_db_net);
    if (drvr_net == nullptr || dontTouch(drvr_net)) {
      continue;
    }

    // Find load pins
    bool keep_tie = false;
    sta::PinSet load_pins_set(db_network_);
    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
        network_->connectedPinIterator(drvr_net));
    while (pin_iter->hasNext()) {
      const sta::Pin* load_pin = pin_iter->next();
      if (!(db_network_->isFlat(load_pin))) {
        continue;
      }

      sta::Instance* load_inst = network_->instance(load_pin);
      if (dontTouch(load_inst)) {
        keep_tie = true;  // This tie cell should not be deleted
        continue;
      }

      if (load_pin == drvr_pin) {
        continue;
      }

      // If load_pin is within ALU mode, set the corresponding hierarchical
      // boundary pin of the ALU module as a new load_pin because the new tie
      // cell cannot be inserted within the ALU module.
      load_pin = findArithBoundaryPin(load_pin);

      load_pins_set.insert(load_pin);
    }

    // Convert to vector and sort by pin path name for deterministic order
    std::vector<const sta::Pin*> load_pins(load_pins_set.begin(),
                                           load_pins_set.end());
    std::ranges::sort(load_pins, [this](const sta::Pin* a, const sta::Pin* b) {
      return strcmp(db_network_->pathName(a), db_network_->pathName(b)) < 0;
    });

    // Create new TIE cell instances for each load pin
    for (const sta::Pin* load_pin : load_pins) {
      sta::Instance* load_inst = network_->instance(load_pin);

      // Create a new tie cell instance
      const char* tie_inst_name = network_->name(load_inst);
      createNewTieCellForLoadPin(load_pin,
                                 tie_inst_name,
                                 db_network_->parent(load_inst),
                                 tie_port,
                                 separation_dbu);
      designAreaIncr(area(db_network_->cell(tie_cell)));
      tie_count++;
    }

    if (keep_tie == false) {
      // Delete the tie instance if no other ports are in use.
      // A tie cell can have both tie hi and low outputs.
      deleteTieCellAndNet(tie_inst, tie_port);
    }
  }  // For each tie_inst

  if (tie_count > 0) {
    logger_->info(
        RSZ, 42, "Inserted {} tie {} instances.", tie_count, tie_cell->name());
    level_drvr_vertices_valid_ = false;
  }
}

const sta::Pin* Resizer::findArithBoundaryPin(const sta::Pin* load_pin)
{
  sta::Instance* load_inst = network_->instance(load_pin);
  odb::dbModInst* arith_mod_inst = nullptr;
  swap_arith_modules_->isArithInstance(load_inst, arith_mod_inst);

  // load_inst is not in any arithmetic module
  if (arith_mod_inst == nullptr) {
    return load_pin;
  }

  // Find the hierarchical port of the module.
  // - It will be a new load_pin and a new tie cell will be inserted in
  //   front of the hierarchical port. It is because the new tie cell
  //   should not be inserted inside ALU module.
  odb::dbITerm* leaf_iterm = db_network_->flatPin(load_pin);
  odb::dbModITerm* parent_mod_iterm = db_network_->findInputModITermInParent(
      db_network_->dbToSta(leaf_iterm));
  while (parent_mod_iterm) {
    odb::dbModInst* parent_mod_inst = parent_mod_iterm->getParent();
    if (parent_mod_inst == arith_mod_inst) {
      // Found the pin on the boundary of the arithmetic module.
      // It is a new load_pin.
      return db_network_->dbToSta(parent_mod_iterm);
    }

    // Traverse up to the parent
    parent_mod_iterm = db_network_->findInputModITermInParent(
        db_network_->dbToSta(parent_mod_iterm));
  }

  // If traversal fails, return the original load pin
  return load_pin;
}

sta::Instance* Resizer::createNewTieCellForLoadPin(const sta::Pin* load_pin,
                                                   const char* new_inst_name,
                                                   sta::Instance* parent,
                                                   sta::LibertyPort* tie_port,
                                                   int separation_dbu)
{
  odb::dbITerm* load_iterm = nullptr;
  odb::dbBTerm* load_bterm = nullptr;
  odb::dbModITerm* load_mod_iterm = nullptr;
  db_network_->staToDb(load_pin, load_iterm, load_bterm, load_mod_iterm);

  uint32_t load_pin_id = 0;
  if (load_iterm != nullptr) {
    load_pin_id = load_iterm->getId();
  } else if (load_bterm != nullptr) {
    load_pin_id = load_bterm->getId();
  } else if (load_mod_iterm != nullptr) {
    load_pin_id = load_mod_iterm->getId();
  }

  sta::LibertyCell* tie_cell = tie_port->libertyCell();

  // Create the tie instance in the parent of the existing tie instance
  odb::Point new_tie_loc = tieLocation(load_pin, separation_dbu);
  sta::Instance* new_tie_inst
      = makeInstance(tie_cell,
                     new_inst_name,
                     parent,
                     new_tie_loc,
                     odb::dbNameUniquifyType::IF_NEEDED_WITH_UNDERSCORE);

  // If the load pin is not in the top module, move the new tie instance
  sta::Instance* load_inst = network_->instance(load_pin);
  if (network_->isTopInstance(load_inst) == false) {
    dbInst* db_inst = nullptr;
    odb::dbModInst* db_mod_inst = nullptr;
    odb::dbModule* module = nullptr;
    db_network_->staToDb(load_inst, db_inst, db_mod_inst);
    if (db_inst) {
      module = db_inst->getModule();
    } else if (db_mod_inst) {
      module = db_mod_inst->getParent();
    }

    if (module) {
      dbInst* new_tie_db_inst = db_network_->staToDb(new_tie_inst);
      module->addInst(new_tie_db_inst);
    }
  }

  sta::Pin* new_tie_out_pin = network_->findPin(new_tie_inst, tie_port);
  assert(new_tie_out_pin != nullptr);
  odb::dbITerm* new_tie_iterm = db_network_->flatPin(new_tie_out_pin);

  std::string connection_name = "net";

  debugPrint(logger_,
             RSZ,
             "repair_tie_fanout",
             1,
             "load_pin_id={} load_pin={} tie_inst={} tie_port={}/{}",
             load_pin_id,
             sdc_network_->pathName(load_pin),
             network_->name(new_tie_inst),
             tie_cell->name(),
             tie_port->name());

  // load_pin is a flat pin
  if (load_iterm) {
    // Connect the tie instance output pin to the load pin.
    debugPrint(logger_, RSZ, "repair_tie_fanout", 1, "  connect flat iterm");
    load_iterm->disconnect();  // This is required
    db_network_->hierarchicalConnect(
        new_tie_iterm, load_iterm, connection_name.c_str());
    return new_tie_inst;
  }

  // load_pin is a hier pin
  if (load_mod_iterm) {
    // Should not disconnect load_mod_iterm first. hierarchicalConnect() will
    // handle it. If load_mod_iterm is disconnected first,
    // hierarchicalConnect() cannot find the matching flat net with the
    // load_mod_iterm.
    debugPrint(
        logger_, RSZ, "repair_tie_fanout", 1, "  connect hier mod_iterm");
    db_network_->hierarchicalConnect(
        new_tie_iterm, load_mod_iterm, connection_name.c_str());
    return new_tie_inst;
  }

  // load_pin is an output port
  if (load_bterm) {
    // Create a new net and connect both the tie cell output and the top-level
    // port to it.
    debugPrint(logger_, RSZ, "repair_tie_fanout", 1, "  connect bterm");
    sta::Net* new_net = db_network_->makeNet(
        connection_name.c_str(), parent, odb::dbNameUniquifyType::IF_NEEDED);
    new_tie_iterm->connect(db_network_->staToDb(new_net));
    load_bterm->connect(db_network_->staToDb(new_net));
    return new_tie_inst;
  }

  logger_->error(
      RSZ, 216, "Should not reach here. createNewTieCellForLoadPin() failed.");
  return nullptr;
}

void Resizer::deleteTieCellAndNet(const sta::Instance* tie_inst,
                                  sta::LibertyPort* tie_port)
{
  // Get flat and hier nets.
  sta::Pin* tie_pin = network_->findPin(tie_inst, tie_port);
  odb::dbModNet* tie_hier_net;
  odb::dbNet* tie_flat_net;
  db_network_->net(tie_pin, tie_flat_net, tie_hier_net);

  // Delete hier net if it is dangling.
  if (tie_hier_net && tie_hier_net->connectionCount() <= 1) {
    odb::dbModNet::destroy(tie_hier_net);
  }

  // Delete inst output net.
  sta::Net* tie_net = db_network_->dbToSta(tie_flat_net);
  sta_->deleteNet(tie_net);
  estimate_parasitics_->removeNetFromParasiticsInvalid(tie_net);

  // Delete the tie instance if no other ports are in use.
  // A tie cell can have both tie hi and low outputs.
  bool has_other_fanout = false;
  sta::Pin* drvr_pin = network_->findPin(tie_inst, tie_port);
  std::unique_ptr<sta::InstancePinIterator> inst_pin_iter{
      network_->pinIterator(tie_inst)};
  while (inst_pin_iter->hasNext()) {
    sta::Pin* pin = inst_pin_iter->next();
    if (pin != drvr_pin) {
      // hier fix
      sta::Net* net = db_network_->dbToSta(db_network_->flatNet(pin));
      if (net && !network_->isPower(net) && !network_->isGround(net)) {
        has_other_fanout = true;
        break;
      }
    }
  }
  if (!has_other_fanout) {
    sta_->deleteInstance(const_cast<sta::Instance*>(tie_inst));
  }
}

void Resizer::findCellInstances(sta::LibertyCell* cell,
                                // Return value.
                                sta::InstanceSeq& insts)
{
  // TODO: iterating dbInsts in odb::dbBlock might be better. try it
  sta::LeafInstanceIterator* inst_iter = network_->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    sta::Instance* inst = inst_iter->next();
    if (network_->libertyCell(inst) == cell) {
      insts.emplace_back(inst);
    }
  }
  delete inst_iter;

  // Deterministic ordering of tie instances by full instance name
  // - This sort is to remove non-determinism.
  // - The sort will be removed when hierarhical flow is enabled by default.
  std::ranges::sort(
      insts, [this](const sta::Instance* a, const sta::Instance* b) {
        return strcmp(db_network_->pathName(a), db_network_->pathName(b)) < 0;
      });
}

// Place the tie instance on the side of the load pin.
odb::Point Resizer::tieLocation(const sta::Pin* load, int separation)
{
  odb::Point load_loc = db_network_->location(load);
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
  return odb::Point(tie_x, tie_y);
}

////////////////////////////////////////////////////////////////

void Resizer::reportLongWires(int count, int digits)
{
  initBlock();
  graph_ = sta_->ensureGraph();
  for (auto mode : sta_->modes()) {
    sta_->ensureClkNetwork(mode);
  }
  sta::VertexSeq drvrs;
  findLongWires(drvrs);
  logger_->report("Driver    length delay");
  const sta::Scene* corner = sta_->cmdScene();
  double wire_res = estimate_parasitics_->wireSignalResistance(corner);
  double wire_cap = estimate_parasitics_->wireSignalCapacitance(corner);
  int i = 0;
  for (sta::Vertex* drvr : drvrs) {
    sta::Pin* drvr_pin = drvr->pin();
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

using DrvrDist = std::pair<sta::Vertex*, int>;

void Resizer::findLongWires(sta::VertexSeq& drvrs)
{
  std::vector<DrvrDist> drvr_dists;
  sta::VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    sta::Vertex* vertex = vertex_iter.next();
    if (vertex->isDriver(network_)) {
      sta::Pin* pin = vertex->pin();
      // Hands off the clock nets.
      if (!sta_->isClock(pin, sta_->cmdMode())
          && !sta_->isConstant(pin, sta_->cmdMode())
          && !sta_->cmdMode()->sdc()->isDisabledConstraint(pin)) {
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
int Resizer::findMaxSteinerDist(sta::Vertex* drvr, const sta::Scene* corner)

{
  sta::Pin* drvr_pin = drvr->pin();
  BufferedNetPtr bnet = makeBufferedNetSteiner(drvr_pin, corner);
  if (bnet) {
    return bnet->maxLoadWireLength();
  }
  return 0;
}

double Resizer::maxLoadManhattenDistance(const sta::Net* net)
{
  sta::NetPinIterator* pin_iter = network_->pinIterator(net);
  int max_dist = 0;
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (network_->isDriver(pin)) {
      sta::Vertex* drvr = graph_->pinDrvrVertex(pin);
      if (drvr) {
        int dist = maxLoadManhattenDistance(drvr);
        max_dist = max(max_dist, dist);
      }
    }
  }
  delete pin_iter;
  return dbuToMeters(max_dist);
}

int Resizer::maxLoadManhattenDistance(sta::Vertex* drvr)
{
  int max_dist = 0;
  odb::Point drvr_loc = db_network_->location(drvr->pin());
  sta::VertexOutEdgeIterator edge_iter(drvr, graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::Vertex* load = edge->to(graph_);
    odb::Point load_loc = db_network_->location(load->pin());
    int dist = odb::Point::manhattanDistance(drvr_loc, load_loc);
    max_dist = max(max_dist, dist);
  }
  return max_dist;
}

////////////////////////////////////////////////////////////////

sta::NetSeq* Resizer::findFloatingNets()
{
  sta::NetSeq* floating_nets = new sta::NetSeq;
  sta::NetIterator* net_iter = network_->netIterator(network_->topInstance());
  while (net_iter->hasNext()) {
    sta::Net* net = net_iter->next();
    sta::PinSeq loads;
    sta::PinSeq drvrs;
    sta::PinSet visited_drvrs(db_network_);
    sta::FindNetDrvrLoads visitor(
        nullptr, visited_drvrs, loads, drvrs, network_);
    network_->visitConnectedPins(net, visitor);
    if (drvrs.empty() && !loads.empty()) {
      floating_nets->emplace_back(net);
    }
  }
  delete net_iter;
  sort(floating_nets, sta::NetPathNameLess(network_));
  return floating_nets;
}

sta::PinSet* Resizer::findFloatingPins()
{
  sta::PinSet* floating_pins = new sta::PinSet(network_);

  // Find instances with inputs without a net
  sta::LeafInstanceIterator* leaf_iter = network_->leafInstanceIterator();
  while (leaf_iter->hasNext()) {
    const sta::Instance* inst = leaf_iter->next();
    sta::InstancePinIterator* pin_iter = network_->pinIterator(inst);
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();
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

sta::NetSeq* Resizer::findOverdrivenNets(bool include_parallel_driven)
{
  sta::NetSeq* overdriven_nets = new sta::NetSeq;
  std::unique_ptr<sta::NetIterator> net_iter(
      network_->netIterator(network_->topInstance()));
  while (net_iter->hasNext()) {
    sta::Net* net = net_iter->next();
    sta::PinSeq loads;
    sta::PinSeq drvrs;
    sta::PinSet visited_drvrs(db_network_);
    sta::FindNetDrvrLoads visitor(
        nullptr, visited_drvrs, loads, drvrs, network_);
    network_->visitConnectedPins(net, visitor);
    if (drvrs.size() > 1) {
      bool all_tristate = true;
      for (const sta::Pin* drvr : drvrs) {
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

        for (const sta::Pin* drvr : drvrs) {
          const sta::Instance* inst = network_->instance(drvr);
          const sta::LibertyCell* cell = network_->libertyCell(inst);
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

          std::unique_ptr<sta::InstancePinIterator> inst_pin_iter(
              network_->pinIterator(inst));
          while (inst_pin_iter->hasNext()) {
            sta::Pin* inst_pin = inst_pin_iter->next();
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

float Resizer::portFanoutLoad(sta::LibertyPort* port) const
{
  float fanout_load;
  bool exists;
  port->fanoutLoad(fanout_load, exists);
  if (!exists) {
    sta::LibertyLibrary* lib = port->libertyLibrary();
    lib->defaultFanoutLoad(fanout_load, exists);
  }
  if (exists) {
    return fanout_load;
  }
  return 0.0;
}

float Resizer::bufferDelay(sta::LibertyCell* buffer_cell,
                           const sta::RiseFall* rf,
                           float load_cap,
                           const sta::Scene* scene,
                           const sta::MinMax* min_max)
{
  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  sta::ArcDelay gate_delays[sta::RiseFall::index_count];
  sta::Slew slews[sta::RiseFall::index_count];
  gateDelays(output, load_cap, scene, min_max, gate_delays, slews);
  return gate_delays[rf->index()];
}

float Resizer::bufferDelay(sta::LibertyCell* buffer_cell,
                           float load_cap,
                           const sta::Scene* scene,
                           const sta::MinMax* min_max)
{
  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  sta::ArcDelay gate_delays[sta::RiseFall::index_count];
  sta::Slew slews[sta::RiseFall::index_count];
  gateDelays(output, load_cap, scene, min_max, gate_delays, slews);
  return max(gate_delays[sta::RiseFall::riseIndex()],
             gate_delays[sta::RiseFall::fallIndex()]);
}

void Resizer::bufferDelays(sta::LibertyCell* buffer_cell,
                           float load_cap,
                           const sta::Scene* scene,
                           const sta::MinMax* min_max,
                           // Return values.
                           sta::ArcDelay delays[sta::RiseFall::index_count],
                           sta::Slew slews[sta::RiseFall::index_count])
{
  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  gateDelays(output, load_cap, scene, min_max, delays, slews);
}

// Rise/fall delays across all timing arcs into drvr_port.
// Uses target slew for input slew.
void Resizer::gateDelays(const sta::LibertyPort* drvr_port,
                         const float load_cap,
                         const sta::Scene* scene,
                         const sta::MinMax* min_max,
                         // Return values.
                         sta::ArcDelay delays[sta::RiseFall::index_count],
                         sta::Slew slews[sta::RiseFall::index_count])
{
  for (int rf_index : sta::RiseFall::rangeIndex()) {
    delays[rf_index] = -sta::INF;
    slews[rf_index] = -sta::INF;
  }
  sta::LibertyCell* cell = drvr_port->libertyCell();
  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        // use annotated slews if available
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
        const sta::Slew& drvr_slew = dcalc_result.drvrSlew();
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        slews[out_rf_index] = max(slews[out_rf_index], drvr_slew);
      }
    }
  }
}

// Rise/fall delays across all timing arcs into drvr_port.
// Takes input slews and load cap
void Resizer::gateDelays(const sta::LibertyPort* drvr_port,
                         const float load_cap,
                         const sta::Slew in_slews[sta::RiseFall::index_count],
                         const sta::Scene* scene,
                         const sta::MinMax* min_max,
                         // Return values.
                         sta::ArcDelay delays[sta::RiseFall::index_count],
                         sta::Slew out_slews[sta::RiseFall::index_count])
{
  for (int rf_index : sta::RiseFall::rangeIndex()) {
    delays[rf_index] = -sta::INF;
    out_slews[rf_index] = -sta::INF;
  }
  sta::LibertyCell* cell = drvr_port->libertyCell();
  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == drvr_port && !arc_set->role()->isTimingCheck()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        sta::LoadPinIndexMap load_pin_index_map(network_);
        sta::ArcDcalcResult dcalc_result
            = arc_delay_calc_->gateDelay(nullptr,
                                         arc,
                                         in_slews[in_rf->index()],
                                         load_cap,
                                         nullptr,
                                         load_pin_index_map,
                                         scene,
                                         min_max);

        const sta::ArcDelay& gate_delay = dcalc_result.gateDelay();
        const sta::Slew& drvr_slew = dcalc_result.drvrSlew();
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        out_slews[out_rf_index] = max(out_slews[out_rf_index], drvr_slew);
      }
    }
  }
}

sta::ArcDelay Resizer::gateDelay(const sta::LibertyPort* drvr_port,
                                 const sta::RiseFall* rf,
                                 const float load_cap,
                                 const sta::Scene* scene,
                                 const sta::MinMax* min_max)
{
  sta::ArcDelay delays[sta::RiseFall::index_count];
  sta::Slew slews[sta::RiseFall::index_count];
  gateDelays(drvr_port, load_cap, scene, min_max, delays, slews);
  return delays[rf->index()];
}

sta::ArcDelay Resizer::gateDelay(const sta::LibertyPort* drvr_port,
                                 const float load_cap,
                                 const sta::Scene* scene,
                                 const sta::MinMax* min_max)
{
  sta::ArcDelay delays[sta::RiseFall::index_count];
  sta::Slew slews[sta::RiseFall::index_count];
  gateDelays(drvr_port, load_cap, scene, min_max, delays, slews);
  return max(delays[sta::RiseFall::riseIndex()],
             delays[sta::RiseFall::fallIndex()]);
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
  for (const sta::Scene* corner : sta_->scenes()) {
    if (estimate_parasitics_->wireSignalResistance(corner) <= 0.0) {
      if (issue_error) {
        logger_->warn(RSZ,
                      88,
                      "Corner: {} has no wire signal resistance value.",
                      corner->name());
      }
      continue;
    }

    // buffer_cells_ is required to be non-empty.
    for (sta::LibertyCell* buffer_cell : buffer_cells_) {
      const double buffer_length = findMaxWireLength(buffer_cell, corner);
      max_length = min(max_length.value_or(sta::INF), buffer_length);
      debugPrint(logger_,
                 RSZ,
                 "max_wire_length",
                 1,
                 "Buffer {} has max_wire_length {}",
                 buffer_cell->name(),
                 units_->distanceUnit()->asString(buffer_length, 1));
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
double Resizer::findMaxWireLength(sta::LibertyCell* buffer_cell,
                                  const sta::Scene* corner)
{
  initBlock();
  sta::LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  return findMaxWireLength(drvr_port, corner);
}

double Resizer::findMaxWireLength(sta::LibertyPort* drvr_port,
                                  const sta::Scene* corner)
{
  sta::LibertyCell* cell = drvr_port->libertyCell();
  if (db_network_->staToDb(cell) == nullptr) {
    logger_->error(RSZ, 70, "no LEF cell for {}.", cell->name());
  }
  // Make a (hierarchical) block to use as a scratchpad.
  odb::dbBlock* block = odb::dbBlock::create(block_, "wire_delay", '/');
  std::unique_ptr<dbSta> sta = sta_->makeBlockSta(block);

  const double drvr_r = drvr_port->driveResistance();
  // wire_length_low - lower bound
  // wire_length_high - upper bound
  double wire_length_low = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  double wire_length_high
      = drvr_r / estimate_parasitics_->wireSignalResistance(corner);
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
  odb::dbBlock::destroy(block);
  return wire_length_low;
}

// objective function
double Resizer::splitWireDelayDiff(double wire_length,
                                   sta::LibertyCell* buffer_cell,
                                   std::unique_ptr<dbSta>& sta)
{
  sta::Delay delay1, delay2;
  sta::Slew slew1, slew2;
  bufferWireDelay(buffer_cell, wire_length, sta, delay1, slew1);
  bufferWireDelay(buffer_cell, wire_length / 2, sta, delay2, slew2);
  return delay1 - delay2 * 2;
}

// For tcl accessor.
void Resizer::bufferWireDelay(sta::LibertyCell* buffer_cell,
                              double wire_length,  // meters
                              // Return values.
                              sta::Delay& delay,
                              sta::Slew& slew)
{
  // Make a (hierarchical) block to use as a scratchpad.
  odb::dbBlock* block = odb::dbBlock::create(block_, "wire_delay", '/');
  std::unique_ptr<dbSta> sta = sta_->makeBlockSta(block);
  bufferWireDelay(buffer_cell, wire_length, sta, delay, slew);
  odb::dbBlock::destroy(block);
}

void Resizer::bufferWireDelay(sta::LibertyCell* buffer_cell,
                              double wire_length,  // meters
                              std::unique_ptr<dbSta>& sta,
                              // Return values.
                              sta::Delay& delay,
                              sta::Slew& slew)
{
  sta::LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  cellWireDelay(drvr_port, load_port, wire_length, sta, delay, slew);
}

// Cell delay plus wire delay.
// Use target slew for input slew.
// drvr_port and load_port do not have to be the same liberty cell.
void Resizer::cellWireDelay(sta::LibertyPort* drvr_port,
                            sta::LibertyPort* load_port,
                            double wire_length,  // meters
                            std::unique_ptr<dbSta>& sta,
                            // Return values.
                            sta::Delay& delay,
                            sta::Slew& slew)
{
  sta::Network* network = sta->network();
  sta::ArcDelayCalc* arc_delay_calc = sta->arcDelayCalc();

  // Create the same scenes in the same order, this will make liberty indices
  // line up and allow sharing the library between the two dbSta instances
  sta::StringSeq scene_names;
  for (auto scene : sta_->scenes()) {
    scene_names.push_back(scene->name().c_str());
  }
  sta->makeScenes(&scene_names);

  sta::Instance* top_inst = network->topInstance();
  // Tmp net for parasitics to live on.
  sta::Net* net = sta->makeNet("wire", top_inst);
  sta::LibertyCell* drvr_cell = drvr_port->libertyCell();
  sta::LibertyCell* load_cell = load_port->libertyCell();
  sta::Instance* drvr = sta->makeInstance("drvr", drvr_cell, top_inst);
  sta::Instance* load = sta->makeInstance("load", load_cell, top_inst);
  sta->connectPin(drvr, drvr_port, net);
  sta->connectPin(load, load_port, net);
  sta::Pin* drvr_pin = network->findPin(drvr, drvr_port);
  sta::Pin* load_pin = network->findPin(load, load_port);

  // Max rise/fall delays.
  delay = -sta::INF;
  slew = -sta::INF;

  sta::LoadPinIndexMap load_pin_index_map(network_);
  load_pin_index_map[load_pin] = 0;
  for (sta::Scene* corner : sta->scenes()) {
    estimate_parasitics_->makeWireParasitic(
        net, drvr_pin, load_pin, wire_length, corner);

    for (sta::TimingArcSet* arc_set : drvr_cell->timingArcSets()) {
      if (arc_set->to() == drvr_port) {
        for (sta::TimingArc* arc : arc_set->arcs()) {
          const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
          const sta::RiseFall* drvr_rf = arc->toEdge()->asRiseFall();
          double in_slew = tgt_slews_[in_rf->index()];
          sta::Parasitic* drvr_parasitic
              = arc_delay_calc->findParasitic(drvr_pin, drvr_rf, corner, max_);
          float load_cap
              = corner->parasitics(max_)->capacitance(drvr_parasitic);
          sta::ArcDcalcResult dcalc_result
              = arc_delay_calc->gateDelay(drvr_pin,
                                          arc,
                                          in_slew,
                                          load_cap,
                                          drvr_parasitic,
                                          load_pin_index_map,
                                          corner,
                                          max_);
          sta::ArcDelay gate_delay = dcalc_result.gateDelay();
          // Only one load pin, so load_idx is 0.
          sta::ArcDelay wire_delay = dcalc_result.wireDelay(0);
          sta::ArcDelay load_slew = dcalc_result.loadSlew(0);
          delay = max(delay, gate_delay + wire_delay);
          slew = max(slew, load_slew);
        }
      }
    }
    arc_delay_calc->finishDrvrPin();
    corner->parasitics(max_)->deleteParasitics(net);
  }

  // Cleanup the temporaries.
  sta->deleteInstance(drvr);
  sta->deleteInstance(load);
  sta->deleteNet(net);
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
    if (!master->isFiller()) {
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

bool Resizer::isFuncOneZero(const sta::Pin* drvr_pin)
{
  sta::LibertyPort* port = network_->libertyPort(drvr_pin);
  if (port) {
    sta::FuncExpr* func = port->function();
    return func
           && (func->op() == sta::FuncExpr::Op::zero
               || func->op() == sta::FuncExpr::Op::one);
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
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  repair_design_->repairDesign(
      max_wire_length, slew_margin, cap_margin, buffer_gain, verbose);
}

int Resizer::repairDesignBufferCount() const
{
  return repair_design_->insertedBufferCount();
}

void Resizer::repairNet(sta::Net* net,
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

  if (clk_buffers_.empty()) {
    std::vector<std::string> inferred_buffers;
    inferClockBufferList(nullptr, inferred_buffers);  // update clk_buffers_
  }

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
  for (const sta::Instance* inv : findClkInverters()) {
    if (!dontTouch(inv)) {
      cloneClkInverter(const_cast<sta::Instance*>(inv));
    }
  }
}

class ClkArrivalSearchPred : public sta::EvalPred
{
 public:
  ClkArrivalSearchPred(const sta::StaState* sta) : EvalPred(sta) {}
  bool searchThru(sta::Edge* edge, const sta::Mode* mode) const override
  {
    const sta::TimingRole* role = edge->role();
    return (role->isWire() || role == sta::TimingRole::combinational())
           && EvalPred::searchThru(edge, mode);
  }
};

sta::InstanceSeq Resizer::findClkInverters()
{
  sta::InstanceSeq clk_inverters;
  ClkArrivalSearchPred srch_pred(this);
  sta::BfsFwdIterator bfs(sta::BfsIndex::other, &srch_pred, this);
  for (sta::Clock* clk : sta_->cmdMode()->sdc()->clocks()) {
    for (const sta::Pin* pin : clk->leafPins()) {
      sta::Vertex* vertex = graph_->pinDrvrVertex(pin);
      bfs.enqueue(vertex);
    }
  }
  while (bfs.hasNext()) {
    sta::Vertex* vertex = bfs.next();
    const sta::Pin* pin = vertex->pin();
    sta::Instance* inst = network_->instance(pin);
    sta::LibertyCell* lib_cell = network_->libertyCell(inst);
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

void Resizer::cloneClkInverter(sta::Instance* inv)
{
  sta::LibertyCell* inv_cell = network_->libertyCell(inv);
  sta::LibertyPort *in_port, *out_port;
  inv_cell->bufferPorts(in_port, out_port);
  sta::Pin* in_pin = network_->findPin(inv, in_port);
  sta::Pin* out_pin = network_->findPin(inv, out_port);
  sta::Net* in_net = db_network_->findFlatNet(in_pin);
  odb::dbNet* in_net_db = db_network_->findFlatDbNet(in_net);
  sta::Net* out_net = network_->isTopLevelPort(out_pin)
                          ? network_->net(network_->term(out_pin))
                          : network_->net(out_pin);
  if (out_net) {
    const char* inv_name = network_->name(inv);
    sta::Instance* top_inst = network_->topInstance();
    sta::NetConnectedPinIterator* load_iter = network_->pinIterator(out_net);
    while (load_iter->hasNext()) {
      const sta::Pin* load_pin = load_iter->next();
      if (load_pin != out_pin) {
        odb::Point clone_loc = db_network_->location(load_pin);
        sta::Instance* clone
            = makeInstance(inv_cell,
                           inv_name,
                           top_inst,
                           clone_loc,
                           odb::dbNameUniquifyType::ALWAYS_WITH_UNDERSCORE);
        journalMakeBuffer(clone);

        sta::Net* clone_out_net = db_network_->makeNet(top_inst);
        odb::dbNet* clone_out_net_db = db_network_->staToDb(clone_out_net);
        clone_out_net_db->setSigType(in_net_db->getSigType());

        sta::Instance* load = network_->instance(load_pin);
        sta_->connectPin(clone, in_port, in_net);
        sta_->connectPin(clone, out_port, clone_out_net);

        // Connect load to clone
        sta_->disconnectPin(const_cast<sta::Pin*>(load_pin));
        sta::Port* load_port = network_->port(load_pin);
        sta_->connectPin(load, load_port, clone_out_net);
      }
    }
    delete load_iter;

    bool has_term = false;
    sta::NetTermIterator* term_iter = network_->termIterator(out_net);
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
      estimate_parasitics_->removeNetFromParasiticsInvalid(out_net);
      sta_->deleteInstance(inv);
    }
  }
}

////////////////////////////////////////////////////////////////

bool Resizer::repairSetup(double setup_margin,
                          double repair_tns_end_percent,
                          int max_passes,
                          int max_iterations,
                          int max_repairs_per_pass,
                          bool match_cell_footprint,
                          bool verbose,
                          const std::vector<MoveType>& sequence,
                          bool skip_pin_swap,
                          bool skip_gate_cloning,
                          bool skip_size_down,
                          bool skip_buffering,
                          bool skip_buffer_removal,
                          bool skip_last_gasp,
                          bool skip_vt_swap,
                          bool skip_crit_vt_swap)
{
  utl::SetAndRestore set_match_footprint(match_cell_footprint_,
                                         match_cell_footprint);
  resizePreamble();
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  return repair_setup_->repairSetup(setup_margin,
                                    repair_tns_end_percent,
                                    max_passes,
                                    max_iterations,
                                    max_repairs_per_pass,
                                    verbose,
                                    sequence,
                                    skip_pin_swap,
                                    skip_gate_cloning,
                                    skip_size_down,
                                    skip_buffering,
                                    skip_buffer_removal,
                                    skip_last_gasp,
                                    skip_vt_swap,
                                    skip_crit_vt_swap);
}

void Resizer::reportSwappablePins()
{
  resizePreamble();
  swap_pins_move_->reportSwappablePins();
}

void Resizer::repairSetup(const sta::Pin* end_pin)
{
  resizePreamble();
  repair_setup_->repairSetup(end_pin);
}

void Resizer::rebufferNet(const sta::Pin* drvr_pin)
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
    int max_iterations,
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
  utl::SetAndRestore set_buffers(buffer_cells_, sta::LibertyCellSeq());

  resizePreamble();
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  return repair_hold_->repairHold(setup_margin,
                                  hold_margin,
                                  allow_setup_violations,
                                  max_buffer_percent,
                                  max_passes,
                                  max_iterations,
                                  verbose);
}

void Resizer::repairHold(const sta::Pin* end_pin,
                         double setup_margin,
                         double hold_margin,
                         bool allow_setup_violations,
                         float max_buffer_percent,
                         int max_passes)
{
  // See comment on the method above.
  utl::SetAndRestore set_exclude_clk_buffers(exclude_clock_buffers_, false);
  utl::SetAndRestore set_buffers(buffer_cells_, sta::LibertyCellSeq());

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
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
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
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->initMacrosAndGrid();
  }
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  if (swap_arith_modules_->replaceArithModules(
          path_count, target, slack_margin)) {
    // Update levels and parasitics
    // - Note that updateParasitics() requires correct levels
    sta_->ensureLevelized();
    estimate_parasitics_->updateParasitics();

    // Module swap requires constant propagation and full update timing because
    // it changes many cell instances and incremental update of STA data
    // structure is incomplete.
    sta_->clearLogicConstants();
    sta_->updateTiming(true);
    sta_->findRequireds();  // Recompute timing
  }
}

////////////////////////////////////////////////////////////////
// Journal to roll back changes
void Resizer::journalBegin()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  odb::dbDatabase::beginEco(block_);

  size_up_move_->undoMoves();
  size_up_match_move_->undoMoves();
  size_down_move_->undoMoves();
  buffer_move_->undoMoves();
  clone_move_->undoMoves();
  swap_pins_move_->undoMoves();
  vt_swap_speed_move_->undoMoves();
  unbuffer_move_->undoMoves();
  split_load_move_->undoMoves();
}

void Resizer::journalEnd()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal end");
  if (!odb::dbDatabase::ecoEmpty(block_)) {
    estimate_parasitics_->updateParasitics();
    sta_->findRequireds();
  }
  odb::dbDatabase::commitEco(block_);

  int move_count_ = 0;
  move_count_ += size_up_move_->numPendingMoves();
  move_count_ += size_up_match_move_->numPendingMoves();
  move_count_ += size_down_move_->numPendingMoves();
  move_count_ += buffer_move_->numPendingMoves();
  move_count_ += clone_move_->numPendingMoves();
  move_count_ += swap_pins_move_->numPendingMoves();
  move_count_ += vt_swap_speed_move_->numPendingMoves();
  move_count_ += unbuffer_move_->numPendingMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             2,
             "COMMIT {} moves: up {} up_match {} down {} buffer {} clone {} "
             "swap {} vt_swap {} unbuf {}",
             move_count_,
             size_up_move_->numPendingMoves(),
             size_up_match_move_->numPendingMoves(),
             size_down_move_->numPendingMoves(),
             buffer_move_->numPendingMoves(),
             clone_move_->numPendingMoves(),
             swap_pins_move_->numPendingMoves(),
             vt_swap_speed_move_->numPendingMoves(),
             unbuffer_move_->numPendingMoves());

  accepted_move_count_ += move_count_;

  size_up_move_->commitMoves();
  size_up_match_move_->commitMoves();
  size_down_move_->commitMoves();
  buffer_move_->commitMoves();
  clone_move_->commitMoves();
  swap_pins_move_->commitMoves();
  vt_swap_speed_move_->commitMoves();
  unbuffer_move_->commitMoves();
  split_load_move_->commitMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             1,
             "TOTAL {} moves (acc {} rej {}): up {} up_match {} down {} buffer "
             "{} clone {} swap {} vt_swap {} unbuf {}",
             accepted_move_count_ + rejected_move_count_,
             accepted_move_count_,
             rejected_move_count_,
             size_up_move_->numCommittedMoves(),
             size_up_match_move_->numCommittedMoves(),
             size_down_move_->numCommittedMoves(),
             buffer_move_->numCommittedMoves(),
             clone_move_->numCommittedMoves(),
             swap_pins_move_->numCommittedMoves(),
             vt_swap_speed_move_->numCommittedMoves(),
             unbuffer_move_->numCommittedMoves());
}

void Resizer::journalMakeBuffer(sta::Instance* buffer)
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
    odb::dbDatabase::undoEco(block_);
    debugPrint(logger_,
               RSZ,
               "journal",
               1,
               "journal restore ends due to empty ECO >>>");
    return;
  }

  // Odb callbacks invalidate parasitics
  odb::dbDatabase::undoEco(block_);

  estimate_parasitics_->updateParasitics();
  sta_->findRequireds();

  // Update transform counts
  debugPrint(logger_,
             RSZ,
             "journal",
             1,
             "Undid {} up {} up_match {} down {} buffer {} clone {} swap {} "
             "vt_swap {} unbuf",
             size_up_move_->numPendingMoves(),
             size_up_match_move_->numPendingMoves(),
             size_down_move_->numPendingMoves(),
             buffer_move_->numPendingMoves(),
             clone_move_->numPendingMoves(),
             swap_pins_move_->numPendingMoves(),
             vt_swap_speed_move_->numPendingMoves(),
             unbuffer_move_->numPendingMoves());

  int move_count_ = 0;
  move_count_ += size_up_move_->numPendingMoves();
  move_count_ += size_up_match_move_->numPendingMoves();
  move_count_ += size_down_move_->numPendingMoves();
  move_count_ += buffer_move_->numPendingMoves();
  move_count_ += clone_move_->numPendingMoves();
  move_count_ += swap_pins_move_->numPendingMoves();
  move_count_ += vt_swap_speed_move_->numPendingMoves();
  move_count_ += unbuffer_move_->numPendingMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             2,
             "UNDO {} moves: up {} up_match {} down {} buffer {} clone {} swap "
             "{} vt_swap {} unbuf {}",
             move_count_,
             size_up_move_->numPendingMoves(),
             size_up_match_move_->numPendingMoves(),
             size_down_move_->numPendingMoves(),
             buffer_move_->numPendingMoves(),
             clone_move_->numPendingMoves(),
             swap_pins_move_->numPendingMoves(),
             vt_swap_speed_move_->numPendingMoves(),
             unbuffer_move_->numPendingMoves());

  rejected_move_count_ += move_count_;

  size_up_move_->undoMoves();
  size_up_match_move_->undoMoves();
  size_down_move_->undoMoves();
  buffer_move_->undoMoves();
  clone_move_->undoMoves();
  swap_pins_move_->undoMoves();
  vt_swap_speed_move_->undoMoves();
  unbuffer_move_->undoMoves();
  split_load_move_->undoMoves();

  debugPrint(logger_,
             RSZ,
             "opt_moves",
             1,
             "TOTAL {} moves (acc {} rej {}): up {} up_match {} down {} buffer "
             "{} clone {} swap {} vt_swap {} unbuf {}",
             accepted_move_count_ + rejected_move_count_,
             accepted_move_count_,
             rejected_move_count_,
             size_up_move_->numCommittedMoves(),
             size_up_match_move_->numCommittedMoves(),
             size_down_move_->numCommittedMoves(),
             buffer_move_->numCommittedMoves(),
             clone_move_->numCommittedMoves(),
             swap_pins_move_->numCommittedMoves(),
             vt_swap_speed_move_->numCommittedMoves(),
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

void Resizer::getBufferPins(sta::Instance* buffer, sta::Pin*& ip, sta::Pin*& op)
{
  ip = nullptr;
  op = nullptr;
  auto pin_iter = std::unique_ptr<sta::InstancePinIterator>(
      network_->pinIterator(buffer));
  while (pin_iter->hasNext()) {
    sta::Pin* pin = pin_iter->next();
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
sta::Instance* Resizer::makeBuffer(sta::LibertyCell* cell,
                                   const char* name,
                                   sta::Instance* parent,
                                   const odb::Point& loc)
{
  sta::Instance* inst = makeInstance(cell, name, parent, loc);
  journalMakeBuffer(inst);
  return inst;
}

sta::Instance* Resizer::insertBufferAfterDriver(
    sta::Net* net,
    sta::LibertyCell* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify)
{
  odb::dbMaster* buffer_master
      = db_network_->block()->getDataBase()->findMaster(buffer_cell->name());
  if (!buffer_master) {
    logger_->error(
        RSZ,
        3000,
        "insertBufferAfterDriver: Cannot find dbMaster for buffer cell {}",
        buffer_cell->name());
    return nullptr;
  }

  odb::dbNet* db_net = db_network_->staToDb(net);
  if (!db_net) {
    logger_->error(
        RSZ,
        3001,
        "insertBufferAfterDriver: Cannot convert net {} to odb::dbNet",
        network_->pathName(net));
    return nullptr;
  }

  odb::dbInst* buffer_inst = insertBufferAfterDriver(db_net,
                                                     buffer_master,
                                                     loc,
                                                     new_buf_base_name,
                                                     new_net_base_name,
                                                     uniquify);
  return db_network_->dbToSta(buffer_inst);
}

odb::dbInst* Resizer::insertBufferAfterDriver(
    odb::dbNet* net,
    odb::dbMaster* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify)
{
  // Find the driver of the current net using ODB API
  odb::dbITerm* drvr_iterm = net->getFirstOutput();
  odb::dbObject* drvr_obj = nullptr;

  if (drvr_iterm) {
    drvr_obj = drvr_iterm;
  } else {
    // Check for BTerm driver
    odb::dbSet<odb::dbBTerm> bterms = net->getBTerms();
    for (odb::dbBTerm* bterm : bterms) {
      if (bterm->getIoType() == odb::dbIoType::INPUT
          || bterm->getIoType() == odb::dbIoType::INOUT) {
        drvr_obj = bterm;
        break;
      }
    }
  }

  if (!drvr_obj) {
    logger_->error(RSZ,
                   3002,
                   "insertBufferAfterDriver: No driver found for net {}",
                   net->getName());
    return nullptr;
  }

  odb::dbInst* buffer_inst = net->insertBufferAfterDriver(drvr_obj,
                                                          buffer_cell,
                                                          loc,
                                                          new_buf_base_name,
                                                          new_net_base_name,
                                                          uniquify);

  if (!buffer_inst) {
    logger_->error(RSZ,
                   3003,
                   "insertBufferAfterDriver: Failed to insert buffer after "
                   "driver for net {}",
                   net->getName());
    return nullptr;
  }

  // Legalize the buffer position and update the design area
  insertBufferPostProcess(buffer_inst);

  return buffer_inst;
}

sta::Instance* Resizer::insertBufferBeforeLoad(
    sta::Pin* load_pin,
    sta::LibertyCell* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify)
{
  odb::dbMaster* buffer_master
      = db_network_->block()->getDataBase()->findMaster(buffer_cell->name());
  if (!buffer_master) {
    logger_->error(
        RSZ,
        3007,
        "insertBufferBeforeLoad: Cannot find dbMaster for buffer cell {}",
        buffer_cell->name());
    return nullptr;
  }

  odb::dbObject* db_load_pin = db_network_->staToDb(load_pin);
  if (!db_load_pin
      || (db_load_pin->getObjectType() != odb::dbObjectType::dbITermObj
          && db_load_pin->getObjectType() != odb::dbObjectType::dbBTermObj)) {
    logger_->error(
        RSZ,
        3008,
        "insertBufferBeforeLoad: Load pin '{}' is not an ITerm or BTerm",
        network_->pathName(load_pin));
    return nullptr;
  }

  odb::dbInst* buffer_inst = insertBufferBeforeLoad(db_load_pin,
                                                    buffer_master,
                                                    loc,
                                                    new_buf_base_name,
                                                    new_net_base_name,
                                                    uniquify);
  return db_network_->dbToSta(buffer_inst);
}

odb::dbInst* Resizer::insertBufferBeforeLoad(
    odb::dbObject* load_pin,
    odb::dbMaster* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify)
{
  // Find the flat net of the load_pin
  odb::dbNet* db_net = nullptr;
  if (load_pin->getObjectType() == odb::dbObjectType::dbITermObj) {
    odb::dbITerm* iterm = static_cast<odb::dbITerm*>(load_pin);
    db_net = iterm->getNet();
  } else if (load_pin->getObjectType() == odb::dbObjectType::dbBTermObj) {
    odb::dbBTerm* bterm = static_cast<odb::dbBTerm*>(load_pin);
    db_net = bterm->getNet();
  }

  if (!db_net) {
    logger_->error(RSZ,
                   3014,
                   "insertBufferBeforeLoad: No net found for load pin {}",
                   load_pin->getName());
    return nullptr;
  }

  // Insert buffer before the load_pin
  odb::dbInst* buffer_inst = db_net->insertBufferBeforeLoad(load_pin,
                                                            buffer_cell,
                                                            loc,
                                                            new_buf_base_name,
                                                            new_net_base_name,
                                                            uniquify);

  if (!buffer_inst) {
    logger_->error(RSZ,
                   3017,
                   "insertBufferBeforeLoad: Failed to insert buffer before "
                   "load for load pin '{}'",
                   load_pin->getName());
    return nullptr;
  }

  // Legalize the buffer position and update the design area
  insertBufferPostProcess(buffer_inst);

  return buffer_inst;
}

sta::Instance* Resizer::insertBufferBeforeLoads(
    sta::Net* net,
    sta::PinSeq* loads,
    sta::LibertyCell* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify,
    bool loads_on_diff_nets)
{
  // PinSeq -> PinSet
  sta::PinSet load_set{db_network_};
  for (const sta::Pin* pin : *loads) {
    load_set.insert(pin);
  }

  return insertBufferBeforeLoads(net,
                                 &load_set,
                                 buffer_cell,
                                 loc,
                                 new_buf_base_name,
                                 new_net_base_name,
                                 uniquify,
                                 loads_on_diff_nets);
}

sta::Instance* Resizer::insertBufferBeforeLoads(
    sta::Net* net,
    sta::PinSet* loads,
    sta::LibertyCell* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify,
    bool loads_on_diff_nets)
{
  odb::dbMaster* buffer_master
      = db_network_->block()->getDataBase()->findMaster(buffer_cell->name());
  if (!buffer_master) {
    logger_->error(
        RSZ,
        3004,
        "insertBufferBeforeLoads: Cannot find dbMaster for buffer cell {}",
        buffer_cell->name());
    return nullptr;
  }

  std::set<odb::dbObject*> db_loads;
  if (loads) {
    for (const sta::Pin* pin : *loads) {
      db_loads.insert(db_network_->staToDb(pin));
    }
  }

  odb::dbNet* db_net = net ? db_network_->staToDb(net) : nullptr;

  odb::dbInst* buffer_inst = insertBufferBeforeLoads(db_net,
                                                     db_loads,
                                                     buffer_master,
                                                     loc,
                                                     new_buf_base_name,
                                                     new_net_base_name,
                                                     uniquify,
                                                     loads_on_diff_nets);
  return db_network_->dbToSta(buffer_inst);
}

odb::dbInst* Resizer::insertBufferBeforeLoads(
    odb::dbNet* net,
    const std::set<odb::dbObject*>& loads,
    odb::dbMaster* buffer_cell,
    const odb::Point* loc,
    const char* new_buf_base_name,
    const char* new_net_base_name,
    const odb::dbNameUniquifyType& uniquify,
    bool loads_on_diff_nets)
{
  if (loads.empty()) {
    logger_->warn(RSZ, 3015, "insertBufferBeforeLoads: no loads specified");
    return nullptr;
  }

  // If no net provided, try to infer from loads
  if (!net) {
    odb::dbObject* first_load = *loads.begin();
    if (first_load->getObjectType() == odb::dbObjectType::dbITermObj) {
      net = static_cast<odb::dbITerm*>(first_load)->getNet();
    } else if (first_load->getObjectType() == odb::dbObjectType::dbBTermObj) {
      net = static_cast<odb::dbBTerm*>(first_load)->getNet();
    }
  }

  if (!net) {
    logger_->error(RSZ, 3016, "Cannot infer net from loads.");
    return nullptr;
  }

  // Make a non-const copy for dbNet API
  const std::set<odb::dbObject*>& loads_copy = loads;

  odb::dbInst* buffer_inst = net->insertBufferBeforeLoads(loads_copy,
                                                          buffer_cell,
                                                          loc,
                                                          new_buf_base_name,
                                                          new_net_base_name,
                                                          uniquify,
                                                          loads_on_diff_nets);

  if (!buffer_inst) {
    logger_->error(RSZ,
                   3006,
                   "Failed to insert buffer before loads for net {}",
                   net->getName());
    return nullptr;
  }

  // Legalize the buffer position and update the design area
  insertBufferPostProcess(buffer_inst);

  return buffer_inst;
}

sta::Instance* Resizer::makeInstance(sta::LibertyCell* cell,
                                     const char* name,
                                     sta::Instance* parent,
                                     const odb::Point& loc,
                                     const odb::dbNameUniquifyType& uniquify)
{
  debugPrint(logger_, RSZ, "make_instance", 1, "make instance {}", name);

  // make new instance name
  odb::dbModInst* parent_mod_inst = db_network_->getModInst(parent);
  std::string full_name
      = block_->makeNewInstName(parent_mod_inst, name, uniquify);

  // make new instance
  sta::Instance* inst
      = db_network_->makeInstance(cell, full_name.c_str(), parent);
  dbInst* db_inst = db_network_->staToDb(inst);
  db_inst->setSourceType(odb::dbSourceType::TIMING);
  setLocation(db_inst, loc);
  // Legalize the position of the instance in case it leaves the die
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->legalCellPos(db_inst);
  }
  designAreaIncr(area(db_inst->getMaster()));
  return inst;
}

void Resizer::insertBufferPostProcess(dbInst* buffer_inst)
{
  // Legalize the cell position for accurate parasitic estimation
  if (estimate_parasitics_->getParasiticsSrc()
          == est::ParasiticsSrc::global_routing
      || estimate_parasitics_->getParasiticsSrc()
             == est::ParasiticsSrc::detailed_routing) {
    opendp_->legalCellPos(buffer_inst);
  }

  // Increment the design area
  designAreaIncr(area(buffer_inst->getMaster()));

  // Update GUI
  if (graphics_) {
    graphics_->makeBuffer(buffer_inst);
  }

  // Increment the inserted buffer count
  inserted_buffer_count_++;
}

void Resizer::setLocation(dbInst* db_inst, const odb::Point& pt)
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

float Resizer::portCapacitance(sta::LibertyPort* input,
                               const sta::Scene* scene) const
{
  int lib_ap = scene->libertyIndex(max_);
  const sta::LibertyPort* corner_input
      = static_cast<const sta::LibertyPort*>(input)->scenePort(lib_ap);
  return corner_input->capacitance();
}

float Resizer::bufferSlew(sta::LibertyCell* buffer_cell,
                          float load_cap,
                          const sta::Scene* scene,
                          const sta::MinMax* min_max)
{
  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  sta::ArcDelay gate_delays[sta::RiseFall::index_count];
  sta::Slew slews[sta::RiseFall::index_count];
  gateDelays(output, load_cap, scene, min_max, gate_delays, slews);
  return max(slews[sta::RiseFall::riseIndex()],
             slews[sta::RiseFall::fallIndex()]);
}

float Resizer::maxInputSlew(const sta::LibertyPort* input,
                            const sta::Scene* scene) const
{
  float limit;
  bool exists;
  sta_->findSlewLimit(input, scene, sta::MinMax::max(), limit, exists);
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
      limit = sta::INF;
    }
  }
  return limit;
}

void Resizer::checkLoadSlews(const sta::Pin* drvr_pin,
                             double slew_margin,
                             // Return values.
                             sta::Slew& slew,
                             float& limit,
                             float& slack,
                             const sta::Scene*& corner)
{
  slack = sta::INF;
  limit = sta::INF;
  sta::PinConnectedPinIterator* pin_iter
      = network_->connectedPinIterator(drvr_pin);
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (pin != drvr_pin) {
      const sta::Scene* corner1;
      const sta::RiseFall* tr1;
      sta::Slew slew1;
      float limit1, slack1;
      sta_->checkSlew(pin,
                      sta_->scenes(),
                      max_,
                      false,
                      slew1,
                      limit1,
                      slack1,
                      tr1,
                      corner1);
      if (!corner1) {
        // Fixup for nangate45: see comment in maxInputSlew
        sta::LibertyPort* port = network_->libertyPort(pin);
        if (port) {
          bool exists;
          port->libertyLibrary()->defaultMaxSlew(limit1, exists);
          if (exists) {
            slew1 = 0.0;
            corner1 = tgt_slew_corner_;
            limit = limit1;
            for (const sta::RiseFall* rf : sta::RiseFall::range()) {
              auto dcalc_ap = corner1->dcalcAnalysisPtIndex(max_);
              const sta::Vertex* vertex = graph_->pinLoadVertex(pin);
              sta::Slew slew2 = sta_->graph()->slew(vertex, rf, dcalc_ap);
              slew1 = std::max(slew1, slew2);
            }
          }
        }
      } else {
        limit1 *= (1.0 - slew_margin / 100.0);
        limit1 = min(limit, limit1);
        limit = limit1;
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

void Resizer::setDebugPin(const sta::Pin* pin)
{
  debug_pin_ = pin;
}

void Resizer::setWorstSlackNetsPercent(float percent)
{
  worst_slack_nets_percent_ = percent;
}

void Resizer::annotateInputSlews(sta::Instance* inst,
                                 const sta::Scene* scene,
                                 const sta::MinMax* min_max)
{
  sta::SceneSeq scene1({(sta::Scene*) scene});
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
        slews[sta::RiseFall::rise()->index()]
            = sta_->slew(vertex, sta::RiseFallBoth::rise(), scene1, min_max);
        slews[sta::RiseFall::fall()->index()]
            = sta_->slew(vertex, sta::RiseFallBoth::fall(), scene1, min_max);
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
  std::vector<const sta::Instance*> queue;
  std::set<const sta::Instance*> kept_instances;

  auto keepInst = [&](const sta::Instance* inst) {
    if (!kept_instances.contains(inst)) {
      kept_instances.insert(inst);
      queue.push_back(inst);
    }
  };

  auto keepPinDriver = [&](sta::Pin* pin) {
    sta::PinSet* drivers = network_->drivers(pin);
    if (drivers) {
      for (const sta::Pin* drvr_pin : *drivers) {
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
      sta::Pin* pin = iter->next();
      sta::Net* net = network_->net(network_->term(pin));
      sta::PinSet* drivers = network_->drivers(net);
      if (drivers) {
        for (const sta::Pin* drvr_pin : *drivers) {
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
        sta::Pin* pin = iter->next();
        sta::Net* net = network_->net(pin);
        if (net && dontTouch(net)) {
          keepInst(inst);
        }
      }
      delete iter;
    }
  }

  while (!queue.empty()) {
    const sta::Instance* inst = queue.back();
    queue.pop_back();
    auto iter = network_->pinIterator(inst);
    while (iter->hasNext()) {
      keepPinDriver(iter->next());
    }
    delete iter;
  }

  int remove_inst_count = 0, remove_net_count = 0;
  for (auto inst : network_->leafInstances()) {
    if (!kept_instances.contains(inst)) {
      sta_->deleteInstance((sta::Instance*) inst);
      remove_inst_count++;
    }
  }

  if (clean_nets) {
    auto nets = db_network_->block()->getNets();
    for (auto it = nets.begin(); it != nets.end();) {
      if (!dontTouch(db_network_->dbToSta(*it)) && !it->isSpecial()
          && it->getITerms().empty() && it->getBTerms().empty()) {
        it = odb::dbNet::destroy(it);
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
  equiv_cells_made_ = false;
}

void Resizer::copyDontUseFromLiberty()
{
  std::unique_ptr<sta::LibertyLibraryIterator> itr(
      db_network_->libertyLibraryIterator());

  while (itr->hasNext()) {
    sta::LibertyLibrary* lib = itr->next();

    std::unique_ptr<sta::ConcreteLibraryCellIterator> cells(
        lib->cellIterator());

    while (cells->hasNext()) {
      sta::LibertyCell* lib_cell = cells->next()->libertyCell();
      if (lib_cell == nullptr) {
        continue;
      }

      if (lib_cell->dontUse()) {
        setDontUse(lib_cell, true);
      }
    }
  }
}

void Resizer::fullyRebuffer(sta::Pin* user_pin)
{
  rebuffer_->fullyRebuffer(user_pin);
}

////////////////////////////////////////////////////////////////

void Resizer::setDebugGraphics(std::shared_ptr<ResizerObserver> graphics)
{
  repair_design_->setDebugGraphics(graphics);
  graphics_ = std::move(graphics);
}

MoveType Resizer::parseMove(const std::string& s)
{
  std::string lower = s;
  std::ranges::transform(lower, lower.begin(), ::tolower);
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
  if (lower == "sizeup_match") {
    return rsz::MoveType::SIZEUP_MATCH;
  }
  if (lower == "vt_swap") {
    return rsz::MoveType::VTSWAP_SPEED;
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

bool Resizer::okToBufferNet(const sta::Pin* driver_pin) const
{
  if (isTristateDriver(driver_pin)) {
    return false;
  }

  const sta::Net* net = db_network_->dbToSta(db_network_->flatNet(driver_pin));

  if (!net) {
    return false;
  }

  if (dontTouch(net)) {
    return false;
  }

  odb::dbNet* db_net = db_network_->staToDb(net);

  if (db_net->isConnectedByAbutment() || db_net->isSpecial()) {
    return false;
  }

  return true;
}

// Check if current instance can be swapped to the
// fastest VT variant.  If not, mark it as such.
bool Resizer::checkAndMarkVTSwappable(
    sta::Instance* inst,
    std::unordered_set<sta::Instance*>& notSwappable,
    sta::LibertyCell*& best_lib_cell)
{
  best_lib_cell = nullptr;
  if (notSwappable.find(inst) != notSwappable.end()) {
    return false;
  }
  if (dontTouch(inst) || !isLogicStdCell(inst)) {
    notSwappable.insert(inst);
    return false;
  }
  sta::Cell* cell = network_->cell(inst);
  if (!cell) {
    notSwappable.insert(inst);
    return false;
  }
  sta::LibertyCell* curr_lib_cell = network_->libertyCell(cell);
  if (!curr_lib_cell) {
    notSwappable.insert(inst);
    return false;
  }
  sta::LibertyCellSeq equiv_cells = getVTEquivCells(curr_lib_cell);
  if (equiv_cells.empty()) {
    notSwappable.insert(inst);
    return false;
  }
  best_lib_cell = equiv_cells.back();
  if (best_lib_cell == curr_lib_cell) {
    best_lib_cell = nullptr;
    notSwappable.insert(inst);
    return false;
  }

  return true;
}

bool Resizer::isClockCellCandidate(sta::LibertyCell* cell)
{
  return (!cell->dontUse() && !this->dontUse(cell) && !cell->alwaysOn()
          && !cell->isIsolationCell() && !cell->isLevelShifter());
}

////////////////////////////////////////////////////////////////
// sta::Clock buffer pattern configuration

static bool containsIgnoreCase(const std::string& str,
                               const std::string& substr)
{
  auto it = std::ranges::search(str, substr, [](char a, char b) {
    return std::tolower(static_cast<unsigned char>(a))
           == std::tolower(static_cast<unsigned char>(b));
  });
  return !it.empty();  // Check if subrange is non-empty
}

void Resizer::setClockBufferString(const std::string& clk_str)
{
  clock_buffer_string_ = clk_str;
  clock_buffer_footprint_.clear();
  logger_->info(RSZ, 205, "Clock buffer string set to '{}'", clk_str);
}

void Resizer::setClockBufferFootprint(const std::string& footprint)
{
  clock_buffer_footprint_ = footprint;
  clock_buffer_string_.clear();
  logger_->info(RSZ, 206, "Clock buffer footprint set to '{}'", footprint);
}

void Resizer::resetClockBufferPattern()
{
  clock_buffer_string_.clear();
  clock_buffer_footprint_.clear();
  logger_->info(RSZ, 207, "Clock buffer string and footprint have been reset");
}

BufferUse Resizer::getBufferUse(sta::LibertyCell* buffer)
{
  // is_clock_cell is a custom lib attribute that may not exist,
  // so we also use the name/footprint pattern to help
  if (buffer->isClockCell()) {
    return BufferUse::CLOCK;
  }

  if (!clock_buffer_string_.empty()) {
    if (containsIgnoreCase(buffer->name(), clock_buffer_string_)) {
      return BufferUse::CLOCK;
    }
  } else if (!clock_buffer_footprint_.empty()) {
    const char* footprint = buffer->footprint();
    if (footprint && containsIgnoreCase(footprint, clock_buffer_footprint_)) {
      return BufferUse::CLOCK;
    }
  } else {
    // Default: check for "CLKBUF" in cell name
    if (containsIgnoreCase(buffer->name(), "CLKBUF")) {
      return BufferUse::CLOCK;
    }
  }

  return BufferUse::DATA;
}

////////////////////////////////////////////////////////////////

void Resizer::inferClockBufferList(const char* lib_name,
                                   std::vector<std::string>& buffers)
{
  // Lists to store different types of clock buffer candidates based on several
  // criteria.
  std::vector<sta::LibertyCell*> clock_cell_attribute_buffers;
  std::vector<sta::LibertyCell*> lef_use_clock_buffers;
  std::vector<sta::LibertyCell*> user_clock_buffers;
  std::vector<sta::LibertyCell*> clkbuf_pattern_buffers;
  std::vector<sta::LibertyCell*> buf_pattern_buffers;
  std::vector<sta::LibertyCell*> all_candidate_buffers;

  // Determine the pattern to use for clock buffer matching.
  // If user has configured a string or footprint via set_opt_config, use that.
  // Otherwise, fall back to the default "CLKBUF" pattern.
  bool use_user_string = hasClockBufferString();
  bool use_user_footprint = hasClockBufferFootprint();

  // 1. Iterate over all liberty libraries to find candidate cells.
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      network_->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    // Filter by library name if provided.
    if (lib_name != nullptr && strcmp(lib->name(), lib_name) != 0) {
      continue;
    }

    // Collect candidates by checking cell attributes and LEF pin signal types.
    for (sta::LibertyCell* buffer : *lib->buffers()) {
      if (!isClockCellCandidate(buffer)) {
        continue;
      }
      all_candidate_buffers.emplace_back(buffer);

      // Priority 1: Check for explicit "is_clock_cell" attribute in liberty.
      if (buffer->isClockCell()) {
        clock_cell_attribute_buffers.emplace_back(buffer);
      }

      // Priority 2: Check for any input pin with LEF signal type set to CLOCK.
      odb::dbMaster* master = db_->findMaster(buffer->name());
      for (odb::dbMTerm* mterm : master->getMTerms()) {
        if (mterm->getIoType() == odb::dbIoType::INPUT
            && mterm->getSigType() == odb::dbSigType::CLOCK) {
          lef_use_clock_buffers.emplace_back(buffer);
          break;  // Avoid duplicates for multiple clock pins
        }
      }

      // Priority 3: User-configured string or footprint matching.
      if (use_user_string) {
        if (containsIgnoreCase(buffer->name(), clock_buffer_string_)) {
          user_clock_buffers.emplace_back(buffer);
        }
      } else if (use_user_footprint) {
        const char* footprint = buffer->footprint();
        if (footprint
            && containsIgnoreCase(footprint, clock_buffer_footprint_)) {
          user_clock_buffers.emplace_back(buffer);
        }
      }

      // Priority 4: Default CLKBUF pattern matching (case-insensitive).
      if (containsIgnoreCase(buffer->name(), "CLKBUF")) {
        clkbuf_pattern_buffers.emplace_back(buffer);
      }

      // Priority 5: General BUF pattern matching (case-insensitive).
      if (containsIgnoreCase(buffer->name(), "BUF")) {
        buf_pattern_buffers.emplace_back(buffer);
      }
    }
  }

  // 2. Select the final list of buffers based on the defined priority.
  // We take the first non-empty set found.
  std::vector<sta::LibertyCell*>* selected_ptr = nullptr;
  if (!clock_cell_attribute_buffers.empty()) {
    selected_ptr = &clock_cell_attribute_buffers;
    for (sta::LibertyCell* buffer : *selected_ptr) {
      debugPrint(logger_,
                 RSZ,
                 "inferClockBufferList",
                 1,
                 "{} has clock cell attribute",
                 buffer->name());
    }
  } else if (!lef_use_clock_buffers.empty()) {
    selected_ptr = &lef_use_clock_buffers;
    for (sta::LibertyCell* buffer : *selected_ptr) {
      odb::dbMaster* master = db_->findMaster(buffer->name());
      for (odb::dbMTerm* mterm : master->getMTerms()) {
        if (mterm->getIoType() == odb::dbIoType::INPUT
            && mterm->getSigType() == odb::dbSigType::CLOCK) {
          debugPrint(logger_,
                     RSZ,
                     "inferClockBufferList",
                     1,
                     "{} has input port {} with LEF USE as CLOCK",
                     buffer->name(),
                     mterm->getName());
          break;
        }
      }
    }
  } else if (!user_clock_buffers.empty()) {
    // Priority 3: User-configured string or footprint matching.
    selected_ptr = &user_clock_buffers;
    for (sta::LibertyCell* buffer : *selected_ptr) {
      if (use_user_string) {
        debugPrint(logger_,
                   RSZ,
                   "inferClockBufferList",
                   1,
                   "{} found by user-configured string '{}'",
                   buffer->name(),
                   clock_buffer_string_);
      } else if (use_user_footprint) {
        debugPrint(logger_,
                   RSZ,
                   "inferClockBufferList",
                   1,
                   "{} found by user-configured footprint '{}'",
                   buffer->name(),
                   clock_buffer_footprint_);
      }
    }
  } else if (!clkbuf_pattern_buffers.empty()) {
    // Priority 4: Default CLKBUF pattern matching.
    selected_ptr = &clkbuf_pattern_buffers;
    for (sta::LibertyCell* buffer : *selected_ptr) {
      debugPrint(logger_,
                 RSZ,
                 "inferClockBufferList",
                 1,
                 "{} found by 'CLKBUF' pattern match",
                 buffer->name());
    }
  } else if (!buf_pattern_buffers.empty()) {
    selected_ptr = &buf_pattern_buffers;
    for (sta::LibertyCell* buffer : *selected_ptr) {
      debugPrint(logger_,
                 RSZ,
                 "inferClockBufferList",
                 1,
                 "{} found by 'BUF' pattern match",
                 buffer->name());
    }
  } else {
    // Priority 5: Fallback to all buffers that are valid clock cell candidates.
    debugPrint(logger_,
               RSZ,
               "inferClockBufferList",
               1,
               "No buffers with clock attributes or name patterns found, using "
               "all buffers");
    selected_ptr = &all_candidate_buffers;
    if (selected_ptr->empty()) {
      logger_->error(
          RSZ,
          110,
          "No clock buffer candidates could be found from any libraries.");
    }
  }

  // 3. Finalize the inferred list: store it in Resizer and populate the output
  // vector of buffer names.
  if (selected_ptr != nullptr) {
    setClockBuffersList(*selected_ptr);

    for (sta::LibertyCell* buffer : *selected_ptr) {
      buffers.emplace_back(buffer->name());
      debugPrint(logger_,
                 RSZ,
                 "inferClockBufferList",
                 1,
                 "{} has been inferred as clock buffer",
                 buffer->name());
    }
  }
}

float Resizer::getSlewRCFactor() const
{
  return repair_design_->getSlewRCFactor();
}

sta::Slew Resizer::findDriverSlewForLoad(sta::Pin* drvr_pin,
                                         float load,
                                         const sta::Scene* scene)
{
  sta::Slew max_slew = 0;

  sta::Instance* inst = network_->instance(drvr_pin);
  const sta::Pvt* pvt = scene->sdc()->pvt(inst, max_);
  if (pvt == nullptr) {
    pvt = scene->sdc()->operatingConditions(max_);
  }

  sta::VertexInEdgeIterator edge_iter(graph_->pinDrvrVertex(drvr_pin), graph_);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::TimingArcSet* arc_set = edge->timingArcSet();
    const sta::TimingRole* role = arc_set->role();
    if (!role->isTimingCheck() && role != sta::TimingRole::tristateDisable()
        && role != sta::TimingRole::tristateEnable()
        && role != sta::TimingRole::clockTreePathMin()
        && role != sta::TimingRole::clockTreePathMax()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        const sta::RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        sta::Slew in_slew = graph_delay_calc_->edgeFromSlew(
            edge->from(graph_), in_rf, role, scene, max_);
        sta::GateTimingModel* model
            = dynamic_cast<sta::GateTimingModel*>(arc->model());
        if (model) {
          sta::ArcDelay arc_delay;
          sta::Slew arc_slew;
          model->gateDelay(pvt, in_slew, load, false, arc_delay, arc_slew);
          max_slew = std::max(max_slew, arc_slew);
        }
      }
    }
  }

  return max_slew;
}

// Compute new delays and slews for the driver that drives the buffer
// Return delays, slews and load cap before and after the buffer removal
bool Resizer::computeNewDelaysSlews(
    sta::Pin* driver_pin,
    sta::Instance* buffer,
    const sta::Scene* corner,
    // return values
    sta::ArcDelay old_delay[sta::RiseFall::index_count],
    sta::ArcDelay new_delay[sta::RiseFall::index_count],
    sta::Slew old_drvr_slew[sta::RiseFall::index_count],
    sta::Slew new_drvr_slew[sta::RiseFall::index_count],
    // caps seen by driver_pin
    float& old_load_cap,
    float& new_load_cap)
{
  // Prep for delay calc
  sta::GraphDelayCalc* dcalc = sta_->graphDelayCalc();
  sta::LibertyPort* driver_port = network_->libertyPort(driver_pin);
  if (driver_port == nullptr) {
    return false;
  }

  sta::LibertyCell* buffer_cell = network_->libertyCell(buffer);
  if (!buffer_cell || !buffer_cell->isBuffer()) {
    return false;
  }

  sta::LibertyPort *buffer_input_port, *buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);
  if (!buffer_input_port || !buffer_output_port) {
    return false;
  }
  sta::Pin* buffer_out_pin = network_->findPin(buffer, buffer_output_port);
  if (!buffer_out_pin) {
    return false;
  }

  annotateInputSlews(network_->instance(driver_pin), corner, max_);

  // Compute current delays/slews/loads
  old_load_cap = dcalc->loadCap(driver_pin, corner, max_);
  gateDelays(driver_port, old_load_cap, corner, max_, old_delay, old_drvr_slew);

  // new_load_cap = current_load_on_driver + load_after_buffer -
  // buffer_input_cap
  new_load_cap = old_load_cap + dcalc->loadCap(buffer_out_pin, corner, max_)
                 - portCapacitance(buffer_input_port, corner);

  // Compute new delays/slews with updated lods
  gateDelays(driver_port, new_load_cap, corner, max_, new_delay, new_drvr_slew);

  resetInputSlews();
  return true;
}

bool Resizer::estimateSlewsAfterBufferRemoval(
    sta::Pin* drvr_pin,
    sta::Instance* buffer_instance,
    sta::Slew drvr_slew,
    const sta::Scene* corner,
    std::map<const sta::Pin*, float>& load_pin_slew)
{
  ensureLevelDrvrVertices();
  repair_design_->init();

  using BnetPtr = BufferedNetPtr;

  sta::LibertyCell* cell = network_->libertyCell(buffer_instance);
  if (!cell->isBuffer()) {
    return false;
  }
  sta::LibertyPort *in_port, *out_port;
  cell->bufferPorts(in_port, out_port);

  sta::Pin* buffer_load_pin = network_->findPin(buffer_instance, in_port);
  sta::Pin* buffer_drvr_pin = network_->findPin(buffer_instance, out_port);
  if (!buffer_load_pin || !buffer_drvr_pin) {
    logger_->report("Failed to find buffer pins\n");
    return false;
  }

  BnetPtr tree1 = makeBufferedNet(drvr_pin, corner);
  BnetPtr tree2 = makeBufferedNet(buffer_drvr_pin, corner);
  BnetPtr stitched_tree = stitchTrees(tree1, buffer_load_pin, tree2);

  if (stitched_tree == tree1) {
    return false;
  }

  sta::SceneSeq scenes({const_cast<sta::Scene*>(corner)});

  // Calibration part 1 for input path: drvr_pin -> buffer_load_pin (tree1)
  // Get current driver slew with buffer in place
  //
  // drvr_pin --> buffer_load_pin --> buffer_drvr_pin --> load_pin
  //          tree1                                 tree2
  //
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);
  assert(drvr_vertex != nullptr);
  float drvr_slew_with_buf
      = sta_->slew(drvr_vertex, sta::RiseFallBoth::riseFall(), scenes, max_);
  std::map<const sta::Pin*, float> drv2buf_slew;
  if (!estimateSlewsInTree(
          drvr_pin, drvr_slew_with_buf, tree1, corner, drv2buf_slew)) {
    return false;
  }
  auto it = drv2buf_slew.find(buffer_load_pin);
  if (it == drv2buf_slew.end()) {
    return false;
  }
  float estimated_drv2buf_slew = it->second;
  sta::Vertex* buf_load_vertex = graph_->pinLoadVertex(buffer_load_pin);
  assert(buf_load_vertex != nullptr);
  float actual_drv2buf_slew = sta_->slew(
      buf_load_vertex, sta::RiseFallBoth::riseFall(), scenes, max_);
  float in_calib_factor = !sta::fuzzyZero(estimated_drv2buf_slew)
                              ? (actual_drv2buf_slew / estimated_drv2buf_slew)
                              : 1.0f;
  debugPrint(logger_,
             RSZ,
             "slew_check",
             1,
             "estimated drv2buf slew at buffer load pin {} ={} actual slew={}:"
             " in_calib={}",
             network_->name(buffer_load_pin),
             estimated_drv2buf_slew,
             actual_drv2buf_slew,
             in_calib_factor);

  //
  // Calibration part 2 for output path: buffer_drvr_pin -> load_pin (tree2)
  //
  sta::Vertex* buf_drvr_vertex = graph_->pinDrvrVertex(buffer_drvr_pin);
  assert(buf_drvr_vertex != nullptr);
  float buf_drvr_slew = sta_->slew(
      buf_drvr_vertex, sta::RiseFallBoth::riseFall(), scenes, max_);
  std::map<const sta::Pin*, float> buf2load_slew;
  if (!estimateSlewsInTree(
          buffer_drvr_pin, buf_drvr_slew, tree2, corner, buf2load_slew)) {
    return false;
  }
  std::map<const sta::Pin*, float> out_calib_factors;
  for (const auto& [load_pin, estimated_buf2load_slew] : buf2load_slew) {
    sta::Vertex* load_vertex = graph_->pinLoadVertex(load_pin);
    assert(load_vertex != nullptr);
    float actual_buf2load_slew
        = sta_->slew(load_vertex, sta::RiseFallBoth::riseFall(), scenes, max_);
    float out_calib_factor
        = !sta::fuzzyZero(estimated_buf2load_slew)
              ? (actual_buf2load_slew / estimated_buf2load_slew)
              : 1.0f;
    out_calib_factors[load_pin] = in_calib_factor * out_calib_factor;

    debugPrint(logger_,
               RSZ,
               "slew_check",
               1,
               "estimated buf2load slew at load pin {} ={} actual slew={}:"
               " in_calib={} out_calib={}",
               network_->name(load_pin),
               estimated_buf2load_slew,
               actual_buf2load_slew,
               in_calib_factor,
               out_calib_factor);
  }

  // Estimate slews in stitched tree with driver slew after buffer removal
  if (!estimateSlewsInTree(
          drvr_pin, drvr_slew, stitched_tree, corner, load_pin_slew)) {
    return false;
  }

  // Apply calibration factors
  for (auto it = load_pin_slew.begin(); it != load_pin_slew.end(); ++it) {
    auto calib_it = out_calib_factors.find(it->first);
    if (calib_it != out_calib_factors.end()) {
      it->second *= calib_it->second;
      debugPrint(logger_,
                 RSZ,
                 "slew_check",
                 1,
                 "load pin {} has slew {}: calib_factor {}",
                 network_->name(it->first),
                 it->second,
                 calib_it->second);
    }
  }

  return true;
}

bool Resizer::estimateSlewsInTree(
    sta::Pin* drvr_pin,
    sta::Slew drvr_slew,
    const BufferedNetPtr& tree,
    const sta::Scene* corner,
    std::map<const sta::Pin*, float>& load_pin_slew)
{
  if (!tree) {
    logger_->report("Tree is null\n");
    return false;
  }

  load_pin_slew.clear();

  float worst_slew_slack = std::numeric_limits<float>::infinity();
  float worst_pin_slew = 0.0f;
  const sta::Pin* worst_pin = nullptr;

  debugPrint(logger_,
             RSZ,
             "slew_check",
             3,
             "drvr pin slew {}",
             delayAsString(drvr_slew, this));

  using BnetType = BufferedNetType;
  using BnetPtr = BufferedNetPtr;

  // Propagate slew through the tree - inline the visitor
  visitTree(
      [&](auto& recurse, int level, const BnetPtr& node, double upstream_slew)
          -> int {
        if (!node) {
          return 0;
        }

        switch (node->type()) {
          case BnetType::via: {
            double r_via
                = node->viaResistance(corner, this, estimate_parasitics_);
            double t_via = r_via * node->ref()->cap()
                           * repair_design_->slew_rc_factor_.value();
            debugPrint(logger_,
                       RSZ,
                       "slew_check",
                       3,
                       "{:{}s}via degradation {}",
                       "",
                       level,
                       delayAsString(t_via, this));
            recurse(node->ref(), upstream_slew + t_via);
            return 0;
          }
          case BnetType::wire: {
            odb::Point from_loc = node->location();
            odb::Point to_loc = node->ref()->location();
            double length
                = dbuToMeters(odb::Point::manhattanDistance(from_loc, to_loc));
            double unit_res, unit_cap;
            node->wireRC(
                corner, this, estimate_parasitics_, unit_res, unit_cap);
            double t_wire = length * unit_res
                            * (node->ref()->cap() + length * unit_cap / 2)
                            * repair_design_->slew_rc_factor_.value();
            debugPrint(logger_,
                       RSZ,
                       "slew_check",
                       3,
                       "{:{}s}wire degradation {}",
                       "",
                       level,
                       delayAsString(t_wire, this));
            recurse(node->ref(), upstream_slew + t_wire);
            return 0;
          }
          case BnetType::junction:
            recurse(node->ref(), upstream_slew);
            recurse(node->ref2(), upstream_slew);
            return 0;
          case BnetType::load: {
            debugPrint(logger_,
                       RSZ,
                       "slew_check",
                       3,
                       "{:{}s}load {} slew {}",
                       "",
                       level,
                       network_->name(node->loadPin()),
                       delayAsString(upstream_slew, this));
            load_pin_slew[node->loadPin()] = upstream_slew;
            double load_slew_slack = node->maxLoadSlew() - upstream_slew;
            if (load_slew_slack < worst_slew_slack) {
              worst_slew_slack = load_slew_slack;
              worst_pin_slew = upstream_slew;
              worst_pin = node->loadPin();
            }
            return 0;
          }
          case BnetType::buffer: {
            abort();
          }
        }
        return 0;
      },
      tree,
      drvr_slew);

  if (worst_pin) {
    debugPrint(logger_,
               RSZ,
               "slew_check",
               1,
               "worst pin {} slew {}",
               network_->name(worst_pin),
               delayAsString(worst_pin_slew, sta_));
  }

  return true;
}

}  // namespace rsz
