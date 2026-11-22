// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ClockDelayModel.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CtsOptions.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

// Share of the slew limit a driver's output may take; the wire takes the rest.
static constexpr double kDriverSlewShare = 0.85;
// Share of the slew limit that the trees are built to, which leaves room for
// what the model misses and for the buffers moving to legal places.
static constexpr double kSlewShare = 0.95;
// Slew limit, in the resizer's target slews, of a design that sets none on its
// clock network.
static constexpr double kTargetSlews = 3.0;
// Largest number of loads of a net the timer's delay calculator is asked
// about.
static constexpr size_t kStandIns = 256;

ClockDelayModel::ClockDelayModel(CtsOptions* options,
                                 odb::dbDatabase* db,
                                 sta::dbSta* sta,
                                 sta::dbNetwork* network,
                                 est::EstimateParasitics* estimate_parasitics,
                                 rsz::Resizer* resizer,
                                 utl::Logger* logger)
    : options_(options),
      db_(db),
      sta_(sta),
      network_(network),
      estimate_parasitics_(estimate_parasitics),
      resizer_(resizer),
      logger_(logger)
{
  arc_delay_calc_ = sta_->arcDelayCalc();
  if (arc_delay_calc_->name() == "unit") {
    logger_->error(CTS,
                   308,
                   "-delay_balanced has no delays to balance with the unit "
                   "delay calculator.");
  }
  dbu_per_micron_ = db_->getChip()->getBlock()->getDbUnitsPerMicron();
  meters_per_dbu_ = 1e-6 / dbu_per_micron_;
  scenes_[kBalance] = sta_->cmdScene();
  scenes_[kSlow] = sta_->cmdScene();

  findBuffers();
  initDelayCalc();
  findScenes();
  findSlewLimit();
  for (Cell& cell : cells_) {
    sta::LibertyPort *in, *out;
    cell.liberty->bufferPorts(in, out);
    findMaxLoad(cell, out);
    findPins(cell, in, out);
  }
  findMaxHop();
  const Cell& strongest = cells_[strongestBuffer()];
  if (strongest.max_load <= strongest.input_cap
      || max_hop_ < strongest.master->getWidth()) {
    logger_->error(CTS,
                   306,
                   "Clock buffer {} does not hold the clock slew limit of "
                   "{:.1f} ps driving another of its kind.",
                   strongest.name,
                   slew_limit_ * 1e12);
  }
  if (logger_->debugCheck(CTS, "delay balance", 1)) {
    reportCells();
  }
}

void ClockDelayModel::findBuffers()
{
  std::vector<std::string> names = options_->getBufferList();
  names.push_back(options_->getRootBuffer());
  struct Buffer
  {
    Cell cell;
    double drive = 0.0;
  };
  std::vector<Buffer> buffers;
  for (const std::string& name : names) {
    if (std::ranges::any_of(buffers, [&](const Buffer& buffer) {
          return buffer.cell.name == name;
        })) {
      continue;
    }
    odb::dbMaster* master = db_->findMaster(name.c_str());
    sta::LibertyCell* liberty
        = master ? network_->libertyCell(network_->dbToSta(master)) : nullptr;
    if (liberty == nullptr || !liberty->isBuffer()) {
      continue;
    }
    sta::LibertyPort *in, *out;
    liberty->bufferPorts(in, out);
    const sta::TimingArc* arc
        = findArc(liberty, in, out, sta::RiseFall::rise());
    if (arc == nullptr) {
      continue;
    }
    Buffer buffer;
    buffer.cell.name = name;
    buffer.cell.master = master;
    buffer.cell.liberty = liberty;
    buffer.cell.arc = arc;
    buffer.cell.fall_arc = findArc(liberty, in, out, sta::RiseFall::fall());
    buffer.cell.input_cap
        = in->scenePort(sta_->cmdScene(), sta::MinMax::max())
              ->capacitance(sta::RiseFall::rise(), sta::MinMax::max());
    buffer.drive = out->driveResistance();
    buffers.push_back(std::move(buffer));
  }
  if (buffers.empty()) {
    logger_->error(CTS,
                   301,
                   "No clock buffer has a delay table to balance a clock tree "
                   "with.");
  }
  std::ranges::stable_sort(buffers, [](const Buffer& a, const Buffer& b) {
    return a.drive > b.drive;
  });
  for (Buffer& buffer : buffers) {
    cells_.push_back(std::move(buffer.cell));
  }
  num_buffers_ = cells_.size();
}

void ClockDelayModel::initDelayCalc()
{
  const sta::LibertyLibrary* library
      = cells_[strongestBuffer()].liberty->libertyLibrary();
  const sta::RiseFall* rise = sta::RiseFall::rise();
  slew_lower_ = library->slewLowerThreshold(rise);
  slew_upper_ = library->slewUpperThreshold(rise);
  slew_derate_ = library->slewDerateFromLibrary();
  delay_threshold_ = library->inputThreshold(rise);

  // The calculator adjusts a load's delay to the thresholds of its library,
  // so the pins that stand in are those of cells with the buffers' thresholds.
  const auto like_buffers = [&](const sta::LibertyLibrary* other) {
    return other == library
           || (other->slewLowerThreshold(rise) == slew_lower_
               && other->slewUpperThreshold(rise) == slew_upper_
               && other->slewDerateFromLibrary() == slew_derate_
               && other->inputThreshold(rise) == delay_threshold_
               && other->outputThreshold(rise)
                      == library->outputThreshold(rise));
  };
  for (odb::dbInst* inst : db_->getChip()->getBlock()->getInsts()) {
    for (odb::dbITerm* pin : inst->getITerms()) {
      const sta::LibertyPort* port
          = network_->libertyPort(network_->dbToSta(pin));
      if (pin->getIoType() != odb::dbIoType::INPUT || !pin->isInputSignal()
          || port == nullptr
          || !like_buffers(port->libertyCell()->libertyLibrary())) {
        continue;
      }
      if (key_pin_ == nullptr) {
        key_pin_ = network_->dbToSta(pin);
      } else {
        stand_ins_.push_back(network_->dbToSta(pin));
      }
    }
    if (stand_ins_.size() >= kStandIns) {
      break;
    }
  }
}

void ClockDelayModel::reportCells() const
{
  for (int cell = 0; cell < num_buffers_; cell++) {
    debugPrint(logger_,
               CTS,
               "delay balance",
               1,
               "{}: input {:.3f} fF, largest load {:.2f} fF, delay {:.2f} ps "
               "there",
               cells_[cell].name,
               cells_[cell].input_cap * 1e15,
               cells_[cell].max_load * 1e15,
               delay(cell, nominal_slew_, cells_[cell].max_load) * 1e12);
  }
  debugPrint(
      logger_,
      CTS,
      "delay balance",
      1,
      "balanced at scene {}, slew held at scene {} within {:.2f} ps, "
      "longest wire {:.1f} um, nominal slew {:.2f} ps, vias of a "
      "buffer input {:.2f} ohm ({}), output {:.2f} ohm",
      scenes_[kBalance]->name(),
      scenes_[kSlow]->name(),
      slew_limit_ * 1e12,
      max_hop_ / dbu_per_micron_,
      nominal_slew_ * 1e12,
      viaResistance(cells_[strongestBuffer()].in_layer, kBalance, false),
      cells_[strongestBuffer()].in_layer != nullptr
          ? cells_[strongestBuffer()].in_layer->getName()
          : "no layer",
      viaResistance(cells_[strongestBuffer()].out_layer, kBalance, false));
}

void ClockDelayModel::findScenes()
{
  const int cell = strongestBuffer();
  const double load = 10.0 * cells_[cell].input_cap;
  double slowest = 0.0;
  for (const sta::Scene* scene : sta_->scenes()) {
    scenes_[kSlow] = scene;
    double delay, slew;
    gate(kSlow, cell, 0.0, load, delay, slew);
    slowest = std::max(slowest, delay);
  }
  for (const sta::Scene* scene : sta_->scenes()) {
    scenes_[kSlow] = scene;
    double delay, slew;
    gate(kSlow, cell, 0.0, load, delay, slew);
    if (delay == slowest) {
      break;
    }
  }
  for (int corner : {kBalance, kSlow}) {
    const sta::Scene* scene = scenes_[corner];
    rc_[corner][0] = {
        .r_h = estimate_parasitics_->wireClkHResistance(scene),
        .r_v = estimate_parasitics_->wireClkVResistance(scene),
        .c_h = estimate_parasitics_->wireClkHCapacitance(scene),
        .c_v = estimate_parasitics_->wireClkVCapacitance(scene),
    };
    rc_[corner][1] = {
        .r_h = estimate_parasitics_->wireSignalHResistance(scene),
        .r_v = estimate_parasitics_->wireSignalVResistance(scene),
        .c_h = estimate_parasitics_->wireSignalHCapacitance(scene),
        .c_v = estimate_parasitics_->wireSignalVCapacitance(scene),
    };
  }
}

void ClockDelayModel::findSlewLimit()
{
  const sta::MinMax* max = sta::MinMax::max();
  double limit = std::numeric_limits<double>::infinity();
  float slew;
  bool exists;
  const sta::Sdc* sdc = scenes_[kSlow]->sdc();
  bool constrained = false;
  for (const sta::Clock* clock : sdc->clocks()) {
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      sdc->slewLimit(clock, rf, sta::PathClkOrData::clk, max, slew, exists);
      if (exists) {
        limit = std::min<double>(limit, slew);
        constrained = true;
      }
    }
  }
  sdc->slewLimit(network_->cell(network_->topInstance()), max, slew, exists);
  if (exists) {
    limit = std::min<double>(limit, slew);
    constrained = true;
  }
  for (int cell = 0; cell < num_buffers_; cell++) {
    sta::LibertyPort *in, *out;
    cells_[cell].liberty->bufferPorts(in, out);
    out->slewLimit(max, slew, exists);
    if (!exists) {
      cells_[cell].liberty->libertyLibrary()->defaultMaxSlew(slew, exists);
    }
    if (exists) {
      limit = std::min<double>(limit, slew);
    }
  }
  if (!constrained) {
    double target = 0.0;
    for (const sta::RiseFall* rf : sta::RiseFall::range()) {
      target = std::max<double>(target, resizer_->targetSlew(rf));
    }
    if (target > 0.0) {
      limit = std::min(limit, kTargetSlews * target);
    }
  }
  if (!std::isfinite(limit) || limit <= 0.0) {
    logger_->error(CTS,
                   302,
                   "No slew limit found for the clock network; set one with "
                   "set_max_transition -clock_path.");
  }
  slew_limit_ = limit;
}

void ClockDelayModel::findMaxLoad(Cell& cell, const sta::LibertyPort* out)
{
  const sta::MinMax* max = sta::MinMax::max();
  float cap;
  bool exists;
  out->capacitanceLimit(max, cap, exists);
  if (!exists) {
    cell.liberty->libertyLibrary()->defaultMaxCapacitance(cap, exists);
  }
  double low = 0.0;
  double high = exists ? cap : 1e3 * cell.input_cap;
  // Slew at the output on the slower of its two edges.
  auto slew_at = [&](const double load) {
    double worst = 0.0;
    for (const sta::TimingArc* arc : {cell.arc, cell.fall_arc}) {
      if (arc != nullptr) {
        double delay, slew;
        gate(kSlow, arc, kSlewShare * slew_limit_, load, delay, slew);
        worst = std::max(worst, slew);
      }
    }
    return worst;
  };
  if (slew_at(high) > kDriverSlewShare * slew_limit_) {
    for (int i = 0; i < 40; i++) {
      const double mid = (low + high) / 2.0;
      if (slew_at(mid) > kDriverSlewShare * slew_limit_) {
        high = mid;
      } else {
        low = mid;
      }
    }
    high = low;
  }
  cell.max_load = high;
}

void ClockDelayModel::findPins(Cell& cell,
                               const sta::LibertyPort* in,
                               const sta::LibertyPort* out) const
{
  if (cell.master == nullptr) {
    return;
  }
  const odb::Point center(cell.master->getWidth() / 2,
                          cell.master->getHeight() / 2);
  auto offset = [&](const sta::LibertyPort* port) {
    odb::dbMTerm* term = cell.master->findMTerm(port->name().c_str());
    if (term == nullptr) {
      return odb::Point(0, 0);
    }
    const odb::Rect box = term->getBBox();
    return odb::Point(box.xCenter() - center.x(), box.yCenter() - center.y());
  };
  cell.in = offset(in);
  cell.out = offset(out);
  cell.in_layer = pinLayer(cell.master->findMTerm(in->name().c_str()));
  cell.out_layer = pinLayer(cell.master->findMTerm(out->name().c_str()));
}

odb::dbTechLayer* ClockDelayModel::pinLayer(odb::dbMTerm* pin)
{
  odb::dbTechLayer* lowest = nullptr;
  if (pin == nullptr) {
    return lowest;
  }
  for (odb::dbMPin* shapes : pin->getMPins()) {
    for (odb::dbBox* box : shapes->getGeometry()) {
      odb::dbTechLayer* layer = box->getTechLayer();
      if (layer != nullptr && layer->getType() == odb::dbTechLayerType::ROUTING
          && (lowest == nullptr
              || layer->getRoutingLevel() < lowest->getRoutingLevel())) {
        lowest = layer;
      }
    }
  }
  return lowest;
}

odb::dbTechLayer* ClockDelayModel::pinLayer(odb::dbBTerm* pin)
{
  odb::dbShape shape;
  return pin != nullptr && pin->getFirstPin(shape) ? shape.getTechLayer()
                                                   : nullptr;
}

double ClockDelayModel::viaResistance(odb::dbTechLayer* layer,
                                      const int corner,
                                      const bool leaf) const
{
  const auto [it, added] = vias_.try_emplace({layer, corner, leaf}, 0.0);
  if (added) {
    it->second = estimate_parasitics_->pinViaResistance(
        layer, const_cast<sta::Scene*>(scenes_[corner]), !leaf);
  }
  return it->second;
}

NetWire ClockDelayModel::finish(std::vector<TreeNode> tree,
                                const std::vector<int>& load_nodes,
                                const NetPin& driver,
                                const std::vector<NetPin>& loads,
                                const bool leaf) const
{
  // Node 0 becomes the driver pin, and each load pin a node of its own.
  for (TreeNode& node : tree) {
    node.parent++;
  }
  TreeNode root;
  tree.insert(tree.begin(), root);
  tree[1].pin = true;
  tree[1].layer = driver.layer;
  std::vector<int> pin_nodes(loads.size());
  for (int i = 0; i < loads.size(); i++) {
    TreeNode pin;
    pin.parent = load_nodes[i] + 1;
    pin.cap = loads[i].cap;
    pin.pin = true;
    pin.layer = loads[i].layer;
    pin_nodes[i] = tree.size();
    tree.push_back(pin);
  }
  NetWire wire;
  reduce(tree, pin_nodes, leaf, wire);
  return wire;
}

void ClockDelayModel::findMaxHop()
{
  const int cell = strongestBuffer();
  const NetPin driver{.layer = cells_[cell].out_layer};
  const auto load_at = [&](const int x, const int y) {
    return std::vector<NetPin>{
        {
            .x = x,
            .y = y,
            .cap = cells_[cell].input_cap,
            .layer = cells_[cell].in_layer,
        },
    };
  };
  const auto ok = [&](const double length) {
    const int reach = static_cast<int>(length);
    return slewOk(cell, starWire(driver, load_at(reach, 0), false))
           && slewOk(cell, starWire(driver, load_at(0, reach), false));
  };
  double low = 0.0;
  double high = 10000.0 * dbu_per_micron_;
  if (options_->getMaxWl() > 0) {
    high = std::min<double>(high, options_->getMaxWl());
  }
  if (!ok(high)) {
    for (int i = 0; i < 40; i++) {
      const double mid = (low + high) / 2.0;
      if (ok(mid)) {
        low = mid;
      } else {
        high = mid;
      }
    }
    high = low;
  }
  max_hop_ = high;

  // The slew a buffer has at its input half a hop from one like it.
  double slew = 0.0;
  const NetWire half
      = starWire(driver, load_at(static_cast<int>(max_hop_ / 2.0), 0), false);
  for (int i = 0; i < 4; i++) {
    slew = drive(kBalance, cell, slew, half).load_slew[0];
  }
  nominal_slew_ = slew;
}

const sta::TimingArc* ClockDelayModel::findArc(const sta::LibertyCell* cell,
                                               const sta::LibertyPort* from,
                                               const sta::LibertyPort* to,
                                               const sta::RiseFall* edge) const
{
  const sta::MinMax* max = sta::MinMax::max();
  for (const sta::TimingArcSet* set : cell->timingArcSets(from, to)) {
    for (const sta::TimingArc* arc : set->arcs()) {
      if (arc->fromEdge()->asRiseFall() == edge
          && arc->gateTableModel(scenes_[kBalance], max) != nullptr
          && arc->gateTableModel(scenes_[kSlow], max) != nullptr) {
        return arc;
      }
    }
  }
  return nullptr;
}

bool ClockDelayModel::isRegisterClock(odb::dbITerm* pin) const
{
  const sta::LibertyPort* port = network_->libertyPort(network_->dbToSta(pin));
  if (port == nullptr) {
    return false;
  }
  const sta::LibertyCell* cell = port->libertyCell();
  return port->isRegClk() || (cell != nullptr && cell->isPad());
}

int ClockDelayModel::findBuffer(const std::string& master) const
{
  for (int cell = 0; cell < num_buffers_; cell++) {
    if (cells_[cell].name == master) {
      return cell;
    }
  }
  return -1;
}

int ClockDelayModel::addCell(sta::LibertyCell* cell,
                             const sta::LibertyPort* from,
                             const sta::LibertyPort* to)
{
  const sta::TimingArc* arc = findArc(cell, from, to, sta::RiseFall::rise());
  if (arc == nullptr) {
    return -1;
  }
  for (int i = 0; i < cells_.size(); i++) {
    if (cells_[i].arc == arc) {
      return i;
    }
  }
  Cell added;
  added.name = cell->name();
  added.master = db_->findMaster(added.name.c_str());
  added.liberty = cell;
  added.arc = arc;
  added.fall_arc = findArc(cell, from, to, sta::RiseFall::fall());
  added.input_cap
      = from->scenePort(scenes_[kBalance], sta::MinMax::max())
            ->capacitance(sta::RiseFall::rise(), sta::MinMax::max());
  cells_.push_back(std::move(added));
  findMaxLoad(cells_.back(), to);
  findPins(cells_.back(), from, to);
  return cells_.size() - 1;
}

void ClockDelayModel::gate(const Corner corner,
                           const int cell,
                           const double in_slew,
                           const double load,
                           double& delay,
                           double& slew) const
{
  gate(corner, cells_[cell].arc, in_slew, load, delay, slew);
}

void ClockDelayModel::gate(const Corner corner,
                           const sta::TimingArc* arc,
                           const double in_slew,
                           const double load,
                           double& delay,
                           double& slew) const
{
  const sta::Scene* scene = scenes_[corner];
  const sta::MinMax* max = sta::MinMax::max();
  const sta::GateTableModel* model = arc->gateTableModel(scene, max);
  const sta::Pvt* pvt = scene->sdc()->operatingConditions(max);
  float gate_delay = 0.0;
  float drvr_slew = 0.0;
  model->gateDelay(pvt, in_slew, load, gate_delay, drvr_slew);
  delay = gate_delay;
  slew = drvr_slew;
}

double ClockDelayModel::delay(const int cell,
                              const double in_slew,
                              const double load) const
{
  double delay, slew;
  gate(kBalance, cell, in_slew, load, delay, slew);
  return delay;
}

StageTiming ClockDelayModel::drive(const Corner corner,
                                   const int cell,
                                   const double in_slew,
                                   const NetWire& wire) const
{
  return drive(corner, cells_[cell].arc, in_slew, wire);
}

StageTiming ClockDelayModel::drive(const Corner corner,
                                   const sta::TimingArc* arc,
                                   const double in_slew,
                                   const NetWire& wire) const
{
  const double near = wire.c_near[corner];
  const double far = wire.c_far[corner];
  const double tau = wire.r_pi[corner] * far;
  const size_t loads = wire.elmore[corner].size();
  StageTiming stage;
  stage.load = near + far;
  gate(corner, arc, in_slew, stage.load, stage.delay, stage.slew);
  if (tau > 0.0) {
    // The capacitance behind the wire's resistance charges through it while
    // the output ramps; the driver sees the share that takes charge by then.
    for (int i = 0; i < 3; i++) {
      const double ramp
          = stage.slew * slew_derate_ / (slew_upper_ - slew_lower_);
      if (ramp <= 0.0) {
        break;
      }
      const double k = tau / ramp;
      stage.load = near + far * (1.0 - k * (1.0 - std::exp(-1.0 / k)));
      gate(corner, arc, in_slew, stage.load, stage.delay, stage.slew);
    }
  }
  stage.wire_delay.resize(loads);
  stage.load_slew.resize(loads);

  const sta::Scene* scene = scenes_[corner];
  const sta::MinMax* max = sta::MinMax::max();
  sta::Parasitics* parasitics = scene->parasitics(max);
  if (parasitics == nullptr || key_pin_ == nullptr
      || loads > stand_ins_.size()) {
    for (size_t i = 0; i < loads; i++) {
      wireEnd(stage.slew,
              wire.elmore[corner][i],
              stage.wire_delay[i],
              stage.load_slew[i]);
    }
    return stage;
  }
  sta::Parasitic* parasitic = parasitics->makePiElmore(
      key_pin_, arc->toEdge()->asRiseFall(), max, near, wire.r_pi[corner], far);
  for (size_t i = 0; i < loads; i++) {
    parasitics->setElmore(parasitic, stand_ins_[i], wire.elmore[corner][i]);
  }
  if (load_maps_.size() <= loads) {
    load_maps_.resize(loads + 1);
  }
  if (load_maps_[loads] == nullptr) {
    load_maps_[loads]
        = std::make_unique<sta::LoadPinIndexMap>(sta::PinIdLess(network_));
    for (size_t i = 0; i < loads; i++) {
      (*load_maps_[loads])[stand_ins_[i]] = i;
    }
  }
  sta::ArcDcalcResult result = arc_delay_calc_->gateDelay(nullptr,
                                                          arc,
                                                          sta::Slew(in_slew),
                                                          near + far,
                                                          parasitic,
                                                          *load_maps_[loads],
                                                          scene,
                                                          max);
  stage.delay = sta::delayAsFloat(result.gateDelay());
  stage.slew = sta::delayAsFloat(result.drvrSlew());
  for (size_t i = 0; i < loads; i++) {
    stage.wire_delay[i] = sta::delayAsFloat(result.wireDelay(i));
    stage.load_slew[i] = sta::delayAsFloat(result.loadSlew(i));
  }
  return stage;
}

StageTiming ClockDelayModel::port(const Corner corner,
                                  const double slew,
                                  const NetWire& wire) const
{
  const size_t loads = wire.elmore[corner].size();
  StageTiming stage;
  stage.slew = slew;
  stage.load = wire.cap[corner];
  stage.wire_delay.resize(loads);
  stage.load_slew.resize(loads);
  for (size_t i = 0; i < loads; i++) {
    wireEnd(
        slew, wire.elmore[corner][i], stage.wire_delay[i], stage.load_slew[i]);
  }
  return stage;
}

double ClockDelayModel::delayAt(const int cell,
                                const double delay,
                                const double in_slew,
                                const double load) const
{
  return delay + this->delay(cell, in_slew, load)
         - this->delay(cell, nominal_slew_, load);
}

double ClockDelayModel::portCap(odb::dbBTerm* port) const
{
  return scenes_[kBalance]->sdc()->portExtCap(
      network_->port(network_->dbToSta(port)),
      sta::RiseFall::rise(),
      sta::MinMax::max());
}

double ClockDelayModel::pinCap(odb::dbITerm* pin) const
{
  return scenes_[kBalance]->sdc()->pinCapacitance(network_->dbToSta(pin),
                                                  sta::RiseFall::rise(),
                                                  scenes_[kBalance],
                                                  sta::MinMax::max());
}

ClockDelayModel::~ClockDelayModel()
{
  if (key_pin_ == nullptr) {
    return;
  }
  for (const sta::Scene* scene : sta_->scenes()) {
    sta::Parasitics* parasitics = scene->parasitics(sta::MinMax::max());
    if (parasitics != nullptr) {
      parasitics->deleteParasitics(key_pin_);
    }
  }
}

double ClockDelayModel::crossing(const double ramp,
                                 const double tau,
                                 const double v)
{
  auto level = [&](const double t) {
    if (t <= ramp) {
      return (t - tau * (1.0 - std::exp(-t / tau))) / ramp;
    }
    return 1.0
           - (tau / ramp) * (1.0 - std::exp(-ramp / tau))
                 * std::exp(-(t - ramp) / tau);
  };
  double low = v * ramp;
  double high = v * ramp + 40.0 * tau;
  for (int i = 0; i < 44; i++) {
    const double mid = (low + high) / 2.0;
    if (level(mid) < v) {
      low = mid;
    } else {
      high = mid;
    }
  }
  return (low + high) / 2.0;
}

void ClockDelayModel::wireEnd(const double drvr_slew,
                              const double elmore,
                              double& delay,
                              double& slew) const
{
  if (elmore <= drvr_slew * 1e-3 || drvr_slew <= 0.0) {
    delay = elmore;
    slew = drvr_slew;
    return;
  }
  const double ramp = drvr_slew * slew_derate_ / (slew_upper_ - slew_lower_);
  delay = crossing(ramp, elmore, delay_threshold_) - delay_threshold_ * ramp;
  slew = (crossing(ramp, elmore, slew_upper_)
          - crossing(ramp, elmore, slew_lower_))
         / slew_derate_;
  slew = std::max(slew, drvr_slew);
}

double ClockDelayModel::wireCap(const double length, const bool leaf) const
{
  const WireRc& rc = rc_[kBalance][leaf];
  return dbuToMeters(length) * (rc.c_h + rc.c_v) / 2.0;
}

double ClockDelayModel::worstSlew(const int cell, const NetWire& wire) const
{
  double worst = 0.0;
  for (const sta::TimingArc* arc : {cells_[cell].arc, cells_[cell].fall_arc}) {
    if (arc == nullptr) {
      continue;
    }
    const StageTiming stage = drive(kSlow, arc, kSlewShare * slew_limit_, wire);
    worst = std::max(worst, stage.slew);
    for (const double slew : stage.load_slew) {
      worst = std::max(worst, slew);
    }
  }
  return worst;
}

bool ClockDelayModel::slewOk(const int cell, const NetWire& wire) const
{
  return worstSlew(cell, wire) <= kSlewShare * slew_limit_;
}

void ClockDelayModel::reduce(const std::vector<TreeNode>& tree,
                             const std::vector<int>& load_nodes,
                             const bool leaf,
                             NetWire& wire) const
{
  const int count = tree.size();
  std::vector<double> res(count);
  std::vector<double> cap(count);
  std::vector<double> below(count);
  std::vector<double> tau(count);
  std::vector<double> moment(count);
  wire.length = 0.0;
  for (const TreeNode& node : tree) {
    wire.length += node.dx + node.dy;
  }
  for (int corner : {kBalance, kSlow}) {
    const WireRc& rc = rc_[corner][leaf];
    for (int k = 0; k < count; k++) {
      cap[k] = tree[k].cap;
    }
    for (int k = 1; k < count; k++) {
      res[k] = (tree[k].dx * rc.r_h + tree[k].dy * rc.r_v) * meters_per_dbu_;
      if (tree[k].pin) {
        res[k] += viaResistance(tree[k].layer, corner, leaf);
      }
      const double half
          = (tree[k].dx * rc.c_h + tree[k].dy * rc.c_v) * meters_per_dbu_ / 2.0;
      cap[k] += half;
      cap[tree[k].parent] += half;
    }
    below = cap;
    for (int k = count - 1; k > 0; k--) {
      below[tree[k].parent] += below[k];
    }
    tau[0] = 0.0;
    for (int k = 1; k < count; k++) {
      tau[k] = tau[tree[k].parent] + res[k] * below[k];
    }
    // below: now the sum of capacitance times Elmore delay under each node.
    for (int k = 0; k < count; k++) {
      below[k] = cap[k] * tau[k];
    }
    for (int k = count - 1; k > 0; k--) {
      below[tree[k].parent] += below[k];
    }
    moment[0] = 0.0;
    double y1 = 0.0;
    double y3 = 0.0;
    for (int k = 0; k < count; k++) {
      if (k > 0) {
        moment[k] = moment[tree[k].parent] + res[k] * below[k];
      }
      y1 += cap[k];
      y3 += cap[k] * moment[k];
    }
    const double y2 = -below[0];
    wire.cap[corner] = y1;
    if (y2 < 0.0 && y3 > 0.0) {
      wire.c_far[corner] = std::min(y1, y2 * y2 / y3);
      wire.c_near[corner] = y1 - wire.c_far[corner];
      wire.r_pi[corner] = -y3 * y3 / (y2 * y2 * y2);
    } else {
      wire.c_far[corner] = 0.0;
      wire.c_near[corner] = y1;
      wire.r_pi[corner] = 0.0;
    }
    wire.elmore[corner].resize(load_nodes.size());
    for (int i = 0; i < load_nodes.size(); i++) {
      wire.elmore[corner][i] = tau[load_nodes[i]];
    }
  }
}

NetWire ClockDelayModel::starWire(const NetPin& driver,
                                  const std::vector<NetPin>& loads,
                                  const bool leaf) const
{
  std::vector<TreeNode> tree(1 + loads.size());
  std::vector<int> load_nodes(loads.size());
  for (int i = 0; i < loads.size(); i++) {
    tree[i + 1] = {
        .parent = 0,
        .dx = std::abs(loads[i].x - driver.x),
        .dy = std::abs(loads[i].y - driver.y),
    };
    load_nodes[i] = i + 1;
  }
  return finish(std::move(tree), load_nodes, driver, loads, leaf);
}

NetWire ClockDelayModel::wire(const NetPin& driver,
                              const std::vector<NetPin>& loads,
                              const bool leaf) const
{
  // The pins at distinct locations; point 0 is the driver.
  std::vector<int> xs{driver.x};
  std::vector<int> ys{driver.y};
  std::map<std::pair<int, int>, int> points;
  points[{driver.x, driver.y}] = 0;
  std::vector<int> load_points(loads.size());
  for (int i = 0; i < loads.size(); i++) {
    const auto [it, added]
        = points.try_emplace({loads[i].x, loads[i].y}, xs.size());
    if (added) {
      xs.push_back(loads[i].x);
      ys.push_back(loads[i].y);
    }
    load_points[i] = it->second;
  }
  const int count = xs.size();
  if (count <= 2) {
    std::vector<int> load_nodes(loads.size(), 0);
    std::vector<TreeNode> tree(count);
    if (count == 2) {
      tree[1] = {
          .parent = 0,
          .dx = std::abs(xs[1] - xs[0]),
          .dy = std::abs(ys[1] - ys[0]),
      };
      for (int i = 0; i < loads.size(); i++) {
        load_nodes[i] = load_points[i];
      }
    }
    return finish(std::move(tree), load_nodes, driver, loads, leaf);
  }
  std::vector<TreeNode> tree;
  std::vector<int> load_nodes;
  if (!steinerTree(driver, loads, points, load_points, tree, load_nodes)) {
    return starWire(driver, loads, leaf);
  }
  return finish(std::move(tree), load_nodes, driver, loads, leaf);
}

bool ClockDelayModel::steinerTree(
    const NetPin& driver,
    const std::vector<NetPin>& loads,
    const std::map<std::pair<int, int>, int>& points,
    const std::vector<int>& load_points,
    std::vector<TreeNode>& tree,
    std::vector<int>& load_nodes) const
{
  const int count = points.size();
  // The estimator gives the Steiner tree every pin, sorted by location.
  std::vector<std::pair<int, int>> pins{{driver.x, driver.y}};
  for (const NetPin& load : loads) {
    pins.emplace_back(load.x, load.y);
  }
  std::ranges::sort(pins);
  std::vector<int> pins_x;
  std::vector<int> pins_y;
  int drvr_index = 0;
  for (int i = 0; i < pins.size(); i++) {
    pins_x.push_back(pins[i].first);
    pins_y.push_back(pins[i].second);
    if (pins[i] == std::pair(driver.x, driver.y)) {
      drvr_index = i;
    }
  }
  const stt::Tree steiner
      = options_->getSttBuilder()->makeSteinerTree(pins_x, pins_y, drvr_index);
  const int branches = steiner.branchCount();
  std::vector<std::vector<int>> neighbors(branches);
  for (int i = 0; i < branches; i++) {
    const int j = steiner.branch[i].n;
    if (j != i && j >= 0 && j < branches) {
      neighbors[i].push_back(j);
      neighbors[j].push_back(i);
    }
  }
  // The Steiner tree's first points are the pins, found by location.
  std::vector<int> branch_of(count, -1);
  for (int i = 0; i < branches; i++) {
    const auto it = points.find({steiner.branch[i].x, steiner.branch[i].y});
    if (it != points.end() && branch_of[it->second] < 0) {
      branch_of[it->second] = i;
    }
  }
  if (branch_of[0] < 0) {
    return false;
  }
  tree.reserve(branches);
  std::vector<int> node_of(branches, -1);
  std::vector<int> order;
  order.reserve(branches);
  order.push_back(branch_of[0]);
  node_of[branch_of[0]] = 0;
  tree.push_back({});
  for (int head = 0; head < order.size(); head++) {
    const int at = order[head];
    for (const int next : neighbors[at]) {
      if (node_of[next] >= 0) {
        continue;
      }
      node_of[next] = tree.size();
      tree.push_back({
          .parent = node_of[at],
          .dx = std::abs(steiner.branch[next].x - steiner.branch[at].x),
          .dy = std::abs(steiner.branch[next].y - steiner.branch[at].y),
      });
      order.push_back(next);
    }
  }
  for (int point = 0; point < count; point++) {
    if (branch_of[point] < 0 || node_of[branch_of[point]] < 0) {
      return false;
    }
  }
  load_nodes.resize(loads.size());
  for (int i = 0; i < loads.size(); i++) {
    load_nodes[i] = node_of[branch_of[load_points[i]]];
  }
  return true;
}

}  // namespace cts
