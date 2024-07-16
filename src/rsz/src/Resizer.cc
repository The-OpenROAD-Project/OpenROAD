/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <cmath>
#include <limits>
#include <optional>

#include "AbstractSteinerRenderer.h"
#include "BufferedNet.hh"
#include "RecoverPower.hh"
#include "RepairDesign.hh"
#include "RepairHold.hh"
#include "RepairSetup.hh"
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

using odb::dbBox;
using odb::dbInst;
using odb::dbMaster;
using odb::dbPlacementStatus;

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
using sta::Term;
using sta::TimingArcSet;
using sta::TimingArcSetSeq;
using sta::TimingRole;
;
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
using sta::stringPrint;
using sta::VertexIterator;
using sta::VertexOutEdgeIterator;

Resizer::Resizer()
    : recover_power_(new RecoverPower(this)),
      repair_design_(new RepairDesign(this)),
      repair_setup_(new RepairSetup(this)),
      repair_hold_(new RepairHold(this)),
      wire_signal_res_(0.0),
      wire_signal_cap_(0.0),
      wire_clk_res_(0.0),
      wire_clk_cap_(0.0),
      tgt_slews_{0.0, 0.0}
{
}

Resizer::~Resizer()
{
  delete repair_design_;
  delete repair_setup_;
  delete repair_hold_;
}

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
  inserted_buffer_set_ = InstanceSet(db_network_);
  steiner_renderer_ = std::move(steiner_renderer);
  all_sized_inst_set_ = InstanceSet(db_network_);
  all_inserted_buffer_set_ = InstanceSet(db_network_);
  all_swapped_pin_inst_set_ = InstanceSet(db_network_);
  all_cloned_inst_set_ = InstanceSet(db_network_);
}

////////////////////////////////////////////////////////////////

double Resizer::coreArea() const
{
  return dbuToMeters(core_.dx()) * dbuToMeters(core_.dy());
}

double Resizer::utilization()
{
  initBlock();
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

// block_ indicates core_, design_area_, db_network_ etc valid.
void Resizer::initBlock()
{
  block_ = db_->getChip()->getBlock();
  core_ = block_->getCoreArea();
  core_exists_ = !(core_.xMin() == 0 && core_.xMax() == 0 && core_.yMin() == 0
                   && core_.yMax() == 0);
  dbu_ = db_->getTech()->getDbUnitsPerMicron();
}

void Resizer::init()
{
  initBlock();
  sta_->ensureLevelized();
  graph_ = sta_->graph();
  initDesignArea();
}

// remove all buffers if no buffers are specified
void Resizer::removeBuffers(sta::InstanceSeq insts)
{
  initBlock();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  int remove_count = 0;
  if (insts.empty()) {
    // remove all the buffers
    for (dbInst* db_inst : block_->getInsts()) {
      Instance* buffer = db_network_->dbToSta(db_inst);
      if (removeBuffer(buffer, /* honor dont touch */ true)) {
        remove_count++;
      }
    }
  } else {
    // remove only select buffers specified by user
    InstanceSeq::Iterator inst_iter(insts);
    while (inst_iter.hasNext()) {
      Instance* buffer = const_cast<Instance*>(inst_iter.next());
      if (removeBuffer(buffer, /* don't honor dont touch */ false)) {
        remove_count++;
      } else {
        logger_->warn(
            RSZ,
            97,
            "Instance {} cannot be removed because it is not a buffer",
            " or is a feedthrough port buffer",
            db_network_->name(buffer));
      }
    }
  }
  level_drvr_vertices_valid_ = false;
  logger_->info(RSZ, 26, "Removed {} buffers.", remove_count);
}

bool Resizer::bufferBetweenPorts(Instance* buffer)
{
  LibertyCell* lib_cell = network_->libertyCell(buffer);
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin* in_pin = db_network_->findPin(buffer, in_port);
  Pin* out_pin = db_network_->findPin(buffer, out_port);
  Net* in_net = db_network_->net(in_pin);
  Net* out_net = db_network_->net(out_pin);
  return hasPort(in_net) && hasPort(out_net);
}

// There are two buffer removal modes: auto and manual:
// 1) auto mode: this happens during setup fixing, power recovery, buffer
// removal
//      Dont-touch, fixed cell and boundary buffer constraints are honored
// 2) manual mode: this happens during manual buffer removal during ECO
//      This ignores dont-touch and fixed cell (boundary buffer constraints are
//      still honored)
bool Resizer::removeBuffer(Instance* buffer, bool honorDontTouchFixed)
{
  LibertyCell* lib_cell = network_->libertyCell(buffer);
  if (!lib_cell || !lib_cell->isBuffer()) {
    return false;
  }
  // Do not remove buffers connected to input/output ports
  // because verilog netlists use the net name for the port.
  if (bufferBetweenPorts(buffer)) {
    return false;
  }
  dbInst* db_inst = db_network_->staToDb(buffer);
  if (db_inst->isDoNotTouch()) {
    if (honorDontTouchFixed) {
      return false;
    }
    //  remove instance dont touch
    db_inst->setDoNotTouch(false);
  }
  if (db_inst->isFixed()) {
    if (honorDontTouchFixed) {
      return false;
    }
    // change FIXED to PLACED just in case
    db_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin* in_pin = db_network_->findPin(buffer, in_port);
  Pin* out_pin = db_network_->findPin(buffer, out_port);
  Net* in_net = db_network_->net(in_pin);
  Net* out_net = db_network_->net(out_pin);
  dbNet* in_db_net = db_network_->staToDb(in_net);
  dbNet* out_db_net = db_network_->staToDb(out_net);
  // honor net dont-touch on input net or output net
  if (in_db_net->isDoNotTouch() || out_db_net->isDoNotTouch()) {
    if (honorDontTouchFixed) {
      return false;
    }
    // remove net dont touch for manual ECO
    in_db_net->setDoNotTouch(false);
    out_db_net->setDoNotTouch(false);
  }
  bool out_net_ports = hasPort(out_net);
  Net *survivor, *removed;
  if (out_net_ports) {
    if (hasPort(in_net)) {
      return false;
    }
    survivor = out_net;
    removed = in_net;
  } else {
    // default or out_net_ports
    // Default to in_net surviving so drivers (cached in dbNetwork)
    // do not change.
    survivor = in_net;
    removed = out_net;
  }

  bool buffer_removed = false;
  if (!sdc_->isConstrained(in_pin) && !sdc_->isConstrained(out_pin)
      && !sdc_->isConstrained(removed) && !sdc_->isConstrained(buffer)) {
    debugPrint(logger_,
               RSZ,
               "remove_buffer",
               1,
               "remove {}",
               db_network_->name(buffer));
    buffer_removed = true;
    sta_->disconnectPin(in_pin);
    sta_->disconnectPin(out_pin);
    sta_->deleteInstance(buffer);

    if (removed) {
      NetPinIterator* pin_iter = db_network_->pinIterator(removed);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        Instance* pin_inst = db_network_->instance(pin);
        if (pin_inst != buffer) {
          Port* pin_port = db_network_->port(pin);
          sta_->disconnectPin(const_cast<Pin*>(pin));
          sta_->connectPin(pin_inst, pin_port, survivor);
        }
      }
      delete pin_iter;
      sta_->deleteNet(removed);
      parasitics_invalid_.erase(removed);
    }
    parasiticsInvalid(survivor);
    updateParasitics();
  }
  return buffer_removed;
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

void Resizer::balanceBin(const vector<odb::dbInst*>& bin)
{
  // Maps sites to the total width of all instances using that site
  map<odb::dbSite*, uint64_t> sites;
  uint64_t total_width = 0;
  for (auto inst : bin) {
    auto master = inst->getMaster();
    sites[master->getSite()] += master->getWidth();
    total_width += master->getWidth();
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
      Instance* sta_inst = db_network_->dbToSta(inst);
      LibertyCell* cell = network_->libertyCell(sta_inst);
      LibertyCellSeq* equiv_cells = sta_->equivCells(cell);
      if (!equiv_cells) {
        continue;
      }
      for (LibertyCell* target_cell : *equiv_cells) {
        if (dontUse(target_cell)) {
          continue;
        }
        dbMaster* target_master = db_network_->staToDb(target_cell);
        // TODO: pick the best choice rather than the first
        //       and consider timing criticality
        if (target_master->getSite() == site) {
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

  for (auto inst : block_->getInsts()) {
    auto master = inst->getMaster();
    auto site = master->getSite();
    // Ignore multi-height cells for now
    if (site->hasRowPattern()) {
      continue;
    }

    const Point origin = inst->getOrigin();
    const int x_bin = (origin.x() - core_.xMin()) / x_step;
    const int y_bin = (origin.y() - core_.yMin()) / y_step;
    grid[x_bin][y_bin].push_back(inst);
  }

  for (int x = 0; x < num_bins; ++x) {
    for (int y = 0; y < num_bins; ++y) {
      balanceBin(grid[x][y]);
    }
  }
}

////////////////////////////////////////////////////////////////

void Resizer::findBuffers()
{
  if (buffer_cells_.empty()) {
    LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
    while (lib_iter->hasNext()) {
      LibertyLibrary* lib = lib_iter->next();
      for (LibertyCell* buffer : *lib->buffers()) {
        if (!dontUse(buffer) && isLinkCell(buffer)) {
          buffer_cells_.emplace_back(buffer);
        }
      }
    }
    delete lib_iter;

    if (buffer_cells_.empty()) {
      logger_->error(RSZ, 22, "no buffers found.");
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

bool Resizer::isLinkCell(LibertyCell* cell)
{
  return network_->findLibertyCell(cell->name()) == cell;
}

////////////////////////////////////////////////////////////////

void Resizer::bufferInputs()
{
  init();
  findBuffers();
  sta_->ensureClkNetwork();
  inserted_buffer_count_ = 0;
  buffer_moved_into_core_ = false;

  incrementalParasiticsBegin();
  InstancePinIterator* port_iter
      = network_->pinIterator(network_->topInstance());
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
      bufferInput(pin, buffer_lowest_drive_);
    }
  }
  delete port_iter;
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    logger_->info(
        RSZ, 27, "Inserted {} input buffers.", inserted_buffer_count_);
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

Instance* Resizer::bufferInput(const Pin* top_pin, LibertyCell* buffer_cell)
{
  Term* term = db_network_->term(top_pin);
  Net* input_net = db_network_->net(term);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);

  bool has_non_buffer = false;
  bool has_dont_touch = false;
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(input_net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    // Leave input port pin connected to input_net.
    if (pin != top_pin) {
      auto inst = network_->instance(pin);
      if (dontTouch(inst)) {
        has_dont_touch = true;
        logger_->warn(RSZ,
                      85,
                      "Input {} can't be buffered due to dont-touch fanout {}",
                      network_->name(input_net),
                      network_->name(pin));
        break;
      }
      auto cell = network_->cell(inst);
      auto lib = network_->libertyCell(cell);
      if (lib && !lib->isBuffer()) {
        has_non_buffer = true;
      }
    }
  }
  delete pin_iter;

  if (has_dont_touch || !has_non_buffer) {
    return nullptr;
  }

  string buffer_name = makeUniqueInstName("input");
  Instance* parent = db_network_->topInstance();
  Net* buffer_out = makeUniqueNet();
  Point pin_loc = db_network_->location(top_pin);
  Instance* buffer
      = makeBuffer(buffer_cell, buffer_name.c_str(), parent, pin_loc);
  inserted_buffer_count_++;

  pin_iter = network_->connectedPinIterator(input_net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    // Leave input port pin connected to input_net.
    if (pin != top_pin) {
      sta_->disconnectPin(const_cast<Pin*>(pin));
      Port* pin_port = db_network_->port(pin);
      sta_->connectPin(db_network_->instance(pin), pin_port, buffer_out);
    }
  }
  delete pin_iter;
  sta_->connectPin(buffer, input, input_net);
  sta_->connectPin(buffer, output, buffer_out);

  parasiticsInvalid(input_net);
  parasiticsInvalid(buffer_out);
  return buffer;
}

void Resizer::bufferOutputs()
{
  init();
  findBuffers();
  inserted_buffer_count_ = 0;
  buffer_moved_into_core_ = false;

  incrementalParasiticsBegin();
  InstancePinIterator* port_iter
      = network_->pinIterator(network_->topInstance());
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
      bufferOutput(pin, buffer_lowest_drive_);
    }
  }
  delete port_iter;
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    logger_->info(
        RSZ, 28, "Inserted {} output buffers.", inserted_buffer_count_);
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
      odb::dbModBTerm* modbterm;
      db_network_->staToDb(pin, iterm, bterm, moditerm, modbterm);
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

void Resizer::bufferOutput(const Pin* top_pin, LibertyCell* buffer_cell)
{
  NetworkEdit* network = networkEdit();
  Term* term = network_->term(top_pin);
  Net* output_net = network_->net(term);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  string buffer_name = makeUniqueInstName("output");
  Instance* parent = network->topInstance();
  Net* buffer_in = makeUniqueNet();
  Point pin_loc = db_network_->location(top_pin);
  Instance* buffer
      = makeBuffer(buffer_cell, buffer_name.c_str(), parent, pin_loc);
  inserted_buffer_count_++;

  NetConnectedPinIterator* pin_iter
      = network_->connectedPinIterator(output_net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (pin != top_pin) {
      // Leave output port pin connected to output_net.
      sta_->disconnectPin(const_cast<Pin*>(pin));
      Port* pin_port = network->port(pin);
      sta_->connectPin(network->instance(pin), pin_port, buffer_in);
    }
  }
  delete pin_iter;
  sta_->connectPin(buffer, input, buffer_in);
  sta_->connectPin(buffer, output, output_net);

  parasiticsInvalid(buffer_in);
  parasiticsInvalid(output_net);
}

////////////////////////////////////////////////////////////////

bool Resizer::hasPort(const Net* net)
{
  if (!net) {
    return false;
  }

  dbNet* db_net = db_network_->staToDb(net);
  return !db_net->getBTerms().empty();
}

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

LibertyCell* Resizer::halfDrivingPowerCell(Instance* inst)
{
  return halfDrivingPowerCell(network_->libertyCell(inst));
}
LibertyCell* Resizer::halfDrivingPowerCell(LibertyCell* cell)
{
  return closestDriver(cell, sta_->equivCells(cell), 0.5);
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
                                    LibertyCellSeq* candidates,
                                    float scale)
{
  LibertyCell* closest = nullptr;
  if (candidates == nullptr || candidates->empty()
      || !isSingleOutputCombinational(cell)) {
    return nullptr;
  }
  const auto output_pin = libraryOutputPins(cell)[0];
  const auto current_limit = scale * maxLoad(output_pin->cell());
  auto diff = sta::INF;
  for (auto& cand : *candidates) {
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
  LibertyCellSeq* equiv_cells = sta_->equivCells(cell);
  if (equiv_cells) {
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
    for (LibertyCell* target_cell : *equiv_cells) {
      if (!dontUse(target_cell) && isLinkCell(target_cell)) {
        float target_load = (*target_load_map_)[target_cell];
        float delay = is_buf_inv ? bufferDelay(
                          target_cell, load_cap, tgt_slew_dcalc_ap_)
                                 : 0.0;
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
  }
  return best_cell;
}

void Resizer::invalidateParasitics(const Pin* pin, const Net* net)
{
  // ODB is clueless about tristates so go to liberty for reality.
  const LibertyPort* port = network_->libertyPort(pin);
  // Invalidate estimated parasitics on all instance input pins.
  // Tristate nets have multiple drivers and this is drivers^2 if
  // the parasitics are updated for each resize.
  if (net && !port->direction()->isAnyTristate()) {
    parasiticsInvalid(net);
  }
}

void Resizer::swapPins(Instance* inst,
                       LibertyPort* port1,
                       LibertyPort* port2,
                       bool journal)
{
  // Add support for undo.
  if (journal) {
    journalSwapPins(inst, port1, port2);
  }

  Pin *found_pin1, *found_pin2;
  Net *net1, *net2;

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
    }
    if (std::strcmp(port->name(), port2->name()) == 0) {
      found_pin2 = pin;
      net2 = net;
    }
  }

  if (net1 != nullptr && net2 != nullptr) {
    // Swap the ports and nets
    sta_->disconnectPin(found_pin1);
    sta_->connectPin(inst, port1, net2);
    sta_->disconnectPin(found_pin2);
    sta_->connectPin(inst, port2, net1);

    // Invalidate the parasitics on these two nets.
    if (haveEstimatedParasitics()) {
      invalidateParasitics(found_pin2, net1);
      invalidateParasitics(found_pin1, net2);
    }
  }
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
    if (journal) {
      journalInstReplaceCellBefore(inst);
    }
    sta_->replaceCell(inst, replacement_cell1);
    designAreaIncr(area(replacement_master));

    // Legalize the position of the instance in case it leaves the die
    if (parasitics_src_ == ParasiticsSrc::global_routing) {
      opendp_->legalCellPos(db_network_->staToDb(inst));
    }
    if (haveEstimatedParasitics()) {
      InstancePinIterator* pin_iter = network_->pinIterator(inst);
      while (pin_iter->hasNext()) {
        const Pin* pin = pin_iter->next();
        const Net* net = network_->net(pin);
        invalidateParasitics(pin, net);
      }
      delete pin_iter;
    }
    return true;
  }
  return false;
}

bool Resizer::hasMultipleOutputs(const Instance* inst)
{
  int output_count = 0;
  InstancePinIterator* pin_iter = network_->pinIterator(inst);
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
void Resizer::findResizeSlacks()
{
  journalBegin();
  estimateWireParasitics();
  int repaired_net_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repair_design_->repairDesign(max_wire_length_,
                               0.0,
                               0.0,
                               false,
                               repaired_net_count,
                               slew_violations,
                               cap_violations,
                               fanout_violations,
                               length_violations);
  findResizeSlacks1();
  journalRestore(resize_count_, inserted_buffer_count_, cloned_gate_count_);
}

void Resizer::findResizeSlacks1()
{
  // Use driver pin slacks rather than Sta::netSlack to save visiting
  // the net pins and min'ing the slack.
  net_slack_map_.clear();
  NetSeq nets;
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
      nets.emplace_back(net);
    }
  }

  // Find the nets with the worst slack.

  //  sort(nets.begin(), nets.end(). [&](const Net *net1,
  sort(nets, [this](const Net* net1, const Net* net2) {
    return resizeNetSlack(net1) < resizeNetSlack(net2);
  });
  worst_slack_nets_.clear();
  for (int i = 0; i < nets.size() * worst_slack_nets_percent_ / 100.0; i++) {
    worst_slack_nets_.emplace_back(nets[i]);
  }
}

NetSeq& Resizer::resizeWorstSlackNets()
{
  return worst_slack_nets_;
}

vector<dbNet*> Resizer::resizeWorstSlackDbNets()
{
  vector<dbNet*> nets;
  for (const Net* net : worst_slack_nets_) {
    nets.emplace_back(db_network_->staToDb(net));
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

  // Reset buffer set to ensure it honors dont_use_
  buffer_cells_.clear();
  buffer_lowest_drive_ = nullptr;
}

bool Resizer::dontUse(LibertyCell* cell)
{
  return cell->dontUse() || dont_use_.hasKey(cell);
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
  return db_inst->isDoNotTouch() || db_inst->isPad();
}

void Resizer::setDontTouch(const Net* net, bool dont_touch)
{
  dbNet* db_net = db_network_->staToDb(net);
  db_net->setDoNotTouch(dont_touch);
}

bool Resizer::dontTouch(const Net* net)
{
  dbNet* db_net = db_network_->staToDb(net);
  return db_net->isDoNotTouch();
}

////////////////////////////////////////////////////////////////

// Find a target slew for the libraries and then
// a target load for each cell that gives the target slew.
void Resizer::findTargetLoads()
{
  if (target_load_map_ == nullptr) {
    // Find target slew across all buffers in the libraries.
    findBufferTargetSlews();

    target_load_map_ = new CellTargetLoadMap;
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
    TimingRole* role = arc_set->role();
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
                   arc->toEdge()->asString(),
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
        RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        RiseFall* out_rf = arc->toEdge()->asRiseFall();
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
  initBlock();
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
        Net* net = network_->net(drvr_pin);
        if (net && !dontTouch(net)) {
          NetConnectedPinIterator* pin_iter
              = network_->connectedPinIterator(net);
          while (pin_iter->hasNext()) {
            const Pin* load = pin_iter->next();
            if (load != drvr_pin) {
              // Make tie inst.
              Point tie_loc = tieLocation(load, separation_dbu);
              Instance* load_inst = network_->instance(load);
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

          // Delete inst output net.
          Pin* tie_pin = network_->findPin(inst, tie_port);
          Net* tie_net = network_->net(tie_pin);
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
              Net* net = network_->net(pin);
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
  if (!network_->isTopLevelPort(load)) {
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

////////////////////////////////////////////////////////////////

string Resizer::makeUniqueNetName()
{
  string node_name;
  Instance* top_inst = network_->topInstance();
  do {
    stringPrint(node_name, "net%d", unique_net_index_++);
  } while (network_->findNet(top_inst, node_name.c_str()));
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
    stringPrint(inst_name,
                underscore ? "%s_%d" : "%s%d",
                base_name,
                unique_inst_index_++);
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

// Create a map of all the pins that are equivalent and then use the fastest pin
// for our violating path. Current implementation does not handle the case
// where 2 paths go through the same gate (we could end up swapping pins twice)
void Resizer::findSwapPinCandidate(LibertyPort* input_port,
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
        RiseFall* in_rf = arc->fromEdge()->asRiseFall();
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
        RiseFall* in_rf = arc->fromEdge()->asRiseFall();
        int out_rf_index = arc->toEdge()->asRiseFall()->index();
        float in_slew = tgt_slews_[in_rf->index()];
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
        RiseFall* in_rf = arc->fromEdge()->asRiseFall();
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

double Resizer::findMaxWireLength()
{
  init();
  checkLibertyForAllCorners();
  findBuffers();
  findTargetLoads();
  return findMaxWireLength1();
}

double Resizer::findMaxWireLength1()
{
  std::optional<double> max_length;
  for (const Corner* corner : *sta_->corners()) {
    if (wireSignalResistance(corner) <= 0.0) {
      logger_->warn(RSZ,
                    88,
                    "Corner: {} has no wire signal resistance value.",
                    corner->name());
      continue;
    }

    // buffer_cells_ is required to be non-empty.
    for (LibertyCell* buffer_cell : buffer_cells_) {
      double buffer_length = findMaxWireLength(buffer_cell, corner);
      max_length = min(max_length.value_or(INF), buffer_length);
    }
  }

  if (!max_length.has_value()) {
    logger_->error(RSZ,
                   89,
                   "Could not find a resistance value for any corner. Cannot "
                   "evaluate max wire length for buffer. Check over your "
                   "`set_wire_rc` configuration");
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
  double drvr_r = drvr_port->driveResistance();
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  double wire_length2 = drvr_r / wireSignalResistance(corner);
  double tol = .01;  // 1%
  double diff1 = splitWireDelayDiff(wire_length2, cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2)
         > max(wire_length1, wire_length2) * tol) {
    if (diff1 < 0.0) {
      wire_length1 = wire_length2;
      wire_length2 *= 2;
      diff1 = splitWireDelayDiff(wire_length2, cell);
    } else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff2 = splitWireDelayDiff(wire_length3, cell);
      if (diff2 < 0.0) {
        wire_length1 = wire_length3;
      } else {
        wire_length2 = wire_length3;
        diff1 = diff2;
      }
    }
  }
  return wire_length1;
}

// objective function
double Resizer::splitWireDelayDiff(double wire_length, LibertyCell* buffer_cell)
{
  Delay delay1, delay2;
  Slew slew1, slew2;
  bufferWireDelay(buffer_cell, wire_length, delay1, slew1);
  bufferWireDelay(buffer_cell, wire_length / 2, delay2, slew2);
  return delay1 - delay2 * 2;
}

void Resizer::bufferWireDelay(LibertyCell* buffer_cell,
                              double wire_length,  // meters
                              // Return values.
                              Delay& delay,
                              Slew& slew)
{
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  return cellWireDelay(drvr_port, load_port, wire_length, delay, slew);
}

// Cell delay plus wire delay.
// Use target slew for input slew.
// drvr_port and load_port do not have to be the same liberty cell.
void Resizer::cellWireDelay(LibertyPort* drvr_port,
                            LibertyPort* load_port,
                            double wire_length,  // meters
                            // Return values.
                            Delay& delay,
                            Slew& slew)
{
  // Make a (hierarchical) block to use as a scratchpad.
  dbBlock* block
      = dbBlock::create(block_, "wire_delay", block_->getTech(), '/');
  std::unique_ptr<dbSta> sta = sta_->makeBlockSta(block);
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
          RiseFall* in_rf = arc->fromEdge()->asRiseFall();
          RiseFall* drvr_rf = arc->toEdge()->asRiseFall();
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

  // Cleanup the turds.
  sta->deleteInstance(drvr);
  sta->deleteInstance(load);
  sta->deleteNet(net);
  dbBlock::destroy(block);
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
  initDesignArea();
  return design_area_;
}

void Resizer::designAreaIncr(float delta)
{
  design_area_ += delta;
}

void Resizer::initDesignArea()
{
  design_area_ = 0.0;
  for (dbInst* inst : block_->getInsts()) {
    dbMaster* master = inst->getMaster();
    // Don't count fillers otherwise you'll always get 100% utilization
    if (!master->isFiller()) {
      design_area_ += area(master);
    }
  }
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
                           bool verbose)
{
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing) {
    opendp_->initMacrosAndGrid();
  }
  repair_design_->repairDesign(
      max_wire_length, slew_margin, cap_margin, verbose);
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
  repair_design_->repairClkNets(max_wire_length);
}

////////////////////////////////////////////////////////////////

// Find inverters in the clock network and clone them next to the
// each register they drive.
void Resizer::repairClkInverters()
{
  initBlock();
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

void Resizer::repairSetup(double setup_margin,
                          double repair_tns_end_percent,
                          int max_passes,
                          bool verbose,
                          bool skip_pin_swap,
                          bool skip_gate_cloning,
                          bool skip_buffer_removal)
{
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing) {
    opendp_->initMacrosAndGrid();
  }
  repair_setup_->repairSetup(setup_margin,
                             repair_tns_end_percent,
                             max_passes,
                             verbose,
                             skip_pin_swap,
                             skip_gate_cloning,
                             skip_buffer_removal);
}

void Resizer::reportSwappablePins()
{
  resizePreamble();
  repair_setup_->reportSwappablePins();
}

void Resizer::repairSetup(const Pin* end_pin)
{
  resizePreamble();
  repair_setup_->repairSetup(end_pin);
}

void Resizer::rebufferNet(const Pin* drvr_pin)
{
  resizePreamble();
  repair_setup_->rebufferNet(drvr_pin);
}

////////////////////////////////////////////////////////////////

void Resizer::repairHold(
    double setup_margin,
    double hold_margin,
    bool allow_setup_violations,
    // Max buffer count as percent of design instance count.
    float max_buffer_percent,
    int max_passes,
    bool verbose)
{
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing) {
    opendp_->initMacrosAndGrid();
  }
  repair_hold_->repairHold(setup_margin,
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
void Resizer::recoverPower(float recover_power_percent)
{
  resizePreamble();
  if (parasitics_src_ == ParasiticsSrc::global_routing) {
    opendp_->initMacrosAndGrid();
  }
  recover_power_->recoverPower(recover_power_percent);
}
////////////////////////////////////////////////////////////////
// Journal to roll back changes (OpenDB not up to the task).
void Resizer::journalBegin()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  resized_inst_map_.clear();
  inserted_buffers_.clear();
  inserted_buffer_set_.clear();
  cloned_gates_ = {};
  cloned_inst_set_.clear();
  swapped_pins_.clear();
}

void Resizer::journalEnd()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal end");
  resized_inst_map_.clear();
  inserted_buffers_.clear();
  inserted_buffer_set_.clear();
  cloned_gates_ = {};
  cloned_inst_set_.clear();
  swapped_pins_.clear();
}

void Resizer::journalSwapPins(Instance* inst,
                              LibertyPort* port1,
                              LibertyPort* port2)
{
  debugPrint(logger_,
             RSZ,
             "journal",
             1,
             "journal swap pins {} ({}->{})",
             network_->pathName(inst),
             port1->name(),
             port2->name());
  swapped_pins_[inst] = std::make_tuple(port1, port2);
  all_swapped_pin_inst_set_.insert(inst);
}

void Resizer::journalInstReplaceCellBefore(Instance* inst)
{
  LibertyCell* lib_cell = network_->libertyCell(inst);
  debugPrint(logger_,
             RSZ,
             "journal",
             1,
             "journal replace {} ({})",
             network_->pathName(inst),
             lib_cell->name());
  // Do not clobber an existing checkpoint cell.
  if (!resized_inst_map_.hasKey(inst)) {
    resized_inst_map_[inst] = lib_cell;
    all_sized_inst_set_.insert(inst);
  }
}

void Resizer::journalMakeBuffer(Instance* buffer)
{
  debugPrint(logger_,
             RSZ,
             "journal",
             1,
             "journal make_buffer {}",
             network_->pathName(buffer));
  inserted_buffers_.emplace_back(buffer);
  inserted_buffer_set_.insert(buffer);
  all_inserted_buffer_set_.insert(buffer);
}

Instance* Resizer::journalCloneInstance(LibertyCell* cell,
                                        const char* name,
                                        Instance* original_inst,
                                        Instance* parent,
                                        const Point& loc)
{
  Instance* clone_inst = makeInstance(cell, name, parent, loc);
  cloned_gates_.emplace(original_inst, clone_inst);
  cloned_inst_set_.insert(clone_inst);
  all_cloned_inst_set_.insert(clone_inst);
  all_cloned_inst_set_.insert(original_inst);
  return clone_inst;
}

void Resizer::journalUndoGateCloning(int& cloned_gate_count)
{
  // Undo gate cloning
  while (!cloned_gates_.empty()) {
    auto element = cloned_gates_.top();
    cloned_gates_.pop();
    auto original_inst = std::get<0>(element);
    auto cloned_inst = std::get<1>(element);
    debugPrint(logger_,
               RSZ,
               "journal",
               1,
               "journal unclone {} ({}) -> {} ({})",
               network_->pathName(original_inst),
               network_->libertyCell(original_inst)->name(),
               network_->pathName(cloned_inst),
               network_->libertyCell(cloned_inst)->name());

    const Pin* original_output_pin = nullptr;
    PinVector original_pins;
    getPins(original_inst, original_pins);
    for (auto& pin : original_pins) {
      if (network_->direction(pin)->isOutput()) {
        original_output_pin = pin;
        break;
      }
    }
    Net* original_out_net = network_->net(original_output_pin);
    Net* clone_out_net = nullptr;
    //=========================================================================
    // Go through the cloned instance, disconnect pins
    PinVector clone_pins;
    getPins(cloned_inst, clone_pins);
    for (auto& pin : clone_pins) {
      // Disconnect the current instance pins. Also store the output net
      if (network_->direction(pin)->isOutput()) {
        clone_out_net = network_->net(pin);
      }
      sta_->disconnectPin(const_cast<Pin*>(pin));
    }
    //=========================================================================
    // Go through the cloned output net, disconnect pins, connect pins to the
    // original output net
    clone_pins.clear();
    getPins(clone_out_net, clone_pins);
    for (auto& pin : clone_pins) {
      if (network_->direction(pin)->isOutput()) {
        // We should never get here.
        logger_->error(RSZ, 23, "Output pin found when none was expected.");
      } else if (network_->direction(pin)->isInput()) {
        // Connect them to the original nets if they are inputs
        Instance* inst = network_->instance(pin);
        auto term_port = network_->port(pin);
        sta_->disconnectPin(const_cast<Pin*>(pin));
        sta_->connectPin(inst, term_port, original_out_net);
      }
    }
    //=========================================================================
    // Final cleanup
    if (clone_out_net != nullptr) {
      sta_->deleteNet(clone_out_net);
    }
    sta_->deleteInstance(cloned_inst);
    sta_->graphDelayCalc()->delaysInvalid();
    --cloned_gate_count;
  }
  cloned_inst_set_.clear();
}

void Resizer::journalRestore(int& resize_count,
                             int& inserted_buffer_count,
                             int& cloned_gate_count)
{
  for (auto [inst, lib_cell] : resized_inst_map_) {
    if (!inserted_buffer_set_.hasKey(inst)) {
      debugPrint(logger_,
                 RSZ,
                 "journal",
                 1,
                 "journal restore {} ({})",
                 network_->pathName(inst),
                 lib_cell->name());
      // skip if it is a cloned cell
      if (cloned_inst_set_.find(inst) != cloned_inst_set_.end()) {
        debugPrint(logger_,
                   RSZ,
                   "journal",
                   1,
                   "journal skip cloned {} ({})",
                   network_->pathName(inst),
                   lib_cell->name());
        continue;
      }
      debugPrint(logger_,
                 RSZ,
                 "journal",
                 1,
                 "journal replace {} ({})",
                 network_->pathName(inst),
                 lib_cell->name());
      replaceCell(inst, lib_cell, false);
      resize_count--;
    }
  }
  inserted_buffer_set_.clear();

  while (!inserted_buffers_.empty()) {
    const Instance* buffer = inserted_buffers_.back();
    debugPrint(logger_,
               RSZ,
               "journal",
               1,
               "journal remove buffer {}",
               network_->pathName(buffer));
    removeBuffer(const_cast<Instance*>(buffer));
    inserted_buffers_.pop_back();
    inserted_buffer_count--;
  }

  // Undo pin swaps
  for (const auto& element : swapped_pins_) {
    Instance* inst = element.first;
    LibertyPort* port1 = std::get<0>(element.second);
    LibertyPort* port2 = std::get<1>(element.second);
    debugPrint(logger_,
               RSZ,
               "journal",
               1,
               "journal unswap pins {} ({}<-{})",
               network_->pathName(inst),
               port1->name(),
               port2->name());
    swapPins(inst, port1, port2, false);
  }
  swapped_pins_.clear();

  journalUndoGateCloning(cloned_gate_count);
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
  if (parasitics_src_ == ParasiticsSrc::global_routing) {
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
  // umich brain damage control
  if (!exists || limit == 0.0) {
    limit = INF;
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

}  // namespace rsz
