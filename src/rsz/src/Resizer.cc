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

#include "rsz/SteinerTree.hh"

#include "gui/gui.h"
#include "utl/Logger.h"

#include "sta/Report.hh"
#include "sta/FuncExpr.hh"
#include "sta/PortDirection.hh"
#include "sta/TimingRole.hh"
#include "sta/Units.hh"
#include "sta/Liberty.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/Network.hh"
#include "sta/Graph.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Parasitics.hh"
#include "sta/Sdc.hh"
#include "sta/InputDrive.hh"
#include "sta/Corner.hh"
#include "sta/PathVertex.hh"
#include "sta/SearchPred.hh"
#include "sta/Bfs.hh"
#include "sta/Search.hh"
#include "sta/PathRef.hh"
#include "sta/PathExpanded.hh"
#include "sta/StaMain.hh"
#include "sta/Fuzzy.hh"

// http://vlsicad.eecs.umich.edu/BK/Slots/cache/dropzone.tamu.edu/~zhuoli/GSRC/fast_buffer_insertion.html

namespace sta {
extern const char *rsz_tcl_inits[];
}

namespace rsz {

using std::abs;
using std::min;
using std::max;
using std::string;
using std::to_string;
using std::vector;
using std::map;
using std::pair;
using std::sqrt;

using utl::RSZ;

using odb::dbInst;
using odb::dbPlacementStatus;
using odb::Rect;
using odb::dbOrientType;
using odb::dbMPin;
using odb::dbBox;
using odb::dbMasterType;

using sta::evalTclInit;
using sta::makeBlockSta;
using sta::Level;
using sta::stringLess;
using sta::Network;
using sta::NetworkEdit;
using sta::NetPinIterator;
using sta::NetConnectedPinIterator;
using sta::InstancePinIterator;
using sta::LeafInstanceIterator;
using sta::LibertyLibraryIterator;
using sta::LibertyCellIterator;
using sta::LibertyCellTimingArcSetIterator;
using sta::TimingArcSet;
using sta::TimingArcSetArcIterator;
using sta::TimingArcSetSeq;
using sta::GateTimingModel;
using sta::TimingRole;
using sta::FuncExpr;
using sta::Term;
using sta::Port;
using sta::PinSeq;
using sta::NetIterator;
using sta::PinConnectedPinIterator;
using sta::FindNetDrvrLoads;;
using sta::VertexIterator;
using sta::VertexOutEdgeIterator;
using sta::Edge;
using sta::Search;
using sta::SearchPredNonReg2;
using sta::ClkArrivalSearchPred;
using sta::BfsBkwdIterator;
using sta::BfsFwdIterator;
using sta::BfsIndex;
using sta::Clock;
using sta::PathExpanded;
using sta::INF;
using sta::fuzzyEqual;
using sta::fuzzyLess;
using sta::fuzzyLessEqual;
using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::delayInf;
using sta::stringPrint;
using sta::Unit;
using sta::ArcDelayCalc;
using sta::Corners;
using sta::InputDrive;

extern "C" {
extern int Rsz_Init(Tcl_Interp *interp);
}

Resizer::Resizer() :
  StaState(),
  wire_signal_res_(0.0),
  wire_signal_cap_(0.0),
  wire_clk_res_(0.0),
  wire_clk_cap_(0.0),
  max_area_(0.0),
  openroad_(nullptr),
  logger_(nullptr),
  gui_(nullptr),
  sta_(nullptr),
  db_network_(nullptr),
  db_(nullptr),
  block_(nullptr),
  core_exists_(false),
  parasitics_src_(ParasiticsSrc::none),
  design_area_(0.0),
  max_(MinMax::max()),
  buffer_lowest_drive_(nullptr),
  buffer_med_drive_(nullptr),
  buffer_highest_drive_(nullptr),
  target_load_map_(nullptr),
  level_drvr_vertices_valid_(false),
  tgt_slews_{0.0, 0.0},
  tgt_slew_corner_(nullptr),
  tgt_slew_dcalc_ap_(nullptr),
  unique_net_index_(1),
  unique_inst_index_(1),
  resize_count_(0),
  inserted_buffer_count_(0),
  max_wire_length_(0),
  steiner_renderer_(nullptr),
  rebuffer_net_count_(0)
{
}

void
Resizer::init(OpenRoad *openroad,
              Tcl_Interp *interp,
              Logger *logger,
              Gui *gui,
              dbDatabase *db,
              dbSta *sta,
              SteinerTreeBuilder *stt_builder,
              GlobalRouter *global_router)
{
  openroad_ = openroad;
  logger_ = logger;
  gui_ = gui;
  db_ = db;
  block_ = nullptr;
  sta_ = sta;
  stt_builder_ = stt_builder;
  global_router_ = global_router;
  incr_groute_ = nullptr;
  db_network_ = sta->getDbNetwork();
  copyState(sta);
  // Define swig TCL commands.
  Rsz_Init(interp);
  // Eval encoded sta TCL sources.
  evalTclInit(interp, sta::rsz_tcl_inits);
}

////////////////////////////////////////////////////////////////

double
Resizer::coreArea() const
{
  return dbuToMeters(core_.dx()) * dbuToMeters(core_.dy());
}

double
Resizer::utilization()
{
  ensureBlock();
  ensureDesignArea();
  double core_area = coreArea();
  if (core_area > 0.0)
    return design_area_ / core_area;
  else
    return 1.0;
}

double
Resizer::maxArea() const
{
  return max_area_;
}

////////////////////////////////////////////////////////////////

class VertexLevelLess
{
public:
  VertexLevelLess(const Network *network);
  bool operator()(const Vertex *vertex1,
                  const Vertex *vertex2) const;

protected:
  const Network *network_;
};

VertexLevelLess::VertexLevelLess(const Network *network) :
  network_(network)
{
}

bool
VertexLevelLess::operator()(const Vertex *vertex1,
                            const Vertex *vertex2) const
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
void
Resizer::ensureBlock()
{
  // block_ indicates core_, design_area_
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
    block_->getCoreArea(core_);
    core_exists_ = !(core_.xMin() == 0
                     && core_.xMax() == 0
                     && core_.yMin() == 0
                     && core_.yMax() == 0);
  }
}

void
Resizer::init()
{
  // Abbreviated copyState
  db_network_ = sta_->getDbNetwork();
  sta_->ensureLevelized();
  graph_ = sta_->graph();
  ensureBlock();
  ensureDesignArea();
  ensureLevelDrvrVertices();
  sta_->ensureClkNetwork();
}

void
Resizer::removeBuffers()
{
  ensureBlock();
  db_network_ = sta_->getDbNetwork();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  int remove_count = 0;
  for (dbInst *inst : block_->getInsts()) {
    LibertyCell *lib_cell = db_network_->libertyCell(inst);
    if (lib_cell && lib_cell->isBuffer()) {
      Instance *buffer = db_network_->dbToSta(inst);
      // Do not remove buffers connected to input/output ports
      // because verilog netlists use the net name for the port.
      if (!bufferBetweenPorts(buffer)) {
        removeBuffer(buffer);
        remove_count++;
      }
    }
  }
  level_drvr_vertices_valid_ = false;
  logger_->info(RSZ, 26, "Removed {} buffers.", remove_count);
}

bool
Resizer::bufferBetweenPorts(Instance *buffer)
{
  LibertyCell *lib_cell = network_->libertyCell(buffer);
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin *in_pin = db_network_->findPin(buffer, in_port);
  Pin *out_pin = db_network_->findPin(buffer, out_port);
  Net *in_net = db_network_->net(in_pin);
  Net *out_net = db_network_->net(out_pin);
  bool in_net_ports = hasPort(in_net);
  bool out_net_ports = hasPort(out_net);
  return in_net_ports && out_net_ports;
}

void
Resizer::removeBuffer(Instance *buffer)
{
  LibertyCell *lib_cell = network_->libertyCell(buffer);
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin *in_pin = db_network_->findPin(buffer, in_port);
  Pin *out_pin = db_network_->findPin(buffer, out_port);
  Net *in_net = db_network_->net(in_pin);
  Net *out_net = db_network_->net(out_pin);
  bool out_net_ports = hasPort(out_net);
  Net *survivor, *removed;
  if (out_net_ports) {
    survivor = out_net;
    removed = in_net;
  }
  else {
    // default or out_net_ports
    // Default to in_net surviving so drivers (cached in dbNetwork)
    // do not change.
    survivor = in_net;
    removed = out_net;
  }

  if (!sdc_->isConstrained(in_pin)
      && !sdc_->isConstrained(out_pin)
      && !sdc_->isConstrained(removed)
      && !sdc_->isConstrained(buffer)) {
    sta_->disconnectPin(in_pin);
    sta_->disconnectPin(out_pin);
    sta_->deleteInstance(buffer);

    NetPinIterator *pin_iter = db_network_->pinIterator(removed);
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      Instance *pin_inst = db_network_->instance(pin);
      if (pin_inst != buffer) {
        Port *pin_port = db_network_->port(pin);
        sta_->disconnectPin(pin);
        sta_->connectPin(pin_inst, pin_port, survivor);
      }
    }
    delete pin_iter;
    sta_->deleteNet(removed);
    parasitics_invalid_.erase(removed);
  }
}

void
Resizer::ensureLevelDrvrVertices()
{
  if (!level_drvr_vertices_valid_) {
    level_drvr_vertices_.clear();
    VertexIterator vertex_iter(graph_);
    while (vertex_iter.hasNext()) {
      Vertex *vertex = vertex_iter.next();
      if (vertex->isDriver(network_))
        level_drvr_vertices_.push_back(vertex);
    }
    sort(level_drvr_vertices_, VertexLevelLess(network_));
    level_drvr_vertices_valid_ = true;
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::resizePreamble()
{
  init();
  makeEquivCells();
  findBuffers();
  findTargetLoads();
}

static float
bufferDrive(const LibertyCell *buffer)
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  return output->driveResistance();
}

void
Resizer::findBuffers()
{
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary *lib = lib_iter->next();
    for (LibertyCell *buffer : *lib->buffers()) {
      if (!dontUse(buffer)
          && isLinkCell(buffer)) {
        buffer_cells_.push_back(buffer);
      }
    }
  }
  delete lib_iter;

  if (buffer_cells_.empty())
    logger_->error(RSZ, 22, "no buffers found.");

  sort(buffer_cells_, [] (const LibertyCell *buffer1,
                          const LibertyCell *buffer2) {
                        return bufferDrive(buffer1) > bufferDrive(buffer2);
                      });
  buffer_lowest_drive_ = buffer_cells_[0];
  buffer_med_drive_ = buffer_cells_[buffer_cells_.size() / 2];
  buffer_highest_drive_ = buffer_cells_[buffer_cells_.size() - 1];
}

bool
Resizer::isLinkCell(LibertyCell *cell)
{
  return network_->findLibertyCell(cell->name()) == cell;
}

////////////////////////////////////////////////////////////////

void
Resizer::bufferInputs()
{
  init();
  inserted_buffer_count_ = 0;
  incrementalParasiticsBegin();
  InstancePinIterator *port_iter = network_->pinIterator(network_->topInstance());
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Vertex *vertex = graph_->pinDrvrVertex(pin);
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isInput()
        && !vertex->isConstant()
        && !sta_->isClock(pin)
        // Hands off special nets.
        && !db_network_->isSpecial(net)
        && hasPins(net))
      // repair_design will resize to target slew.
      bufferInput(pin, buffer_lowest_drive_);
  }
  delete port_iter;
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 27, "Inserted {} input buffers.", inserted_buffer_count_);
    level_drvr_vertices_valid_ = false;
  }
}
   
bool
Resizer::hasPins(Net *net)
{
  NetPinIterator *pin_iter = db_network_->pinIterator(net);
  bool has_pins = pin_iter->hasNext();
  delete pin_iter;
  return has_pins;
}

Instance *
Resizer::bufferInput(const Pin *top_pin,
                     LibertyCell *buffer_cell)
{
  Term *term = db_network_->term(top_pin);
  Net *input_net = db_network_->net(term);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  string buffer_name = makeUniqueInstName("input");
  Instance *parent = db_network_->topInstance();
  Net *buffer_out = makeUniqueNet();
  Instance *buffer = makeInstance(buffer_cell,
                                  buffer_name.c_str(),
                                  parent);
  if (buffer) {
    journalMakeBuffer(buffer);
    Point pin_loc = db_network_->location(top_pin);
    Point buf_loc = core_exists_ ? core_.closestPtInside(pin_loc) : pin_loc;
    setLocation(buffer, buf_loc);
    designAreaIncr(area(db_network_->cell(buffer_cell)));
    inserted_buffer_count_++;

    NetPinIterator *pin_iter = db_network_->pinIterator(input_net);
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      // Leave input port pin connected to input_net.
      if (pin != top_pin) {
        sta_->disconnectPin(pin);
        Port *pin_port = db_network_->port(pin);
        sta_->connectPin(db_network_->instance(pin), pin_port, buffer_out);
      }
    }
    delete pin_iter;
    sta_->connectPin(buffer, input, input_net);
    sta_->connectPin(buffer, output, buffer_out);

    parasiticsInvalid(input_net);
    parasiticsInvalid(buffer_out);
  }
  return buffer;
}

void
Resizer::setLocation(Instance *inst,
                     Point pt)
{
  // Stay inside the lines.
  if (core_exists_)
    pt = core_.closestPtInside(pt);

  dbInst *dinst = db_network_->staToDb(inst);
  dinst->setPlacementStatus(dbPlacementStatus::PLACED);
  dinst->setLocation(pt.getX(), pt.getY());
}

void
Resizer::bufferOutputs()
{
  init();
  inserted_buffer_count_ = 0;
  incrementalParasiticsBegin();
  InstancePinIterator *port_iter = network_->pinIterator(network_->topInstance());
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Vertex *vertex = graph_->pinLoadVertex(pin);
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isOutput()
        && net
        // DEF does not have tristate output types so we have look at the drivers.
        && !hasTristateDriver(net)
        && !vertex->isConstant()
        // Hands off special nets.
        && !db_network_->isSpecial(net)
        && hasPins(net))
      bufferOutput(pin, buffer_lowest_drive_);
  }
  delete port_iter;
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 28, "Inserted {} output buffers.", inserted_buffer_count_);
    level_drvr_vertices_valid_ = false;
  }
}

bool
Resizer::hasTristateDriver(const Net *net)
{
  PinSet *drivers = network_->drivers(net);
  if (drivers) {
    for (Pin *pin : *drivers) {
      if (isTristateDriver(pin))
        return true;
    }
  }
  return false;
}

bool
Resizer::isTristateDriver(const Pin *pin)
{
  // Note LEF macro PINs do not have a clue about tristates.
  LibertyPort *port = network_->libertyPort(pin);
  return port && port->direction()->isAnyTristate();
}

void
Resizer::bufferOutput(Pin *top_pin,
                      LibertyCell *buffer_cell)
{
  NetworkEdit *network = networkEdit();
  Term *term = network_->term(top_pin);
  Net *output_net = network_->net(term);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  string buffer_name = makeUniqueInstName("output");
  Instance *parent = network->topInstance();
  Net *buffer_in = makeUniqueNet();
  Instance *buffer = makeInstance(buffer_cell,
                                  buffer_name.c_str(),
                                  parent);
  if (buffer) {
    journalMakeBuffer(buffer);
    setLocation(buffer, db_network_->location(top_pin));
    designAreaIncr(area(db_network_->cell(buffer_cell)));
    inserted_buffer_count_++;

    NetPinIterator *pin_iter = network->pinIterator(output_net);
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      if (pin != top_pin) {
        // Leave output port pin connected to output_net.
        sta_->disconnectPin(pin);
        Port *pin_port = network->port(pin);
        sta_->connectPin(network->instance(pin), pin_port, buffer_in);
      }
    }
    delete pin_iter;
    sta_->connectPin(buffer, input, buffer_in);
    sta_->connectPin(buffer, output, output_net);

    parasiticsInvalid(buffer_in);
    parasiticsInvalid(output_net);
  }
}

////////////////////////////////////////////////////////////////

// Repair long wires, max slew, max capacitance, max fanout violations
// The whole enchilada.
// max_wire_length zero for none (meters)
void
Resizer::repairDesign(double max_wire_length,
                      double slew_margin,
                      double max_cap_margin)
{
  int repair_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repairDesign(max_wire_length, slew_margin, max_cap_margin,
               repair_count, slew_violations, cap_violations,
               fanout_violations, length_violations);

  if (slew_violations > 0)
    logger_->info(RSZ, 34, "Found {} slew violations.", slew_violations);
  if (fanout_violations > 0)
    logger_->info(RSZ, 35, "Found {} fanout violations.", fanout_violations);
  if (cap_violations > 0)
    logger_->info(RSZ, 36, "Found {} capacitance violations.", cap_violations);
  if (length_violations > 0)
    logger_->info(RSZ, 37, "Found {} long wires.", length_violations);
  if (inserted_buffer_count_ > 0)
    logger_->info(RSZ, 38, "Inserted {} buffers in {} nets.",
                  inserted_buffer_count_,
                  repair_count);
  if (resize_count_ > 0)
    logger_->info(RSZ, 39, "Resized {} instances.", resize_count_);
}

void
Resizer::repairDesign(double max_wire_length, // zero for none (meters)
                      double slew_margin,
                      double max_cap_margin,
                      int &repair_count,
                      int &slew_violations,
                      int &cap_violations,
                      int &fanout_violations,
                      int &length_violations)
{
  repair_count = 0;
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
    if (net
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells and supply nets.
        && !drvr->isConstant())
      repairNet(net, drvr_pin, drvr, slew_margin, max_cap_margin,
                true, true, true, max_length, true,
                repair_count, slew_violations, cap_violations,
                fanout_violations, length_violations);
  }
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0)
    level_drvr_vertices_valid_ = false;
}

// repairDesign but restricted to clock network and
// no max_fanout/max_cap checks.
void
Resizer::repairClkNets(double max_wire_length) // max_wire_length zero for none (meters)
{
  init();
  // Need slews to resize inserted buffers.
  sta_->findDelays();

  inserted_buffer_count_ = 0;
  resize_count_ = 0;

  int repair_count = 0;
  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int max_length = metersToDbu(max_wire_length);
  incrementalParasiticsBegin();
  for (Clock *clk : sdc_->clks()) {
    for (const Pin *clk_pin : *sta_->pins(clk)) {
      Net *net = network_->isTopLevelPort(clk_pin)
        ? network_->net(network_->term(clk_pin))
        : network_->net(clk_pin);
      if (network_->isDriver(clk_pin)) {
        Vertex *drvr = graph_->pinDrvrVertex(clk_pin);
        // Do not resize clock tree gates.
        repairNet(net, clk_pin, drvr, 0.0, 0.0,
                  false, false, false, max_length, false,
                  repair_count, slew_violations, cap_violations,
                  fanout_violations, length_violations);
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
                  repair_count);
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
  int repair_count = 0;
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
              repair_count, slew_violations, cap_violations,
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
                  repair_count);
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
                   int &repair_count,
                   int &slew_violations,
                   int &cap_violations,
                   int &fanout_violations,
                   int &length_violations)
{
  // Hands off special nets.
  if (!db_network_->isSpecial(net)) {
    SteinerTree *tree = makeSteinerTree(drvr_pin, true, max_steiner_pin_count_,
                                        stt_builder_, db_network_, logger_);
    if (tree) {
      debugPrint(logger_, RSZ, "repair_net", 1, "repair net {}",
                 sdc_network_->pathName(drvr_pin));
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
        int wire_length = findMaxSteinerDist(tree);
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
              && max_slew1 > 0.0 && drvr_port) {
            float max_cap1 = findSlewLoadCap(drvr_port, max_slew1, corner1);
            max_cap = min(max_cap, max_cap1);
            corner = corner1;
            debugPrint(logger_, RSZ, "repair_net", 2, "drvr_slew={} max_slew={} max_cap={} corner={}",
                       delayAsString(slew1, this, 3),
                       delayAsString(max_slew1, this, 3),
                       units_->capacitanceUnit()->asString(max_cap1, 3),
                       corner1->name());
            if (slew_slack1 < 0.0)
              repair_slew = true;
          }
          if (slew_slack1 < 0.0)
            slew_violations++;

          // Check slew at the loads.
          // Note that many liberty libraries do not have max_transition attributes on
          // input pins.
          // Max slew violations at the load pins are repaired by reducing the
          // wire length.
          checkLoadSlews(drvr_pin, slew_margin, slew1, max_slew1, slew_slack1, corner1);
          // Even when there are no load violations we need max_load_slew for
          // sizing inserted buffers.
          if (max_slew1 > 0.0) {
            max_load_slew = max_slew1;
            debugPrint(logger_, RSZ, "repair_net", 2, "load_slew={} max_load_slew={}",
                       delayAsString(slew1, this, 3),
                       delayAsString(max_load_slew, this, 3));
            if (slew_slack1 < 0.0) {
              // Don't double count violations on the same net.
              if (!repair_slew)
                slew_violations++;
              corner = corner1;
              repair_slew = true;
            }
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
          SteinerPt drvr_pt = tree->drvrPt(network_);
          int wire_length;
          float pin_cap, fanout;
          PinSeq load_pins;
          if (drvr_pt != SteinerTree::null_pt)
            repairNet(tree, drvr_pt, SteinerTree::null_pt, net, drvr_pin,
                      max_load_slew, max_cap, max_fanout, max_length, corner, 0,
                      wire_length, pin_cap, fanout, load_pins);
          repair_count++;

          if (resize_drvr)
            resizeToTargetSlew(drvr_pin, true);
        }
      }
      delete tree;
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

// Find the output port load capacitance that results in slew.
double
Resizer::findSlewLoadCap(LibertyPort *drvr_port,
                         double slew,
                         const Corner *corner)
{
  const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
  // cap1 lower bound
  // cap2 upper bound
  double cap1 = 0.0;
  double cap2 = slew / drvr_port->driveResistance() * 2;
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
Resizer::repairNet(SteinerTree *tree,
                   SteinerPt pt,
                   SteinerPt prev_pt,
                   Net *net,
                   const Pin *drvr_pin,
                   float max_load_slew,
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
                   PinSeq &load_pins)
{
  Point pt_loc = tree->location(pt);
  int pt_x = pt_loc.getX();
  int pt_y = pt_loc.getY();
  debugPrint(logger_, RSZ, "repair_net", 2, "{:{}s}pt ({} {})",
             "", level,
             units_->distanceUnit()->asString(dbuToMeters(pt_x), 1),
             units_->distanceUnit()->asString(dbuToMeters(pt_y), 1));
  double wire_cap = wireSignalCapacitance(corner);
  double wire_res = wireSignalResistance(corner);
  SteinerPt left = tree->left(pt);
  int wire_length_left = 0;
  float pin_cap_left = 0.0;
  float fanout_left = 0.0;
  PinSeq loads_left;
  if (left != SteinerTree::null_pt)
    repairNet(tree, left, pt, net, drvr_pin, max_load_slew, max_cap, max_fanout, max_length,
              corner, level+1,
              wire_length_left, pin_cap_left, fanout_left, loads_left);
  SteinerPt right = tree->right(pt);
  int wire_length_right = 0;
  float pin_cap_right = 0.0;
  float fanout_right = 0.0;
  PinSeq loads_right;
  if (right != SteinerTree::null_pt)
    repairNet(tree, right, pt, net, drvr_pin, max_load_slew, max_cap, max_fanout, max_length,
              corner, level+1,
              wire_length_right, pin_cap_right, fanout_right, loads_right);
  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}left l={} pin_cap={} fanout={}, right l={} pin_cap={} fanout={}",
             "", level,
             units_->distanceUnit()->asString(dbuToMeters(wire_length_left), 1),
             units_->capacitanceUnit()->asString(pin_cap_left, 3),
             fanout_left,
             units_->distanceUnit()->asString(dbuToMeters(wire_length_right), 1),
             units_->capacitanceUnit()->asString(pin_cap_right, 3),
             fanout_right);
  // Add a buffer to left or right branch to stay under the max cap/length/fanout.
  bool repeater_left = false;
  bool repeater_right = false;
  double cap_left = pin_cap_left + dbuToMeters(wire_length_left) * wire_cap;
  double cap_right = pin_cap_right + dbuToMeters(wire_length_right) * wire_cap;
  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}cap_left={}, right_cap={}",
             "", level,
             units_->capacitanceUnit()->asString(cap_left, 3),
             units_->capacitanceUnit()->asString(cap_right, 3));

  double wire_length1 = dbuToMeters(wire_length_left + wire_length_right);
  float load_cap = cap_left + cap_right;

  LibertyCell *buffer_cell = findTargetCell(buffer_lowest_drive_, load_cap, false);
  LibertyPort *input, *buffer_output;
  buffer_cell->bufferPorts(input, buffer_output);
  float r_buffer = buffer_output->driveResistance();
  float r_drv = driveResistance(drvr_pin);
  float r_drvr = max(r_drv, r_buffer);
  // Elmore factor for 20-80% slew thresholds.
  float k_threshold = 1.39;
  Slew load_slew = (r_drvr + wire_length1 * wire_res) * load_cap * k_threshold;
  debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}load_slew={} r_drvr={} r_buffer={}",
             "", level,
             delayAsString(load_slew, this, 3),
             units_->resistanceUnit()->asString(r_drv, 3),
             units_->resistanceUnit()->asString(r_buffer, 3));

  bool slew_violation = load_slew > max_load_slew;
  if (slew_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}slew violation", "", level);
    if (cap_left > cap_right)
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
    makeRepeater("left", tree, pt, buffer_cell, level,
                 wire_length_left, pin_cap_left, fanout_left, loads_left);
  if (repeater_right)
    makeRepeater("right", tree, pt, buffer_cell, level,
                 wire_length_right, pin_cap_right, fanout_right, loads_right);

  // Update after left/right repeaters are inserted.
  wire_length = wire_length_left + wire_length_right;
  pin_cap = pin_cap_left + pin_cap_right;
  fanout = fanout_left + fanout_right;

  // Union left/right load pins.
  for (Pin *load_pin : loads_left)
    load_pins.push_back(load_pin);
  for (Pin *load_pin : loads_right)
    load_pins.push_back(load_pin);

  // Steiner pt pin is the net driver if prev_pt is null.
  if (prev_pt != SteinerTree::null_pt) {
    const PinSeq *pt_pins = tree->pins(pt);
    if (pt_pins) {
      for (Pin *load_pin : *pt_pins) {
        Point load_loc = db_network_->location(load_pin);
        int load_dist = Point::manhattanDistance(load_loc, pt_loc);
        debugPrint(logger_, RSZ, "repair_net", 2, "{:{}s}load {} ({} {}) dist={}",
                   "", level,
                   sdc_network_->pathName(load_pin),
                   units_->distanceUnit()->asString(dbuToMeters(load_loc.getX()), 1),
                   units_->distanceUnit()->asString(dbuToMeters(load_loc.getY()), 1),
                   units_->distanceUnit()->asString(dbuToMeters(load_dist), 1));
        LibertyPort *load_port = network_->libertyPort(load_pin);
        if (load_port) {
          pin_cap += load_port->capacitance();
          fanout += portFanoutLoad(load_port);
        }
        else
          fanout += 1;
        load_pins.push_back(load_pin);
      }
    }

    Point prev_loc = tree->location(prev_pt);
    int length = Point::manhattanDistance(prev_loc, pt_loc);
    wire_length += length;
    // Back up from pt to prev_pt adding repeaters every max_length.
    int prev_x = prev_loc.getX();
    int prev_y = prev_loc.getY();
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}wl={} l={}",
               "", level,
               units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
               units_->distanceUnit()->asString(dbuToMeters(length), 1));
    wire_length1 = dbuToMeters(wire_length);
    load_cap = pin_cap + wire_length1 * wire_cap;

    buffer_cell = findTargetCell(buffer_lowest_drive_, load_cap, false);
    buffer_cell->bufferPorts(input, buffer_output);
    r_buffer = buffer_output->driveResistance();
    r_drv = driveResistance(drvr_pin);
    r_drvr = max(r_drv, r_buffer);
    load_slew = (r_drvr + wire_length1 * wire_res) * load_cap * k_threshold;
    debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}load_slew={} r_drvr={} r_buffer={}",
               "", level,
               delayAsString(load_slew, this, 3),
               units_->resistanceUnit()->asString(r_drv, 3),
               units_->resistanceUnit()->asString(r_buffer, 3));

    while ((max_length > 0 && wire_length > max_length)
           || (wire_cap > 0.0
               // Cannot fix max cap violations from pin cap by shortening wire.
               && pin_cap < max_cap
               && load_cap > max_cap)
           || load_slew > max_load_slew) {
      // Make the wire a bit shorter than necessary to allow for
      // offset from instance origin to pin and detailed placement movement.
      static double length_margin = .05;
      bool split_wire = false;
      int split_length = std::numeric_limits<int>::max();
      if (max_length > 0 && wire_length > max_length) {
        split_length = min(split_length, max_length);
        split_wire = true;
      }
      if (wire_cap > 0.0
          && pin_cap < max_cap
          && load_cap > max_cap) {
        split_length = min(split_length, metersToDbu((max_cap - pin_cap) / wire_cap));
        split_wire = true;
      }
      if (load_slew > max_load_slew
          // Check that zero length wire meets max slew.
          && r_drvr*pin_cap*k_threshold < max_load_slew) {
        // Using elmore delay to approximate wire
        // load_slew = (Rdrvr + L*Rwire) * (L*Cwire + Cpin) * k_threshold
        // Setting this to max_slew is a quadratic in L
        // L^2*Rwire*Cwire + L*(Rdrvr*Cwire + Rwire*Cpin) + Rdrvr*Cpin - max_slew/k_threshold
        // Solve using quadradic eqn for L.
        float a = wire_res * wire_cap;
        float b = r_drvr * wire_cap + wire_res * pin_cap;
        float c = r_drvr * pin_cap - max_load_slew / k_threshold;
        float l = (-b + sqrt(b*b - 4 * a * c)) / (2 * a);
        if (l > 0.0) {
          split_length = min(split_length, metersToDbu(l));
          split_wire = true;
        }
      }
      if (split_wire) {
        // Distance from pt to repeater backward toward prev_pt.
        double buf_dist = length - (wire_length - split_length * (1.0 - length_margin));
        double dx = prev_x - pt_x;
        double dy = prev_y - pt_y;
        double d = (length == 0) ? 0.0 : buf_dist / length;
        int buf_x = pt_x + d * dx;
        int buf_y = pt_y + d * dy;
        makeRepeater("wire", buf_x, buf_y, buffer_lowest_drive_, level,
                     wire_length, pin_cap, fanout, load_pins);
        // Update for the next round.
        length -= buf_dist;
        wire_length = length;
        pt_x = buf_x;
        pt_y = buf_y;

        wire_length1 = dbuToMeters(wire_length);
        load_cap = pin_cap + wire_length1 * wire_cap;
        load_slew = (r_drvr + wire_length1 * wire_res) * load_cap * k_threshold;
        debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}load_slew={}",
                   "", level,
                   delayAsString(load_slew, this, 3));
        debugPrint(logger_, RSZ, "repair_net", 3, "{:{}s}wl={} l={}",
                   "", level,
                   units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
                   units_->distanceUnit()->asString(dbuToMeters(length), 1));
      }
      else
        break;
    }
  }
}

void
Resizer::makeRepeater(const char *where,
                      SteinerTree *tree,
                      SteinerPt pt,
                      LibertyCell *buffer_cell,
                      int level,
                      int &wire_length,
                      float &pin_cap,
                      float &fanout,
                      PinSeq &load_pins)
{
  Point pt_loc = tree->location(pt);
  makeRepeater(where, pt_loc.getX(), pt_loc.getY(), buffer_cell, level,
               wire_length, pin_cap, fanout, load_pins);
}

void
Resizer::makeRepeater(const char *where,
                      int x,
                      int y,
                      LibertyCell *buffer_cell,
                      int level,
                      int &wire_length,
                      float &pin_cap,
                      float &fanout,
                      PinSeq &load_pins)
{
  LibertyPort *buffer_input_port, *buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

  string buffer_name = makeUniqueInstName("repeater");
  debugPrint(logger_, RSZ, "repair_net", 2, "{:{}s}{} {} ({} {})",
             "", level,
             where,
             buffer_name.c_str(),
             units_->distanceUnit()->asString(dbuToMeters(x), 1),
             units_->distanceUnit()->asString(dbuToMeters(y), 1));

  // Inserting a buffer is complicated by the fact that verilog netlists
  // use the net name for input and output ports. This means the ports
  // cannot be moved to a different net.

  // This cannot depend on the net in caller because the buffer may be inserted
  // between the driver and the loads changing the net as the repair works its
  // way from the loads to the driver.

  Net *net = nullptr, *in_net, *out_net;
  bool have_output_port_load = false;
  for (Pin *pin : load_pins) {
    if (network_->isTopLevelPort(pin)) {
      net = network_->net(network_->term(pin));
      if (network_->direction(pin)->isAnyOutput()) {
        have_output_port_load = true;
        break;
      }
    }
    else
      net = network_->net(pin);
  }
  Instance *parent = db_network_->topInstance();

  // If the net is driven by an input port,
  // use the net as the repeater input net so the port stays connected to it.
  if (hasInputPort(net)
      || !have_output_port_load) {
    in_net = net;
    out_net = makeUniqueNet();
    // Copy signal type to new net.
    dbNet *out_net_db = db_network_->staToDb(out_net);
    dbNet *in_net_db = db_network_->staToDb(in_net);
    out_net_db->setSigType(in_net_db->getSigType());

    // Move load pins to out_net.
    for (Pin *pin : load_pins) {
      Port *port = network_->port(pin);
      Instance *inst = network_->instance(pin);
      sta_->disconnectPin(pin);
      sta_->connectPin(inst, port, out_net);
    }
  }
  else {
    // One of the loads is an output port.
    // Use the net as the repeater output net so the port stays connected to it.
    in_net = makeUniqueNet();
    out_net = net;
    // Copy signal type to new net.
    dbNet *out_net_db = db_network_->staToDb(out_net);
    dbNet *in_net_db = db_network_->staToDb(in_net);
    in_net_db->setSigType(out_net_db->getSigType());

    // Move non-repeater load pins to in_net.
    PinSet load_pins1;
    for (Pin *pin : load_pins)
      load_pins1.insert(pin);

    NetPinIterator *pin_iter = network_->pinIterator(out_net);
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      if (!load_pins1.hasKey(pin)) {
        Port *port = network_->port(pin);
        Instance *inst = network_->instance(pin);
        sta_->disconnectPin(pin);
        sta_->connectPin(inst, port, in_net);
      }
    }
  }

  Instance *buffer = makeInstance(buffer_cell,
                                  buffer_name.c_str(),
                                  parent);
  journalMakeBuffer(buffer);
  Point buf_loc(x, y);
  setLocation(buffer, buf_loc);
  designAreaIncr(area(db_network_->cell(buffer_cell)));
  inserted_buffer_count_++;

  sta_->connectPin(buffer, buffer_input_port, in_net);
  sta_->connectPin(buffer, buffer_output_port, out_net);

  parasiticsInvalid(in_net);
  parasiticsInvalid(out_net);

  // Resize repeater as we back up by levels.
  Pin *drvr_pin = network_->findPin(buffer, buffer_output_port);
  resizeToTargetSlew(drvr_pin, false);
  buffer_cell = network_->libertyCell(buffer);
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

  Pin *buf_in_pin = network_->findPin(buffer, buffer_input_port);
  load_pins.clear();
  load_pins.push_back(buf_in_pin);
  wire_length = 0;
  pin_cap = buffer_input_port->capacitance();
  fanout = portFanoutLoad(buffer_input_port);
}

bool
Resizer::hasInputPort(const Net *net)
{
  bool has_top_level_port = false;
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isTopLevelPort(pin)
        && network_->direction(pin)->isAnyInput()) {
      has_top_level_port = true;
      break;
    }
  }
  delete pin_iter;
  return has_top_level_port;
}

bool
Resizer::hasOutputPort(const Net *net)
{
  bool has_top_level_port = false;
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isTopLevelPort(pin)
        && network_->direction(pin)->isAnyOutput()) {
      has_top_level_port = true;
      break;
    }
  }
  delete pin_iter;
  return has_top_level_port;
}

bool
Resizer::hasPort(const Net *net)
{
  bool has_top_level_port = false;
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isTopLevelPort(pin)) {
      has_top_level_port = true;
      break;
    }
  }
  delete pin_iter;
  return has_top_level_port;
}

float
Resizer::driveResistance(const Pin *drvr_pin)
{
  if (network_->isTopLevelPort(drvr_pin)) {
    InputDrive *drive = sdc_->findInputDrive(network_->port(drvr_pin));
    if (drive) {
      float max_res = 0;
      for (auto min_max : MinMax::range()) {
        for (auto rf : RiseFall::range()) {
          LibertyCell *cell;
          LibertyPort *from_port;
          float *from_slews;
          LibertyPort *to_port;
          drive->driveCell(rf, min_max, cell, from_port, from_slews, to_port);
          if (to_port)
            max_res = max(max_res, to_port->driveResistance());
          else {
            float res;
            bool exists;
            drive->driveResistance(rf, min_max, res, exists);
            max_res = max(max_res, res);
          }
        }
      }
      return max_res;
    }
  }
  else {
    LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
    if (drvr_port)
      return drvr_port->driveResistance();
  }
  return 0.0;
}

////////////////////////////////////////////////////////////////

void
Resizer::resizeToTargetSlew()
{
  resize_count_ = 0;
  resized_multi_output_insts_.clear();
  incrementalParasiticsBegin();
  // Resize in reverse level order.
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex *drvr = level_drvr_vertices_[i];
    Pin *drvr_pin = drvr->pin();
    Net *net = network_->net(drvr_pin);
    if (net
        && !drvr->isConstant()
        && hasFanout(drvr)
        // Hands off the clock nets.
        && !sta_->isClock(drvr_pin)
        // Hands off special nets.
        && !db_network_->isSpecial(net)) {
      resizeToTargetSlew(drvr_pin, true);
      if (overMaxArea()) {
        logger_->error(RSZ, 24, "Max utilization reached.");
        break;
      }
    }
  }
  updateParasitics();
  incrementalParasiticsEnd();

  if (resize_count_ > 0)
    logger_->info(RSZ, 29, "Resized {} instances.", resize_count_);
}

bool
Resizer::hasFanout(Vertex *drvr)
{
  VertexOutEdgeIterator edge_iter(drvr, graph_);
  return edge_iter.hasNext();
}

void
Resizer::makeEquivCells()
{
  LibertyLibrarySeq libs;
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary *lib = lib_iter->next();
    // massive kludge until makeEquivCells is fixed to only incldue link cells
    LibertyCellIterator cell_iter(lib);
    if (cell_iter.hasNext()) {
      LibertyCell *cell = cell_iter.next();
      if (isLinkCell(cell))
        libs.push_back(lib);
    }
  }
  delete lib_iter;
  sta_->makeEquivCells(&libs, nullptr);
}

static float
targetLoadDist(float load_cap,
               float target_load)
{
  return abs(load_cap - target_load);
}

bool
Resizer::resizeToTargetSlew(const Pin *drvr_pin,
                            bool update_count)
{
  Instance *inst = network_->instance(drvr_pin);
  LibertyCell *cell = network_->libertyCell(inst);
  if (cell) {
    bool revisiting_inst = false;
    if (hasMultipleOutputs(inst)) {
      revisiting_inst = resized_multi_output_insts_.hasKey(inst);
      debugPrint(logger_, RSZ, "resize", 2, "multiple outputs{}",
                 revisiting_inst ? " - revisit" : "");
      resized_multi_output_insts_.insert(inst);
    }
    ensureWireParasitic(drvr_pin);
    // Includes net parasitic capacitance.
    float load_cap = graph_delay_calc_->loadCap(drvr_pin, tgt_slew_dcalc_ap_);
    if (load_cap > 0.0) {
      LibertyCell *target_cell = findTargetCell(cell, load_cap, revisiting_inst);
      if (target_cell != cell) {
        debugPrint(logger_, RSZ, "resize", 2, "{} {} -> {}",
                   sdc_network_->pathName(drvr_pin),
                   cell->name(),
                   target_cell->name());
        if (replaceCell(inst, target_cell, true)
            && !revisiting_inst
            && update_count)
          resize_count_++;
      }
    }
  }
  return false;
}

LibertyCell *
Resizer::findTargetCell(LibertyCell *cell,
                        float load_cap,
                        bool revisiting_inst)
{
  LibertyCell *best_cell = cell;
  LibertyCellSeq *equiv_cells = sta_->equivCells(cell);
  if (equiv_cells) {
    bool is_buf_inv = cell->isBuffer() || cell->isInverter();
    float target_load = (*target_load_map_)[cell];
    float best_load = target_load;
    float best_dist = targetLoadDist(load_cap, target_load);
    float best_delay = is_buf_inv
      ? bufferDelay(cell, load_cap, tgt_slew_dcalc_ap_)
      : 0.0;
    debugPrint(logger_, RSZ, "resize", 3, "{} load cap {} dist={:.2e} delay={}",
               cell->name(),
               units_->capacitanceUnit()->asString(load_cap),
               best_dist,
               delayAsString(best_delay, sta_, 3));
    for (LibertyCell *target_cell : *equiv_cells) {
      if (!dontUse(target_cell)
          && isLinkCell(target_cell)) {
        float target_load = (*target_load_map_)[target_cell];
        float delay = is_buf_inv
          ? bufferDelay(target_cell, load_cap, tgt_slew_dcalc_ap_)
          : 0.0;
        float dist = targetLoadDist(load_cap, target_load);
        debugPrint(logger_, RSZ, "resize", 3, " {} dist={:.2e} delay={}",
                   target_cell->name(),
                   dist,
                   delayAsString(delay, sta_, 3));
        if (is_buf_inv
            // Library may have "delay" buffers/inverters that are
            // functionally buffers/inverters but have additional
            // intrinsic delay. Accept worse target load matching if
            // delay is reduced to avoid using them.
            ? ((delay < best_delay
                && dist < best_dist * 1.1)
               || (dist < best_dist
                   && delay < best_delay * 1.1))
            : dist < best_dist
            // If the instance has multiple outputs (generally a register Q/QN)
            // only allow upsizing after the first pin is visited.
            && (!revisiting_inst
                || target_load > best_load)) {
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

// Replace LEF with LEF so ports stay aligned in instance.
bool
Resizer::replaceCell(Instance *inst,
                     LibertyCell *replacement,
                     bool journal)
{
  const char *replacement_name = replacement->name();
  dbMaster *replacement_master = db_->findMaster(replacement_name);
  if (replacement_master) {
    dbInst *dinst = db_network_->staToDb(inst);
    dbMaster *master = dinst->getMaster();
    designAreaIncr(-area(master));
    Cell *replacement_cell1 = db_network_->dbToSta(replacement_master);
    if (journal)
      journalInstReplaceCellBefore(inst);
    sta_->replaceCell(inst, replacement_cell1);
    designAreaIncr(area(replacement_master));

    // Invalidate estimated parasitics on all instance pins.
    // Input nets change pin cap, outputs change location (slightly).
    if (haveEstimatedParasitics()) {
      InstancePinIterator *pin_iter = network_->pinIterator(inst);
      while (pin_iter->hasNext()) {
        const Pin *pin = pin_iter->next();
        const Net *net = network_->net(pin);
        if (net)
          parasiticsInvalid(net);
      }
      delete pin_iter;
    }
    return true;
  }
  return false;
}

bool
Resizer::hasMultipleOutputs(const Instance *inst)
{
  int output_count = 0;
  InstancePinIterator *pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    const Pin *pin = pin_iter->next();
    if (network_->direction(pin)->isAnyOutput()
        && network_->net(pin)) {
      output_count++;
      if (output_count > 1)
        return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////

void
Resizer::resizeSlackPreamble()
{
  resizePreamble();
  // Save max_wire_length for multiple repairDesign calls.
  max_wire_length_ = findMaxWireLength();
  net_slack_map_.clear();
}

// Run repair design to find the slacks but save/restore all changes to the netlist.
void
Resizer::findResizeSlacks()
{
  journalBegin();
  estimateWireParasitics();
  int repair_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repairDesign(max_wire_length_, 0.0, 0.0,
               repair_count, slew_violations, cap_violations,
               fanout_violations, length_violations);
  findResizeSlacks1();
  journalRestore();
}
  
void
Resizer::findResizeSlacks1()
{
  // Use driver pin slacks rather than Sta::netSlack to save visiting
  // the net pins and min'ing the slack.
  NetSeq nets;
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex *drvr = level_drvr_vertices_[i];
    Pin *drvr_pin = drvr->pin();
    Net *net = network_->isTopLevelPort(drvr_pin)
      ? network_->net(network_->term(drvr_pin))
      : network_->net(drvr_pin);
    if (net
        && !drvr->isConstant()
        // Hands off special nets.
        && !db_network_->isSpecial(net)
        && !sta_->isClock(drvr_pin)) {
      net_slack_map_[net] = sta_->vertexSlack(drvr, max_);
      nets.push_back(net);
    }
  }

  // Find the nets with the worst slack.
  double worst_percent = .1;
  //  sort(nets.begin(), nets.end(). [&](const Net *net1,
  sort(nets, [this](const Net *net1,
                 const Net *net2)
             { return resizeNetSlack(net1) < resizeNetSlack(net2); });
  worst_slack_nets_.clear();
  for (int i = 0; i < nets.size() * worst_percent; i++)
    worst_slack_nets_.push_back(nets[i]);
}

NetSeq &
Resizer::resizeWorstSlackNets()
{
  return worst_slack_nets_;
}

vector<dbNet*>
Resizer::resizeWorstSlackDbNets()
{
  vector<dbNet*> nets;
  for (Net* net : worst_slack_nets_)
    nets.push_back(db_network_->staToDb(net));
  return nets;
}

Slack
Resizer::resizeNetSlack(const Net *net)
{
  return net_slack_map_[net];
}

Slack
Resizer::resizeNetSlack(const dbNet *db_net)
{
  const Net *net = db_network_->dbToSta(db_net);
  return net_slack_map_[net];
}

////////////////////////////////////////////////////////////////

double
Resizer::area(Cell *cell)
{
  return area(db_network_->staToDb(cell));
}

double
Resizer::area(dbMaster *master)
{
  if (!master->isCoreAutoPlaceable()) {
    return 0;
  }
  return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

double
Resizer::dbuToMeters(int dist) const
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  return dist / (dbu * 1e+6);
}

int
Resizer::metersToDbu(double dist) const
{
  int dbu = db_->getTech()->getDbUnitsPerMicron();
  return dist * dbu * 1e+6;
}

void
Resizer::setMaxUtilization(double max_utilization)
{
  max_area_ = coreArea() * max_utilization;
}

bool
Resizer::overMaxArea()
{
  return max_area_
    && fuzzyGreaterEqual(design_area_, max_area_);
}

void
Resizer::setDontUse(LibertyCellSeq *dont_use)
{
  if (dont_use) {
    for (LibertyCell *cell : *dont_use)
      dont_use_.insert(cell);
  }
}

bool
Resizer::dontUse(LibertyCell *cell)
{
  return cell->dontUse()
    || dont_use_.hasKey(cell);
}

////////////////////////////////////////////////////////////////

// Find a target slew for the libraries and then
// a target load for each cell that gives the target slew.
void
Resizer::findTargetLoads()
{
  // Find target slew across all buffers in the libraries.
  findBufferTargetSlews();
  if (target_load_map_ == nullptr)
    target_load_map_ = new CellTargetLoadMap;
  target_load_map_->clear();

  // Find target loads at the tgt_slew_corner.
  int lib_ap_index = tgt_slew_corner_->libertyIndex(max_);
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary *lib = lib_iter->next();
    LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      LibertyCell *cell = cell_iter.next();
      if (isLinkCell(cell)) {
        LibertyCell *corner_cell = cell->cornerCell(lib_ap_index);
        float tgt_load;
        bool exists;
        target_load_map_->findKey(corner_cell, tgt_load, exists);
        if (!exists) {
          tgt_load = findTargetLoad(corner_cell);
          (*target_load_map_)[corner_cell] = tgt_load;
        }
        // Map link cell to corner cell target load.
        if (cell != corner_cell)
          (*target_load_map_)[cell] = tgt_load;
      }
    }
  }
  delete lib_iter;
}

float
Resizer::targetLoadCap(LibertyCell *cell)
{
  float load_cap = 0.0;
  bool exists;
  target_load_map_->findKey(cell, load_cap, exists);
  if (!exists)
    logger_->error(RSZ, 68, "missing target load cap.");
  return load_cap;
}

float
Resizer::findTargetLoad(LibertyCell *cell)
{
  LibertyCellTimingArcSetIterator arc_set_iter(cell);
  float target_load_sum = 0.0;
  int arc_count = 0;
  while (arc_set_iter.hasNext()) {
    TimingArcSet *arc_set = arc_set_iter.next();
    TimingRole *role = arc_set->role();
    if (!role->isTimingCheck()
        && role != TimingRole::tristateDisable()
        && role != TimingRole::tristateEnable()) {
      TimingArcSetArcIterator arc_iter(arc_set);
      while (arc_iter.hasNext()) {
        TimingArc *arc = arc_iter.next();
        int in_rf_index = arc->fromTrans()->asRiseFall()->index();
        int out_rf_index = arc->toTrans()->asRiseFall()->index();
        float arc_target_load = findTargetLoad(cell, arc, 
                                               tgt_slews_[in_rf_index],
                                               tgt_slews_[out_rf_index]);
        debugPrint(logger_, RSZ, "target_load", 3, "{} {} -> {} {} target_load = {:.2e}",
                   cell->name(),
                   arc->from()->name(),
                   arc->to()->name(),
                   arc->toTrans()->asString(),
                   arc_target_load);
        target_load_sum += arc_target_load;
        arc_count++;
      }
    }
  }
  float target_load = arc_count ? target_load_sum / arc_count : 0.0;
  debugPrint(logger_, RSZ, "target_load", 2, "{} target_load = {:.2e}",
             cell->name(),
             target_load);
  return target_load;
}

// Find the load capacitance that will cause the output slew
// to be equal to out_slew.
float
Resizer::findTargetLoad(LibertyCell *cell,
                        TimingArc *arc,
                        Slew in_slew,
                        Slew out_slew)
{
  GateTimingModel *model = dynamic_cast<GateTimingModel*>(arc->model());
  if (model) {
    // load_cap1 lower bound
    // load_cap2 upper bound
    double load_cap1 = 0.0;
    double load_cap2 = 1.0e-12;  // 1pF
    double tol = .01; // 1%
    double diff1 = gateSlewDiff(cell, arc, model, in_slew, load_cap1, out_slew);
    if (diff1 > 0.0)
      // Zero load cap out_slew is higher than the target.
      return 0.0;
    double diff2 = gateSlewDiff(cell, arc, model, in_slew, load_cap2, out_slew);
    // binary search for diff = 0.
    while (abs(load_cap1 - load_cap2) > max(load_cap1, load_cap2) * tol) {
      if (diff2 < 0.0) {
        load_cap1 = load_cap2;
        diff1 = diff2;
        load_cap2 *= 2;
        diff2 = gateSlewDiff(cell, arc, model, in_slew, load_cap2, out_slew);
      }
      else {
        double load_cap3 = (load_cap1 + load_cap2) / 2.0;
        double diff3 = gateSlewDiff(cell, arc, model, in_slew, load_cap3, out_slew);
        if (diff3 < 0.0) {
          load_cap1 = load_cap3;
          diff1 = diff3;
        }
        else {
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
Slew
Resizer::gateSlewDiff(LibertyCell *cell,
                      TimingArc *arc,
                      GateTimingModel *model,
                      Slew in_slew,
                      float load_cap,
                      Slew out_slew)

{
  const Pvt *pvt = tgt_slew_dcalc_ap_->operatingConditions();
  ArcDelay arc_delay;
  Slew arc_slew;
  model->gateDelay(cell, pvt, in_slew, load_cap, 0.0, false,
                   arc_delay, arc_slew);
  return arc_slew - out_slew;
}

////////////////////////////////////////////////////////////////

Slew
Resizer::targetSlew(const RiseFall *rf)
{
  return tgt_slews_[rf->index()];
}

// Find target slew across all buffers in the libraries.
void
Resizer::findBufferTargetSlews()
{
  tgt_slews_ = {0.0};
  tgt_slew_corner_ = nullptr;
  
  for (Corner *corner : *sta_->corners()) {
    int lib_ap_index = corner->libertyIndex(max_);
    const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
    const Pvt *pvt = dcalc_ap->operatingConditions();
    // Average slews across buffers at corner.
    Slew slews[RiseFall::index_count]{0.0};
    int counts[RiseFall::index_count]{0};
    for (LibertyCell *buffer : buffer_cells_) {
      LibertyCell *corner_buffer = buffer->cornerCell(lib_ap_index);
      findBufferTargetSlews(corner_buffer, pvt, slews, counts);
    }
    Slew slew_rise = slews[RiseFall::riseIndex()] / counts[RiseFall::riseIndex()];
    Slew slew_fall = slews[RiseFall::fallIndex()] / counts[RiseFall::fallIndex()];
    // Use the target slews from the slowest corner,
    // and resize using that corner.
    if (slew_rise > tgt_slews_[RiseFall::riseIndex()]) {
      tgt_slews_[RiseFall::riseIndex()] = slew_rise;
      tgt_slews_[RiseFall::fallIndex()] = slew_fall;
      tgt_slew_corner_ = corner;
      tgt_slew_dcalc_ap_ = corner->findDcalcAnalysisPt(max_);
    }
  }

  debugPrint(logger_, RSZ, "target_load", 1, "target slew corner {} = {}/{}",
             tgt_slew_corner_->name(),
             delayAsString(tgt_slews_[RiseFall::riseIndex()], sta_, 3),
             delayAsString(tgt_slews_[RiseFall::fallIndex()], sta_, 3));
}

void
Resizer::findBufferTargetSlews(LibertyCell *buffer,
                               const Pvt *pvt,
                               // Return values.
                               Slew slews[],
                               int counts[])
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);
  TimingArcSetSeq *arc_sets = buffer->timingArcSets(input, output);
  if (arc_sets) {
    for (TimingArcSet *arc_set : *arc_sets) {
      TimingArcSetArcIterator arc_iter(arc_set);
      while (arc_iter.hasNext()) {
        TimingArc *arc = arc_iter.next();
        GateTimingModel *model = dynamic_cast<GateTimingModel*>(arc->model());
        RiseFall *in_rf = arc->fromTrans()->asRiseFall();
        RiseFall *out_rf = arc->toTrans()->asRiseFall();
        float in_cap = input->capacitance(in_rf, max_);
        float load_cap = in_cap * tgt_slew_load_cap_factor;
        ArcDelay arc_delay;
        Slew arc_slew;
        model->gateDelay(buffer, pvt, 0.0, load_cap, 0.0, false,
                         arc_delay, arc_slew);
        model->gateDelay(buffer, pvt, arc_slew, load_cap, 0.0, false,
                         arc_delay, arc_slew);
        slews[out_rf->index()] += arc_slew;
        counts[out_rf->index()]++;
      }
    }
  }
}

////////////////////////////////////////////////////////////////

// Repair tie hi/low net driver fanout by duplicating the
// tie hi/low instances for every pin connected to tie hi/low instances.
void
Resizer::repairTieFanout(LibertyPort *tie_port,
                         double separation, // meters
                         bool verbose)
{
  ensureBlock();
  ensureDesignArea();
  Instance *top_inst = network_->topInstance();
  LibertyCell *tie_cell = tie_port->libertyCell();
  InstanceSeq insts;
  findCellInstances(tie_cell, insts);
  int tie_count = 0;
  int separation_dbu = metersToDbu(separation);
  for (Instance *inst : insts) {
    Pin *drvr_pin = network_->findPin(inst, tie_port);
    if (drvr_pin) {
      const char *inst_name = network_->name(inst);
      Net *net = network_->net(drvr_pin);
      if (net) {
        NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
        while (pin_iter->hasNext()) {
          Pin *load = pin_iter->next();
          if (load != drvr_pin) {
            // Make tie inst.
            Point tie_loc = tieLocation(load, separation_dbu);
            Instance *load_inst = network_->instance(load);
            string tie_name = makeUniqueInstName(inst_name, true);
            Instance *tie = makeInstance(tie_cell, tie_name.c_str(),
                                         top_inst);
            setLocation(tie, tie_loc);

            // Make tie output net.
            Net *load_net = makeUniqueNet();

            // Connect tie inst output.
            sta_->connectPin(tie, tie_port, load_net);

            // Connect load to tie output net.
            sta_->disconnectPin(load);
            Port *load_port = network_->port(load);
            sta_->connectPin(load_inst, load_port, load_net);

            designAreaIncr(area(db_network_->cell(tie_cell)));
            tie_count++;
          }
        }
        delete pin_iter;

        // Delete inst output net.
        Pin *tie_pin = network_->findPin(inst, tie_port);
        Net *tie_net = network_->net(tie_pin);
        sta_->deleteNet(tie_net);
        parasitics_invalid_.erase(tie_net);
        // Delete the tie instance.
        sta_->deleteInstance(inst);
      }
    }
  }

  if (tie_count > 0) {
    logger_->info(RSZ, 42, "Inserted {} tie {} instances.",
                  tie_count,
                  tie_cell->name());
    level_drvr_vertices_valid_ = false;
  }
}

void
Resizer::findCellInstances(LibertyCell *cell,
                           // Return value.
                           InstanceSeq &insts)
{
  LeafInstanceIterator *inst_iter = network_->leafInstanceIterator();
  while (inst_iter->hasNext()) {
    Instance *inst = inst_iter->next();
    if (network_->libertyCell(inst) == cell)
      insts.push_back(inst);
  }
  delete inst_iter;
}

// Place the tie instance on the side of the load pin.
Point
Resizer::tieLocation(Pin *load,
                     int separation)
{
  Point load_loc = db_network_->location(load);
  int load_x = load_loc.getX();
  int load_y = load_loc.getY();
  int tie_x = load_x;
  int tie_y = load_y;
  if (!network_->isTopLevelPort(load)) {
    dbInst *db_inst = db_network_->staToDb(network_->instance(load));
    dbBox *bbox = db_inst->getBBox();
    int left_dist = abs(load_x - bbox->xMin());
    int right_dist = abs(load_x - bbox->xMax());
    int bot_dist = abs(load_y - bbox->yMin());
    int top_dist = abs(load_y - bbox->yMax());
    if (left_dist < right_dist
        && left_dist < bot_dist
        && left_dist < top_dist)
      // left
      tie_x -= separation;
    if (right_dist < left_dist
        && right_dist < bot_dist
        && right_dist < top_dist)
      // right
      tie_x += separation;
    if (bot_dist < left_dist
        && bot_dist < right_dist
        && bot_dist < top_dist)
      // bot
      tie_y -= separation;
    if (top_dist < left_dist
        && top_dist < right_dist
        && top_dist < bot_dist)
      // top
      tie_y += separation;
  }
  return Point(tie_x, tie_y);
}

////////////////////////////////////////////////////////////////

void
Resizer::repairSetup(float slack_margin,
                     int max_passes)
{
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  Slack worst_slack;
  Vertex *worst_vertex;
  sta_->worstSlack(max_, worst_slack, worst_vertex);
  debugPrint(logger_, RSZ, "repair_setup", 1, "worst_slack = {}",
             delayAsString(worst_slack, sta_, 3));
  Slack prev_worst_slack = -INF;
  int pass = 1;
  int decreasing_slack_passes = 0;
  incrementalParasiticsBegin();
  while (fuzzyLess(worst_slack, slack_margin)
         && pass <= max_passes) {
    PathRef worst_path = sta_->vertexWorstSlackPath(worst_vertex, max_);
    bool changed = repairSetup(worst_path, worst_slack);
    updateParasitics();
    sta_->findRequireds();
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    bool decreasing_slack = fuzzyLessEqual(worst_slack, prev_worst_slack);
    debugPrint(logger_, RSZ, "repair_setup", 1, "pass {} worst_slack = {} {}",
               pass,
               delayAsString(worst_slack, sta_, 3),
               decreasing_slack ? "v" : "^");
    if (decreasing_slack) {
      // Allow slack to increase to get out of local minima.
      // Do not update prev_worst_slack so it saves the high water mark.
      decreasing_slack_passes++;
      if (!changed
          || decreasing_slack_passes > repair_setup_decreasing_slack_passes_allowed_) {
        // Undo changes that reduced slack.
        journalRestore();
        debugPrint(logger_, RSZ, "repair_setup", 1,
                   "decreasing slack for {} passes. Restoring best slack {}",
                   decreasing_slack_passes,
                   delayAsString(prev_worst_slack, sta_, 3));
        break;
      }
    }
    else {
      prev_worst_slack = worst_slack;
      decreasing_slack_passes = 0;
      // Progress, start journal so we can back up to here.
      journalBegin();
    }
    if (overMaxArea())
      break;
    pass++;
  }
  // Leave the parasitics up to date.
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0)
    logger_->info(RSZ, 40, "Inserted {} buffers.", inserted_buffer_count_);
  if (resize_count_ > 0)
    logger_->info(RSZ, 41, "Resized {} instances.", resize_count_);
  if (fuzzyLess(worst_slack, slack_margin))
    logger_->warn(RSZ, 62, "Unable to repair all setup violations.");
  if (overMaxArea())
    logger_->error(RSZ, 25, "max utilization reached.");
}

// For testing.
void
Resizer::repairSetup(Pin *end_pin)
{
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  Vertex *vertex = graph_->pinLoadVertex(end_pin);
  Slack slack = sta_->vertexSlack(vertex, max_);
  PathRef path = sta_->vertexWorstSlackPath(vertex, max_);
  incrementalParasiticsBegin();
  repairSetup(path, slack);
  // Leave the parasitices up to date.
  updateParasitics();
  incrementalParasiticsEnd();

  if (inserted_buffer_count_ > 0)
    logger_->info(RSZ, 30, "Inserted {} buffers.", inserted_buffer_count_);
  if (resize_count_ > 0)
    logger_->info(RSZ, 31, "Resized {} instances.", resize_count_);
}

bool
Resizer::repairSetup(PathRef &path,
                     Slack path_slack)
{
  PathExpanded expanded(&path, sta_);
  bool changed = false;
  if (expanded.size() > 1) {
    int path_length = expanded.size();
    vector<pair<int, Delay>> load_delays;
    int start_index = expanded.startIndex();
    const DcalcAnalysisPt *dcalc_ap = path.dcalcAnalysisPt(sta_);
    int lib_ap = dcalc_ap->libertyIndex();
    // Find load delay for each gate in the path.
    for (int i = start_index; i < path_length; i++) {
      PathRef *path = expanded.path(i);
      Vertex *path_vertex = path->vertex(sta_);
      const Pin *path_pin = path->pin(sta_);
      if (network_->isDriver(path_pin)
          && !network_->isTopLevelPort(path_pin)) {
        TimingArc *prev_arc = expanded.prevArc(i);
        TimingArc *corner_arc = prev_arc->cornerArc(lib_ap);
        Edge *prev_edge = path->prevEdge(prev_arc, sta_);
        Delay load_delay = graph_->arcDelay(prev_edge, prev_arc, dcalc_ap->index())
          // Remove intrinsic delay to find load dependent delay.
          - corner_arc->intrinsicDelay();
        load_delays.push_back(pair(i, load_delay));
        debugPrint(logger_, RSZ, "repair_setup", 3, "{} load_delay = {}",
                   path_vertex->name(network_),
                   delayAsString(load_delay, sta_, 3));
      }
    }

    sort(load_delays.begin(), load_delays.end(),
         [](pair<int, Delay> pair1,
            pair<int, Delay> pair2) {
           return pair1.second > pair2.second;
         });
    // Attack gates with largest load delays first.
    for (auto index_delay : load_delays) {
      int drvr_index = index_delay.first;
      PathRef *drvr_path = expanded.path(drvr_index);
      Vertex *drvr_vertex = drvr_path->vertex(sta_);
      const Pin *drvr_pin = drvr_vertex->pin();
      LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
      LibertyCell *drvr_cell = drvr_port ? drvr_port->libertyCell() : nullptr;
      int fanout = this->fanout(drvr_vertex);
      debugPrint(logger_, RSZ, "repair_setup", 2, "{} {} fanout = {}",
                 network_->pathName(drvr_pin),
                 drvr_cell ? drvr_cell->name() : "none",
                 fanout);

      if (upsizeDrvr(drvr_path, drvr_index, &expanded)) {
        changed = true;
        break;
      }

      // For tristate nets all we can do is resize the driver.
      bool tristate_drvr = isTristateDriver(drvr_pin);
      if (fanout > 1
          // Rebuffer blows up on large fanout nets.
          && fanout < rebuffer_max_fanout_
          && !tristate_drvr) { 
        int rebuffer_count = rebuffer(drvr_pin);
        if (rebuffer_count > 0) {
          debugPrint(logger_, RSZ, "repair_setup", 2, "rebuffer {} inserted {}",
                     network_->pathName(drvr_pin),
                     rebuffer_count);
          changed = true;
          break;
        }
      }

      // Don't split loads on low fanout nets.
      if (fanout > split_load_min_fanout_
          && !tristate_drvr) {
        splitLoads(drvr_path, drvr_index, path_slack, &expanded);
        changed = true;
        break;
      }
    }
  }
  return changed;
}

bool
Resizer::upsizeDrvr(PathRef *drvr_path,
                    int drvr_index,
                    PathExpanded *expanded)
{
  Pin *drvr_pin = drvr_path->pin(this);
  const DcalcAnalysisPt *dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
  float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
  int in_index = drvr_index - 1;
  PathRef *in_path = expanded->path(in_index);
  Pin *in_pin = in_path->pin(sta_);
  LibertyPort *in_port = network_->libertyPort(in_pin);

  float prev_drive;
  if (drvr_index >= 2) {
    int prev_drvr_index = drvr_index - 2;
    PathRef *prev_drvr_path = expanded->path(prev_drvr_index);
    Pin *prev_drvr_pin = prev_drvr_path->pin(sta_);
    prev_drive = 0.0;
    LibertyPort *prev_drvr_port = network_->libertyPort(prev_drvr_pin);
    if (prev_drvr_port) {
      prev_drive = prev_drvr_port->driveResistance();
    }
  }
  else
    prev_drive = 0.0;
  LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
  LibertyCell *upsize = upsizeCell(in_port, drvr_port, load_cap,
                                   prev_drive, dcalc_ap);
  if (upsize) {
    Instance *drvr = network_->instance(drvr_pin);
    debugPrint(logger_, RSZ, "repair_setup", 2, "resize {} {} -> {}",
               network_->pathName(drvr_pin),
               drvr_port->libertyCell()->name(),
               upsize->name());
    if (replaceCell(drvr, upsize, true)) {
      resize_count_++;
      return true;
    }
  }
  return false;
}

LibertyCell *
Resizer::upsizeCell(LibertyPort *in_port,
                    LibertyPort *drvr_port,
                    float load_cap,
                    float prev_drive,
                    const DcalcAnalysisPt *dcalc_ap)
{
  int lib_ap = dcalc_ap->libertyIndex();
  LibertyCell *cell = drvr_port->libertyCell();
  LibertyCellSeq *equiv_cells = sta_->equivCells(cell);
  if (equiv_cells) {
    const char *in_port_name = in_port->name();
    const char *drvr_port_name = drvr_port->name();
    sort(equiv_cells,
         [=] (const LibertyCell *cell1,
              const LibertyCell *cell2) {
           LibertyPort *port1=cell1->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           LibertyPort *port2=cell2->findLibertyPort(drvr_port_name)->cornerPort(lib_ap);
           return port1->driveResistance() > port2->driveResistance();
         });
    float drive = drvr_port->cornerPort(lib_ap)->driveResistance();
    float delay = gateDelay(drvr_port, load_cap, tgt_slew_dcalc_ap_)
      + prev_drive * in_port->cornerPort(lib_ap)->capacitance();
    for (LibertyCell *equiv : *equiv_cells) {
      LibertyCell *equiv_corner = equiv->cornerCell(lib_ap);
      LibertyPort *equiv_drvr = equiv_corner->findLibertyPort(drvr_port_name);
      LibertyPort *equiv_input = equiv_corner->findLibertyPort(in_port_name);
      float equiv_drive = equiv_drvr->driveResistance();
      // Include delay of previous driver into equiv gate.
      float equiv_delay = gateDelay(equiv_drvr, load_cap, dcalc_ap)
        + prev_drive * equiv_input->capacitance();
      if (!dontUse(equiv)
          && equiv_drive < drive
          && equiv_delay < delay)
        return equiv;
    }
  }
  return nullptr;
}

void
Resizer::splitLoads(PathRef *drvr_path,
                    int drvr_index,
                    Slack drvr_slack,
                    PathExpanded *expanded)
{
  Pin *drvr_pin = drvr_path->pin(this);
  PathRef *load_path = expanded->path(drvr_index + 1);
  Vertex *load_vertex = load_path->vertex(sta_);
  Pin *load_pin = load_vertex->pin();
  // Divide and conquer.
  debugPrint(logger_, RSZ, "repair_setup", 2, "split loads {} -> {}",
             network_->pathName(drvr_pin),
             network_->pathName(load_pin));

  Vertex *drvr_vertex = drvr_path->vertex(sta_);
  const RiseFall *rf = drvr_path->transition(sta_);
  // Sort fanouts of the drvr on the critical path by slack margin
  // wrt the critical path slack.
  vector<pair<Vertex*, Slack>> fanout_slacks;
  VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    Vertex *fanout_vertex = edge->to(graph_);
    Slack fanout_slack = sta_->vertexSlack(fanout_vertex, rf, max_);
    Slack slack_margin = fanout_slack - drvr_slack;
    debugPrint(logger_, RSZ, "repair_setup", 3, " fanin {} slack_margin = {}",
               network_->pathName(fanout_vertex->pin()),
               delayAsString(slack_margin, sta_, 3));
    fanout_slacks.push_back(pair<Vertex*, Slack>(fanout_vertex, slack_margin));
  }

  sort(fanout_slacks.begin(), fanout_slacks.end(),
       [](pair<Vertex*, Slack> pair1,
          pair<Vertex*, Slack> pair2) {
         return pair1.second > pair2.second;
       });

  Net *net = network_->net(drvr_pin);

  string buffer_name = makeUniqueInstName("split");
  Instance *parent = db_network_->topInstance();
  LibertyCell *buffer_cell = buffer_lowest_drive_;
  Instance *buffer = makeInstance(buffer_cell,
                                  buffer_name.c_str(),
                                  parent);
  journalMakeBuffer(buffer);
  inserted_buffer_count_++;
  designAreaIncr(area(db_network_->cell(buffer_cell)));

  Net *out_net = makeUniqueNet();
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  Point drvr_loc = db_network_->location(drvr_pin);
  setLocation(buffer, drvr_loc);

  // Split the loads with extra slack to an inserted buffer.
  // before
  // drvr_pin -> net -> load_pins
  // after
  // drvr_pin -> net -> load_pins with low slack
  //                 -> buffer_in -> net -> rest of loads
  sta_->connectPin(buffer, input, net);
  parasiticsInvalid(net);
  sta_->connectPin(buffer, output, out_net);
  int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; i++) {
    pair<Vertex*, Slack> fanout_slack = fanout_slacks[i];
    Vertex *load_vertex = fanout_slack.first;
    Pin *load_pin = load_vertex->pin();
    // Leave ports connected to original net so verilog port names are preserved.
    if (!network_->isTopLevelPort(load_pin)) {
      LibertyPort *load_port = network_->libertyPort(load_pin);
      Instance *load = network_->instance(load_pin);

      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, out_net);
    }
  }
  Pin *buffer_out_pin = network_->findPin(buffer, output);
  resizeToTargetSlew(buffer_out_pin, false);
}

////////////////////////////////////////////////////////////////

void
Resizer::repairHold(float slack_margin,
                    bool allow_setup_violations,
                    // Max buffer count as percent of design instance count.
                    float max_buffer_percent)
{
  init();
  LibertyCell *buffer_cell = findHoldBuffer();
  sta_->findRequireds();
  VertexSet *ends = sta_->search()->endpoints();
  int max_buffer_count = max_buffer_percent * network_->instanceCount();
  incrementalParasiticsBegin();
  repairHold(ends, buffer_cell, slack_margin,
             allow_setup_violations, max_buffer_count);

  // Leave the parasitices up to date.
  updateParasitics();
  incrementalParasiticsEnd();
}

// For testing/debug.
void
Resizer::repairHold(Pin *end_pin,
                    LibertyCell *buffer_cell,
                    float slack_margin,
                    bool allow_setup_violations,
                    float max_buffer_percent)
{
  Vertex *end = graph_->pinLoadVertex(end_pin);
  VertexSet ends;
  ends.insert(end);

  init();
  sta_->findRequireds();
  int max_buffer_count = max_buffer_percent * network_->instanceCount();
  incrementalParasiticsBegin();
  repairHold(&ends, buffer_cell, slack_margin, allow_setup_violations,
             max_buffer_count);
  // Leave the parasitices up to date.
  updateParasitics();
  incrementalParasiticsEnd();
}

// Find the buffer with the most delay in the fastest corner.
LibertyCell *
Resizer::findHoldBuffer()
{
  LibertyCell *max_buffer = nullptr;
  float max_delay = 0.0;
  for (LibertyCell *buffer : buffer_cells_) {
    float buffer_min_delay = bufferHoldDelay(buffer);
    if (max_buffer == nullptr
        || buffer_min_delay > max_delay) {
      max_buffer = buffer;
      max_delay = buffer_min_delay;
    }
  }
  return max_buffer;
}

float
Resizer::bufferHoldDelay(LibertyCell *buffer)
{
  Delay delays[RiseFall::index_count];
  bufferHoldDelays(buffer, delays);
  return min(delays[RiseFall::riseIndex()],
             delays[RiseFall::fallIndex()]);
}

// Min self delay across corners; buffer -> buffer
void
Resizer::bufferHoldDelays(LibertyCell *buffer,
                          // Return values.
                          Delay delays[RiseFall::index_count])
{
  LibertyPort *input, *output;
  buffer->bufferPorts(input, output);

  for (int rf_index : RiseFall::rangeIndex())
    delays[rf_index] = MinMax::min()->initValue();
  for (Corner *corner : *sta_->corners()) {
    LibertyPort *corner_port = input->cornerPort(corner->libertyIndex(max_));
    const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
    float load_cap = corner_port->capacitance();
    ArcDelay gate_delays[RiseFall::index_count];
    Slew slews[RiseFall::index_count];
    gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
    for (int rf_index : RiseFall::rangeIndex())
      delays[rf_index] = min(delays[rf_index], gate_delays[rf_index]);
  }
}

void
Resizer::repairHold(VertexSet *ends,
                    LibertyCell *buffer_cell,
                    float slack_margin,
                    bool allow_setup_violations,
                    int max_buffer_count)
{
  // Find endpoints with hold violation.
  VertexSet hold_failures;
  Slack worst_slack;
  findHoldViolations(ends, slack_margin, worst_slack, hold_failures);
  if (!hold_failures.empty()) {
    logger_->info(RSZ, 46, "Found {} endpoints with hold violations.",
                  hold_failures.size());
    inserted_buffer_count_ = 0;
    int repair_count = 1;
    int pass = 1;
    while (!hold_failures.empty()
           // Make sure we are making progress.
           && repair_count > 0
           && !overMaxArea()
           && inserted_buffer_count_ <= max_buffer_count) {
      repair_count = repairHoldPass(hold_failures, buffer_cell, slack_margin,
                                    allow_setup_violations, max_buffer_count);
      debugPrint(logger_, RSZ, "repair_hold", 1,
                 "pass {} worst slack {} failures {} inserted {}",
                 pass,
                 delayAsString(worst_slack, sta_, 3),
                 hold_failures .size(),
                 repair_count);
      sta_->findRequireds();
      findHoldViolations(ends, slack_margin, worst_slack, hold_failures);
      pass++;
    }
    if (slack_margin == 0.0 && fuzzyLess(worst_slack, 0.0))
      logger_->warn(RSZ, 66, "Unable to repair all hold violations.");
    else if (fuzzyLess(worst_slack, slack_margin))
      logger_->warn(RSZ, 64, "Unable to repair all hold checks within margin.");

    if (inserted_buffer_count_ > 0) {
      logger_->info(RSZ, 32, "Inserted {} hold buffers.", inserted_buffer_count_);
      level_drvr_vertices_valid_ = false;
    }
    if (inserted_buffer_count_ > max_buffer_count)
      logger_->error(RSZ, 60, "Max buffer count reached.");
    if (overMaxArea())
      logger_->error(RSZ, 50, "Max utilization reached.");
  }
  else
    logger_->info(RSZ, 33, "No hold violations found.");
}

void
Resizer::findHoldViolations(VertexSet *ends,
                            float slack_margin,
                            // Return values.
                            Slack &worst_slack,
                            VertexSet &hold_violations)
{
  worst_slack = INF;
  hold_violations.clear();
  debugPrint(logger_, RSZ, "repair_hold", 3, "Hold violations");
  for (Vertex *end : *ends) {
    Slack slack = sta_->vertexSlack(end, MinMax::min());
    if (!sta_->isClock(end->pin())
        && fuzzyLess(slack, slack_margin)) {
      debugPrint(logger_, RSZ, "repair_hold", 3, " {}",
                 end->name(sdc_network_));
      if (slack < worst_slack)
        worst_slack = slack;
      hold_violations.insert(end);
    }
  }
}

int
Resizer::repairHoldPass(VertexSet &hold_failures,
                        LibertyCell *buffer_cell,
                        float slack_margin,
                        bool allow_setup_violations,
                        int max_buffer_count)
{
  VertexSet fanins = findHoldFanins(hold_failures);
  VertexSeq sorted_fanins = sortHoldFanins(fanins);
  
  int repair_count = 0;
  int max_repair_count = max(static_cast<int>(hold_failures.size() * .2), 10);
  for(int i = 0; i < sorted_fanins.size() && repair_count < max_repair_count ; i++) {
    Vertex *vertex = sorted_fanins[i];
    Pin *drvr_pin = vertex->pin();
    Net *net = network_->isTopLevelPort(drvr_pin)
      ? network_->net(network_->term(drvr_pin))
      : network_->net(drvr_pin);
    updateParasitics();
    Slack drvr_slacks[RiseFall::index_count][MinMax::index_count];
    sta_->vertexSlacks(vertex, drvr_slacks);
    int min_index = MinMax::minIndex();
    int max_index = MinMax::maxIndex();
    const RiseFall *drvr_rf = (drvr_slacks[RiseFall::riseIndex()][min_index] <
                               drvr_slacks[RiseFall::fallIndex()][min_index])
      ? RiseFall::rise()
      : RiseFall::fall();
    Slack drvr_hold_slack = drvr_slacks[drvr_rf->index()][min_index] - slack_margin;
    Slack drvr_setup_slack = drvr_slacks[drvr_rf->index()][max_index];
    if (!vertex->isConstant()
        && fuzzyLess(drvr_hold_slack, 0.0)
        // Hands off special nets.
        && !db_network_->isSpecial(net)
        && !isTristateDriver(drvr_pin)
        // Have to have enough setup slack to add delay to repair the hold violation.
        && (allow_setup_violations
            || fuzzyLess(drvr_hold_slack, drvr_setup_slack))) {
      debugPrint(logger_, RSZ, "repair_hold", 2, "driver {}",
                 vertex->name(sdc_network_));
      // Only add delay to loads with hold violations.
      PinSeq load_pins;
      float load_cap = 0.0;
      bool loads_have_out_port = false;
      VertexOutEdgeIterator edge_iter(vertex, graph_);
      while (edge_iter.hasNext()) {
        Edge *edge = edge_iter.next();
        Vertex *fanout = edge->to(graph_);
        Slack fanout_slack = sta_->vertexSlack(fanout, MinMax::min());
        if (fanout_slack < slack_margin) {
          Pin *fanout_pin = fanout->pin();
          load_pins.push_back(fanout_pin);
          if (network_->direction(fanout_pin)->isAnyOutput()
              && network_->isTopLevelPort(fanout_pin))
            loads_have_out_port = true;
          else {
            LibertyPort *fanout_port = network_->libertyPort(fanout_pin);
            if (fanout_port)
              load_cap += fanout_port->capacitance();
          }
        }
      }
      if (!load_pins.empty()) {
        // multi-corner support MIA
        const DcalcAnalysisPt *dcalc_ap = sta_->cmdCorner()->findDcalcAnalysisPt(max_);
        Delay buffer_delay1 = bufferDelay(buffer_cell, drvr_rf, load_cap, dcalc_ap);
        // Need enough slack to at least insert the last buffer with loads.
        if (allow_setup_violations
            || (buffer_delay1 < drvr_slacks[RiseFall::riseIndex()][max_index]
                && buffer_delay1 < drvr_slacks[RiseFall::fallIndex()][max_index])) {
          Delay buffer_delay = bufferHoldDelay(buffer_cell);
          Delay max_insert = min(drvr_slacks[RiseFall::riseIndex()][max_index],
                                 drvr_slacks[RiseFall::fallIndex()][max_index]);
          int max_insert_count = delayInf(max_insert)
            ? 1
            : (max_insert - buffer_delay1) / buffer_delay + 1;
          int hold_buffer_count = (-drvr_hold_slack > buffer_delay1)
            ? std::ceil((-drvr_hold_slack-buffer_delay1)/buffer_delay)+1
            : 1;
          int buffer_count = allow_setup_violations
            ? hold_buffer_count
            : min(max_insert_count, hold_buffer_count);
          debugPrint(logger_, RSZ, "repair_hold", 2,
                     " {} hold={} inserted {} for {}/{} loads",
                     vertex->name(sdc_network_),
                     delayAsString(drvr_hold_slack, this, 3),
                     buffer_count,
                     load_pins.size(),
                     fanout(vertex));
          makeHoldDelay(vertex, buffer_count, load_pins,
                        loads_have_out_port, buffer_cell);
          repair_count += buffer_count;
          if (logger_->debugCheck(RSZ, "repair_hold", 4)) {
            // Check that no setup violations are introduced.
            updateParasitics();
            Slack drvr_setup_slack1 = sta_->vertexSlack(vertex, max_);
            if (drvr_setup_slack1 < 0
                && drvr_setup_slack1 < drvr_setup_slack)
              printf("%s %s -> %s\n", vertex->name(network_),
                     delayAsString(drvr_setup_slack, sta_, 3),
                     delayAsString(drvr_setup_slack1, sta_, 3));
          }
          if (inserted_buffer_count_ > max_buffer_count
              || overMaxArea())
            return repair_count;
        }
      }
    }
  }
  return repair_count;
}

VertexSet
Resizer::findHoldFanins(VertexSet &ends)
{
  SearchPredNonReg2 pred(sta_);
  BfsBkwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex *vertex : ends)
    iter.enqueueAdjacentVertices(vertex);

  VertexSet fanins;
  while (iter.hasNext()) {
    Vertex *fanin = iter.next();
    if (!sta_->isClock(fanin->pin())) {
      if (fanin->isDriver(network_))
        fanins.insert(fanin);
      iter.enqueueAdjacentVertices(fanin);
    }
  }
  return fanins;
}

VertexSeq
Resizer::sortHoldFanins(VertexSet &fanins)
{
  VertexSeq sorted_fanins;
  for(Vertex *vertex : fanins)
    sorted_fanins.push_back(vertex);

  sort(sorted_fanins, [&](Vertex *v1, Vertex *v2)
                      { float s1 = sta_->vertexSlack(v1, MinMax::min());
                        float s2 = sta_->vertexSlack(v2, MinMax::min());
                        if (fuzzyEqual(s1, s2)) {
                          float gap1 = slackGap(v1);
                          float gap2 = slackGap(v2);
                          // Break ties based on the hold/setup gap.
                          if (fuzzyEqual(gap1, gap2))
                            return v1->level() > v2->level();
                          else
                            return gap1 > gap2;
                        }
                        else
                          return s1 < s2;});
  if (logger_->debugCheck(RSZ, "repair_hold", 4)) {
    printf("Sorted fanins");
    printf("     hold_slack  slack_gap  level");
    for(Vertex *vertex : sorted_fanins)
      printf("%s %s %s %d",
             vertex->name(network_),
             delayAsString(sta_->vertexSlack(vertex, MinMax::min()), sta_, 3),
             delayAsString(slackGap(vertex), sta_, 3),
             vertex->level());
  }
  return sorted_fanins;
}

void
Resizer::makeHoldDelay(Vertex *drvr,
                       int buffer_count,
                       PinSeq &load_pins,
                       bool loads_have_out_port,
                       LibertyCell *buffer_cell)
{
  Pin *drvr_pin = drvr->pin();
  Instance *parent = db_network_->topInstance();
  Net *drvr_net = network_->isTopLevelPort(drvr_pin)
    ? db_network_->net(db_network_->term(drvr_pin))
    : db_network_->net(drvr_pin);
  Net *in_net, *out_net;
  if (loads_have_out_port) {
    // Verilog uses nets as ports, so the net connected to an output port has
    // to be preserved.
    // Move the driver pin over to gensym'd net.
    in_net = makeUniqueNet();
    Port *drvr_port = network_->port(drvr_pin);
    Instance *drvr_inst = network_->instance(drvr_pin);
    sta_->disconnectPin(drvr_pin);
    sta_->connectPin(drvr_inst, drvr_port, in_net);
    out_net = drvr_net;
  }
  else {
    in_net = drvr_net;
    out_net = makeUniqueNet();
  }

  parasiticsInvalid(in_net);
  // Spread buffers between driver and load center.
  Point drvr_loc = db_network_->location(drvr_pin);
  Point load_center = findCenter(load_pins);
  int dx = (load_center.x() - drvr_loc.x()) / (buffer_count + 1);
  int dy = (load_center.y() - drvr_loc.y()) / (buffer_count + 1);

  Net *buf_in_net = in_net;
  Instance *buffer = nullptr;
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  // drvr_pin->in_net->hold_buffer1->net2->hold_buffer2->out_net...->load_pins
  for (int i = 0; i < buffer_count; i++) {
    Net *buf_out_net = (i == buffer_count - 1) ? out_net : makeUniqueNet();
    // drvr_pin->drvr_net->hold_buffer->net2->load_pins
    string buffer_name = makeUniqueInstName("hold");
    buffer = makeInstance(buffer_cell, buffer_name.c_str(),
                          parent);
    journalMakeBuffer(buffer);
    inserted_buffer_count_++;
    designAreaIncr(area(db_network_->cell(buffer_cell)));

    sta_->connectPin(buffer, input, buf_in_net);
    sta_->connectPin(buffer, output, buf_out_net);
    Point buffer_loc(drvr_loc.x() + dx * i,
                     drvr_loc.y() + dy * i);
    setLocation(buffer, buffer_loc);
    buf_in_net = buf_out_net;
    parasiticsInvalid(buf_out_net);
  }

  for (Pin *load_pin : load_pins) {
    Net *load_net = network_->isTopLevelPort(load_pin)
      ? network_->net(network_->term(load_pin))
      : network_->net(load_pin);
    if (load_net != out_net) {
      Instance *load = db_network_->instance(load_pin);
      Port *load_port = db_network_->port(load_pin);
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, out_net);
    }
  }
  Pin *buffer_out_pin = network_->findPin(buffer, output);
  resizeToTargetSlew(buffer_out_pin, false);
}

Point
Resizer::findCenter(PinSeq &pins)
{
  long sum_x = 0;
  long sum_y = 0;
  for (Pin *pin : pins) {
    Point loc = db_network_->location(pin);
    sum_x += loc.x();
    sum_y += loc.y();
  }
  return Point(sum_x / pins.size(), sum_y / pins.size());
}

// Gap between min setup and hold slacks.
// This says how much head room there is for adding delay to fix a
// hold violation before violating a setup check.
Slack
Resizer::slackGap(Slacks &slacks)
{
  return min(slacks[RiseFall::riseIndex()][MinMax::maxIndex()]
             - slacks[RiseFall::riseIndex()][MinMax::minIndex()],
             slacks[RiseFall::fallIndex()][MinMax::maxIndex()]
             - slacks[RiseFall::fallIndex()][MinMax::minIndex()]);
}

Slack
Resizer::slackGap(Vertex *vertex)
{
  Slacks slacks;
  sta_->vertexSlacks(vertex, slacks);
  return slackGap(slacks);
}

int
Resizer::fanout(Vertex *vertex)
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    edge_iter.next();
    fanout++;
  }
  return fanout;
}

////////////////////////////////////////////////////////////////

void
Resizer::reportLongWires(int count,
                         int digits)
{
  graph_ = sta_->ensureGraph();
  sta_->ensureClkNetwork();
  VertexSeq drvrs;
  findLongWires(drvrs);
  report_->reportLine("Driver    length delay");
  const Corner *corner = sta_->cmdCorner();
  double wire_res = wireSignalResistance(corner);
  double wire_cap = wireSignalCapacitance(corner);
  int i = 0;
  for (Vertex *drvr : drvrs) {
    Pin *drvr_pin = drvr->pin();
    double wire_length = dbuToMeters(maxLoadManhattenDistance(drvr));
    double steiner_length = dbuToMeters(findMaxSteinerDist(drvr));
    double delay = (wire_length * wire_res) * (wire_length * wire_cap) * 0.5;
    report_->reportLine("%s manhtn %s steiner %s %s",
                        sdc_network_->pathName(drvr_pin),
                        units_->distanceUnit()->asString(wire_length, 1),
                        units_->distanceUnit()->asString(steiner_length, 1),
                        delayAsString(delay, sta_, digits));
    if (i == count)
      break;
    i++;
  }
}

typedef std::pair<Vertex*, int> DrvrDist;

void
Resizer::findLongWires(VertexSeq &drvrs)
{
  Vector<DrvrDist> drvr_dists;
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex *vertex = vertex_iter.next();
    if (vertex->isDriver(network_)) {
      Pin *pin = vertex->pin();
      // Hands off the clock nets.
      if (!sta_->isClock(pin)
          && !vertex->isConstant()
          && !vertex->isDisabledConstraint())
        drvr_dists.push_back(DrvrDist(vertex, maxLoadManhattenDistance(vertex)));
    }
  }
  sort(drvr_dists, [](const DrvrDist &drvr_dist1,
                          const DrvrDist &drvr_dist2) {
                    return drvr_dist1.second > drvr_dist2.second;
                  });
  drvrs.reserve(drvr_dists.size());
  for (DrvrDist &drvr_dist : drvr_dists)
    drvrs.push_back(drvr_dist.first);
}

void
Resizer::findLongWiresSteiner(VertexSeq &drvrs)
{
  Vector<DrvrDist> drvr_dists;
  VertexIterator vertex_iter(graph_);
  while (vertex_iter.hasNext()) {
    Vertex *vertex = vertex_iter.next();
    if (vertex->isDriver(network_)) {
      Pin *pin = vertex->pin();
      // Hands off the clock nets.
      if (!sta_->isClock(pin)
          && !vertex->isConstant())
        drvr_dists.push_back(DrvrDist(vertex, findMaxSteinerDist(vertex)));
    }
  }
  sort(drvr_dists, [](const DrvrDist &drvr_dist1,
                          const DrvrDist &drvr_dist2) {
                     return drvr_dist1.second > drvr_dist2.second;
                   });
  drvrs.reserve(drvr_dists.size());
  for (DrvrDist &drvr_dist : drvr_dists)
    drvrs.push_back(drvr_dist.first);
}

// Find the maximum distance along steiner tree branches from
// the driver to loads (in dbu).
int
Resizer::findMaxSteinerDist(Vertex *drvr)
{
  Pin *drvr_pin = drvr->pin();
  SteinerTree *tree = makeSteinerTree(drvr_pin, true, max_steiner_pin_count_,
                                      stt_builder_, db_network_, logger_);
  if (tree) {
    int dist = findMaxSteinerDist(tree);
    delete tree;
    return dist;
  }
  return 0;
}

int
Resizer::findMaxSteinerDist(SteinerTree *tree)
{
  SteinerPt drvr_pt = tree->drvrPt(network_);
  if (drvr_pt == SteinerTree::null_pt)
    return 0;
  else
    return findMaxSteinerDist(tree, drvr_pt, 0);
}

// DFS of steiner tree.
int
Resizer::findMaxSteinerDist(SteinerTree *tree,
                            SteinerPt pt,
                            int dist_from_drvr)
{
  const PinSeq *pins = tree->pins(pt);
  if (pins) {
    for (const Pin *pin : *pins) {
      if (db_network_->isLoad(pin))
        return dist_from_drvr;
    }
  }
  Point loc = tree->location(pt);
  SteinerPt left = tree->left(pt);
  int left_max = 0;
  if (left != SteinerTree::null_pt) {
    int left_dist = Point::manhattanDistance(loc, tree->location(left));
    left_max = findMaxSteinerDist(tree, left, dist_from_drvr + left_dist);
  }
  SteinerPt right = tree->right(pt);
  int right_max = 0;
  if (right != SteinerTree::null_pt) {
    int right_dist = Point::manhattanDistance(loc, tree->location(right));
    right_max = findMaxSteinerDist(tree, right, dist_from_drvr + right_dist);
  }
  return max(left_max, right_max);
}

double
Resizer::maxLoadManhattenDistance(const Net *net)
{
  NetPinIterator *pin_iter = network_->pinIterator(net);
  int max_dist = 0;
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isDriver(pin)) {
      Vertex *drvr = graph_->pinDrvrVertex(pin);
      if (drvr) {
        int dist = maxLoadManhattenDistance(drvr);
        max_dist = max(max_dist, dist);
      }
    }
  }
  delete pin_iter;
  return dbuToMeters(max_dist);
}

int
Resizer::maxLoadManhattenDistance(Vertex *drvr)
{
  int max_dist = 0;
  Point drvr_loc = db_network_->location(drvr->pin());
  VertexOutEdgeIterator edge_iter(drvr, graph_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    Vertex *load = edge->to(graph_);
    Point load_loc = db_network_->location(load->pin());
    int dist = Point::manhattanDistance(drvr_loc, load_loc);
    max_dist = max(max_dist, dist);
  }
  return max_dist;
}

////////////////////////////////////////////////////////////////

NetSeq *
Resizer::findFloatingNets()
{
  NetSeq *floating_nets = new NetSeq;
  NetIterator *net_iter = network_->netIterator(network_->topInstance());
  while (net_iter->hasNext()) {
    Net *net = net_iter->next();
    PinSeq loads;
    PinSeq drvrs;
    PinSet visited_drvrs;
    FindNetDrvrLoads visitor(nullptr, visited_drvrs, loads, drvrs, network_);
    network_->visitConnectedPins(net, visitor);
    if (drvrs.size() == 0 && loads.size() > 0)
      floating_nets->push_back(net);
  }
  delete net_iter;
  sort(floating_nets, sta::NetPathNameLess(network_));
  return floating_nets;
}

////////////////////////////////////////////////////////////////

string
Resizer::makeUniqueNetName()
{
  string node_name;
  Instance *top_inst = network_->topInstance();
  do 
    stringPrint(node_name, "net%d", unique_net_index_++);
  while (network_->findNet(top_inst, node_name.c_str()));
  return node_name;
}

Net *
Resizer::makeUniqueNet()
{
  string net_name = makeUniqueNetName();
  Instance *parent = db_network_->topInstance();
  return db_network_->makeNet(net_name.c_str(), parent);
}

string
Resizer::makeUniqueInstName(const char *base_name)
{
  return makeUniqueInstName(base_name, false);
}

string
Resizer::makeUniqueInstName(const char *base_name,
                            bool underscore)
{
  string inst_name;
  do 
    stringPrint(inst_name, underscore ? "%s_%d" : "%s%d",
                base_name, unique_inst_index_++);
  while (network_->findInstance(inst_name.c_str()));
  return inst_name;
}

float
Resizer::portFanoutLoad(LibertyPort *port)
{
  float fanout_load;
  bool exists;
  port->fanoutLoad(fanout_load, exists);
  if (!exists) {
    LibertyLibrary *lib = port->libertyLibrary();
    lib->defaultFanoutLoad(fanout_load, exists);
  }
  if (exists)
    return fanout_load;
  else
    return 0.0;
}

float
Resizer::bufferDelay(LibertyCell *buffer_cell,
                     const RiseFall *rf,
                     float load_cap,
                     const DcalcAnalysisPt *dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return gate_delays[rf->index()];
}

float
Resizer::bufferDelay(LibertyCell *buffer_cell,
                     float load_cap,
                     const DcalcAnalysisPt *dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return max(gate_delays[RiseFall::riseIndex()],
             gate_delays[RiseFall::fallIndex()]);
}

// Self delay; buffer -> buffer
float
Resizer::bufferSelfDelay(LibertyCell *buffer_cell)
{
  const DcalcAnalysisPt *dcalc_ap = sta_->cmdCorner()->findDcalcAnalysisPt(max_);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  float load_cap = input->capacitance();
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return max(gate_delays[RiseFall::riseIndex()],
             gate_delays[RiseFall::fallIndex()]);
}

float
Resizer::bufferSelfDelay(LibertyCell *buffer_cell,
                         const RiseFall *rf)
{
  const DcalcAnalysisPt *dcalc_ap = sta_->cmdCorner()->findDcalcAnalysisPt(max_);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  float load_cap = input->capacitance();
  gateDelays(output, load_cap, dcalc_ap, gate_delays, slews);
  return gate_delays[rf->index()];
}

// Rise/fall delays across all timing arcs into drvr_port.
// Uses target slew for input slew.
void
Resizer::gateDelays(LibertyPort *drvr_port,
                    float load_cap,
                    const DcalcAnalysisPt *dcalc_ap,
                    // Return values.
                    ArcDelay delays[RiseFall::index_count],
                    Slew slews[RiseFall::index_count])
{
  for (int rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = -INF;
    slews[rf_index] = -INF;
  }
  const Pvt *pvt = dcalc_ap->operatingConditions();
  LibertyCell *cell = drvr_port->libertyCell();
  LibertyCellTimingArcSetIterator set_iter(cell);
  while (set_iter.hasNext()) {
    TimingArcSet *arc_set = set_iter.next();
    if (arc_set->to() == drvr_port
        && !arc_set->role()->isTimingCheck()) {
      TimingArcSetArcIterator arc_iter(arc_set);
      while (arc_iter.hasNext()) {
        TimingArc *arc = arc_iter.next();
        RiseFall *in_rf = arc->fromTrans()->asRiseFall();
        int out_rf_index = arc->toTrans()->asRiseFall()->index();
        float in_slew = tgt_slews_[in_rf->index()];
        ArcDelay gate_delay;
        Slew drvr_slew;
        arc_delay_calc_->gateDelay(cell, arc, in_slew, load_cap,
                                   nullptr, 0.0, pvt, dcalc_ap,
                                   gate_delay,
                                   drvr_slew);
        delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
        slews[out_rf_index] = max(slews[out_rf_index], drvr_slew);
      }
    }
  }
}

ArcDelay
Resizer::gateDelay(LibertyPort *drvr_port,
                   const RiseFall *rf,
                   float load_cap,
                   const DcalcAnalysisPt *dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  return delays[rf->index()];
}

ArcDelay
Resizer::gateDelay(LibertyPort *drvr_port,
                   float load_cap,
                   const DcalcAnalysisPt *dcalc_ap)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, dcalc_ap, delays, slews);
  return max(delays[RiseFall::riseIndex()], delays[RiseFall::fallIndex()]);
}

////////////////////////////////////////////////////////////////

double
Resizer::findMaxWireLength()
{
  double max_length = INF;
  for (const Corner *corner : *sta_->corners()) {
    if (wireSignalResistance(corner) > 0.0) {
      for (LibertyCell *buffer_cell : buffer_cells_) {
        double buffer_length = findMaxWireLength(buffer_cell, corner);
        max_length = min(max_length, buffer_length);
      }
    }
  }
  return max_length;
}

// Find the max wire length before it is faster to split the wire
// in half with a buffer (in meters).
double
Resizer::findMaxWireLength(LibertyCell *buffer_cell,
                           const Corner *corner)
{
  ensureBlock();
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  return findMaxWireLength(drvr_port, corner);
}

double
Resizer::findMaxWireLength(LibertyPort *drvr_port,
                           const Corner *corner)
{
  LibertyCell *cell = drvr_port->libertyCell();
  if (db_network_->staToDb(cell) == nullptr)
    logger_->error(RSZ, 70, "no LEF cell for {}.", cell->name());
  double drvr_r = drvr_port->driveResistance();
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  double wire_length2 = drvr_r / wireSignalResistance(corner);
  double tol = .01; // 1%
  double diff1 = splitWireDelayDiff(wire_length2, cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff1 < 0.0) {
      wire_length1 = wire_length2;
      wire_length2 *= 2;
      diff1 = splitWireDelayDiff(wire_length2, cell);
    }
    else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff2 = splitWireDelayDiff(wire_length3, cell);
      if (diff2 < 0.0) {
        wire_length1 = wire_length3;
      }
      else {
        wire_length2 = wire_length3;
        diff1 = diff2;
      }
    }
  }
  return wire_length1;
}

// objective function
double
Resizer::splitWireDelayDiff(double wire_length,
                            LibertyCell *buffer_cell)
{
  Delay delay1, delay2;
  Slew slew1, slew2;
  bufferWireDelay(buffer_cell, wire_length, delay1, slew1);
  bufferWireDelay(buffer_cell, wire_length / 2, delay2, slew2);
  return delay1 - delay2 * 2;
}

void
Resizer::bufferWireDelay(LibertyCell *buffer_cell,
                         double wire_length, // meters
                         // Return values.
                         Delay &delay,
                         Slew &slew)
{
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  return cellWireDelay(drvr_port, load_port, wire_length, delay, slew);
}

// Cell delay plus wire delay.
// Use target slew for input slew.
// drvr_port and load_port do not have to be the same liberty cell.
void
Resizer::cellWireDelay(LibertyPort *drvr_port,
                       LibertyPort *load_port,
                       double wire_length, // meters
                       // Return values.
                       Delay &delay,
                       Slew &slew)
{
  // Make a (hierarchical) block to use as a scratchpad.
  dbBlock *block = dbBlock::create(block_, "wire_delay", '/');
  dbSta *sta = makeBlockSta(openroad_, block);
  Parasitics *parasitics = sta->parasitics();
  Network *network = sta->network();
  ArcDelayCalc *arc_delay_calc = sta->arcDelayCalc();
  Corners *corners = sta->corners();
  corners->copy(sta_->corners());

  Instance *top_inst = network->topInstance();
  // Tmp net for parasitics to live on.
  Net *net = sta->makeNet("wire", top_inst);
  LibertyCell *drvr_cell = drvr_port->libertyCell();
  LibertyCell *load_cell = load_port->libertyCell();
  Instance *drvr = sta->makeInstance("drvr", drvr_cell, top_inst);
  Instance *load = sta->makeInstance("load", load_cell, top_inst);
  sta->connectPin(drvr, drvr_port, net);
  sta->connectPin(load, load_port, net);
  Pin *drvr_pin = network->findPin(drvr, drvr_port);
  Pin *load_pin = network->findPin(load, load_port);

  // Max rise/fall delays.
  delay = -INF;
  slew = -INF;

  for (Corner *corner : *corners) {
    const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
    const Pvt *pvt = dcalc_ap->operatingConditions();

    makeWireParasitic(net, drvr_pin, load_pin, wire_length, corner, parasitics);
    // Let delay calc reduce parasitic network as it sees fit.
    Parasitic *drvr_parasitic = arc_delay_calc->findParasitic(drvr_pin,
                                                              RiseFall::rise(),
                                                              dcalc_ap);
    LibertyCellTimingArcSetIterator set_iter(drvr_cell);
    while (set_iter.hasNext()) {
      TimingArcSet *arc_set = set_iter.next();
      if (arc_set->to() == drvr_port) {
        TimingArcSetArcIterator arc_iter(arc_set);
        while (arc_iter.hasNext()) {
          TimingArc *arc = arc_iter.next();
          RiseFall *in_rf = arc->fromTrans()->asRiseFall();
          double in_slew = tgt_slews_[in_rf->index()];
          ArcDelay gate_delay;
          Slew drvr_slew;
          arc_delay_calc->gateDelay(drvr_cell, arc, in_slew, 0.0,
                                    drvr_parasitic, 0.0, pvt, dcalc_ap,
                                    gate_delay,
                                    drvr_slew);
          ArcDelay wire_delay;
          Slew load_slew;
          arc_delay_calc->loadDelay(load_pin, wire_delay, load_slew);
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
  delete sta;
  dbBlock::destroy(block);
}

void
Resizer::makeWireParasitic(Net *net,
                           Pin *drvr_pin,
                           Pin *load_pin,
                           double wire_length, // meters
                           const Corner *corner,
                           Parasitics *parasitics)
{
  const ParasiticAnalysisPt *parasitics_ap =
    corner->findParasiticAnalysisPt(max_);
  Parasitic *parasitic = parasitics->makeParasiticNetwork(net, false,
                                                          parasitics_ap);
  ParasiticNode *n1 = parasitics->ensureParasiticNode(parasitic, drvr_pin);
  ParasiticNode *n2 = parasitics->ensureParasiticNode(parasitic, load_pin);
  double wire_cap = wire_length * wireSignalCapacitance(corner);
  double wire_res = wire_length * wireSignalResistance(corner);
  parasitics->incrCap(n1, wire_cap / 2.0, parasitics_ap);
  parasitics->makeResistor(nullptr, n1, n2, wire_res, parasitics_ap);
  parasitics->incrCap(n2, wire_cap / 2.0, parasitics_ap);
}

////////////////////////////////////////////////////////////////

double
Resizer::findMaxSlewWireLength(LibertyPort *drvr_port,
                               LibertyPort *load_port,
                               double max_slew,
                               const Corner *corner)
{
  ensureBlock();
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  double wire_length2 = std::sqrt(max_slew /(wireSignalResistance(corner)
                                             * wireSignalCapacitance(corner)));
  double tol = .01; // 1%
  double diff1 = maxSlewWireDiff(drvr_port, load_port, wire_length2, max_slew);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff1 < 0.0) {
      wire_length1 = wire_length2;
      wire_length2 *= 2;
      diff1 = maxSlewWireDiff(drvr_port, load_port, wire_length2, max_slew);
    }
    else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff2 = maxSlewWireDiff(drvr_port, load_port, wire_length3, max_slew);
      if (diff2 < 0.0) {
        wire_length1 = wire_length3;
      }
      else {
        wire_length2 = wire_length3;
        diff1 = diff2;
      }
    }
  }
  return wire_length1;
}

// objective function
double
Resizer::maxSlewWireDiff(LibertyPort *drvr_port,
                         LibertyPort *load_port,
                         double wire_length,
                         double max_slew)
{
  Delay delay;
  Slew slew;
  cellWireDelay(drvr_port, load_port, wire_length, delay, slew);
  return slew - max_slew;
}

////////////////////////////////////////////////////////////////

double
Resizer::designArea()
{
  ensureBlock();
  ensureDesignArea();
  return design_area_;
}

void
Resizer::designAreaIncr(float delta)
{
  design_area_ += delta;
}

void
Resizer::ensureDesignArea()
{
  if (block_) {
    design_area_ = 0.0;
    for (dbInst *inst : block_->getInsts()) {
      dbMaster *master = inst->getMaster();
      // Don't count fillers otherwise you'll always get 100% utilization
      if (!master->isFiller()) {
        design_area_ += area(master);
      }
    }
  }
}

int
Resizer::fanout(Pin *drvr_pin)
{
  int fanout = 0;
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(drvr_pin);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (pin != drvr_pin)
      fanout++;
  }
  delete pin_iter;
  return fanout;
}

bool
Resizer::isFuncOneZero(const Pin *drvr_pin)
{
  LibertyPort *port = network_->libertyPort(drvr_pin);
  if (port) {
    FuncExpr *func = port->function();
    return func && (func->op() == FuncExpr::op_zero
                    || func->op() == FuncExpr::op_one);
  }
  return false;
}

////////////////////////////////////////////////////////////////

void
Resizer::repairClkInverters()
{
  // Abbreviated copyState
  db_network_ = sta_->getDbNetwork();
  sta_->ensureLevelized();
  graph_ = sta_->graph();
  ensureBlock();
  ensureDesignArea();
  for (Instance *inv : findClkInverters())
    cloneClkInverter(inv);
}

InstanceSeq
Resizer::findClkInverters()
{
  InstanceSeq clk_inverters;
  ClkArrivalSearchPred srch_pred(this);
  BfsFwdIterator bfs(BfsIndex::other, &srch_pred, this);
  for (Clock *clk : sdc_->clks()) {
    for (Pin *pin : clk->leafPins()) {
      Vertex *vertex = graph_->pinDrvrVertex(pin);
      bfs.enqueue(vertex);
    }
  }
  while (bfs.hasNext()) {
    Vertex *vertex = bfs.next();
    const Pin *pin = vertex->pin();
    Instance *inst = network_->instance(pin);
    LibertyCell *lib_cell = network_->libertyCell(inst);
    if (vertex->isDriver(network_)
        && lib_cell
        && lib_cell->isInverter()) {
      clk_inverters.push_back(inst);
      debugPrint(logger_, RSZ, "repair_clk_inverters", 2, "inverter {}",
                 network_->pathName(inst));
    }
    if (!vertex->isRegClk())
      bfs.enqueueAdjacentVertices(vertex);
  }
  return clk_inverters;
}

void
Resizer::cloneClkInverter(Instance *inv)
{
  LibertyCell *inv_cell = network_->libertyCell(inv);
  LibertyPort *in_port, *out_port;
  inv_cell->bufferPorts(in_port, out_port);
  Pin *in_pin = network_->findPin(inv, in_port);
  Pin *out_pin = network_->findPin(inv, out_port);
  Net *in_net = network_->net(in_pin);
  dbNet *in_net_db = db_network_->staToDb(in_net);
  Net *out_net = network_->isTopLevelPort(out_pin)
    ? network_->net(network_->term(out_pin))
    : network_->net(out_pin);
  if (out_net) {
    const char *inv_name = network_->name(inv);
    Instance *top_inst = network_->topInstance();
    NetConnectedPinIterator *load_iter = network_->pinIterator(out_net);
    while (load_iter->hasNext()) {
      Pin *load_pin = load_iter->next();
      if (load_pin != out_pin) {
        string clone_name = makeUniqueInstName(inv_name, true);
        Instance *clone = makeInstance(inv_cell, clone_name.c_str(),
                                       top_inst);
        Point clone_loc = db_network_->location(load_pin);
        journalMakeBuffer(clone);
        setLocation(clone, clone_loc);

        Net *clone_out_net = makeUniqueNet();
        dbNet *clone_out_net_db = db_network_->staToDb(clone_out_net);
        clone_out_net_db->setSigType(in_net_db->getSigType());

        Instance *load = network_->instance(load_pin);
        sta_->connectPin(clone, in_port, in_net);
        sta_->connectPin(clone, out_port, clone_out_net);

        // Connect load to clone
        sta_->disconnectPin(load_pin);
        Port *load_port = network_->port(load_pin);
        sta_->connectPin(load, load_port, clone_out_net);
      }
    }
    delete load_iter;

    // Delete inv
    sta_->disconnectPin(in_pin);
    sta_->disconnectPin(out_pin);
    sta_->deleteNet(out_net);
    parasitics_invalid_.erase(out_net);
    sta_->deleteInstance(inv);
  }
}

////////////////////////////////////////////////////////////////

// Journal to roll back changes (OpenDB not up to the task).
void
Resizer::journalBegin()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  resized_inst_map_.clear();
  inserted_buffers_.clear();
}

void
Resizer::journalInstReplaceCellBefore(Instance *inst)
{
  LibertyCell *lib_cell = network_->libertyCell(inst);
  debugPrint(logger_, RSZ, "journal", 1, "journal replace {} ({})",
             network_->pathName(inst),
             lib_cell->name());
  // Do not clobber an existing checkpoint cell.
  if (!resized_inst_map_.hasKey(inst))
    resized_inst_map_[inst] = lib_cell;
}

void
Resizer::journalMakeBuffer(Instance *buffer)
{
  debugPrint(logger_, RSZ, "journal", 1, "journal make_buffer {}",
             network_->pathName(buffer));
  inserted_buffers_.insert(buffer);
}

void
Resizer::journalRestore()
{
  for (auto inst_cell : resized_inst_map_) {
    Instance *inst = inst_cell.first;
    if (!inserted_buffers_.hasKey(inst)) {
      LibertyCell *lib_cell = inst_cell.second;
      debugPrint(logger_, RSZ, "journal", 1, "journal restore {} ({})",
                 network_->pathName(inst),
                 lib_cell->name());
      replaceCell(inst, lib_cell, false);
      resize_count_--;
    }
  }
  for (Instance *buffer : inserted_buffers_) {
    debugPrint(logger_, RSZ, "journal", 1, "journal remove {}",
               network_->pathName(buffer));
    removeBuffer(buffer);
    inserted_buffer_count_--;
  }
}

////////////////////////////////////////////////////////////////

class SteinerRenderer : public gui::Renderer
{
public:
  SteinerRenderer();
  void highlight(SteinerTree *tree);
  virtual void drawObjects(gui::Painter& /* painter */) override;

private:
  SteinerTree *tree_;
};

void
Resizer::highlightSteiner(const Pin *drvr)
{
  if (gui::Gui::enabled()) {
    if (steiner_renderer_ == nullptr) {
      steiner_renderer_ = new SteinerRenderer();
      gui_->registerRenderer(steiner_renderer_);
    }
    SteinerTree *tree = nullptr;
    if (drvr)
      tree = makeSteinerTree(drvr, false, max_steiner_pin_count_,
                             stt_builder_, db_network_, logger_);
    steiner_renderer_->highlight(tree);
  }
}

SteinerRenderer::SteinerRenderer() :
  tree_(nullptr)
{
}

void
SteinerRenderer::highlight(SteinerTree *tree)
{
  tree_ = tree;
}

void
SteinerRenderer::drawObjects(gui::Painter &painter)
{
  if (tree_) {
    painter.setPen(gui::Painter::red, true);
    for (int i = 0 ; i < tree_->branchCount(); ++i) {
      Point pt1, pt2;
      int steiner_pt1, steiner_pt2;
      int wire_length;
      tree_->branch(i, pt1, steiner_pt1, pt2, steiner_pt2, wire_length);
      painter.drawLine(pt1, pt2);
    }
  }
}

////////////////////////////////////////////////////////////////

PinSet
Resizer::findFaninFanouts(PinSet *end_pins)
{
  // Abbreviated copyState
  db_network_ = sta_->getDbNetwork();
  sta_->ensureLevelized();
  graph_ = sta_->graph();

  VertexSet ends;
  for (Pin *pin : *end_pins) {
    Vertex *end = graph_->pinLoadVertex(pin);
    ends.insert(end);
  }
  PinSet fanin_fanout_pins;
  VertexSet fanin_fanouts = findFaninFanouts(ends);
  for (Vertex *vertex : fanin_fanouts)
    fanin_fanout_pins.insert(vertex->pin());
  return fanin_fanout_pins;
}

VertexSet
Resizer::findFaninFanouts(VertexSet &ends)
{
  // Search backwards from ends to fanin register outputs and input ports.
  VertexSet fanin_roots = findFaninRoots(ends);
  // Search forward from register outputs.
  VertexSet fanouts = findFanouts(fanin_roots);
  return fanouts;
}

// Find source pins for logic fanin of ends.
PinSet
Resizer::findFanins(PinSet *end_pins)
{
  // Abbreviated copyState
  db_network_ = sta_->getDbNetwork();
  sta_->ensureLevelized();
  graph_ = sta_->graph();

  VertexSet ends;
  for (Pin *pin : *end_pins) {
    Vertex *end = graph_->pinLoadVertex(pin);
    ends.insert(end);
  }

  SearchPredNonReg2 pred(sta_);
  BfsBkwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex *vertex : ends)
    iter.enqueueAdjacentVertices(vertex);

  PinSet fanins;
  while (iter.hasNext()) {
    Vertex *vertex = iter.next();
    if (isRegOutput(vertex)
        || network_->isTopLevelPort(vertex->pin()))
      continue;
    else {
      iter.enqueueAdjacentVertices(vertex);
      fanins.insert(vertex->pin());
    }
  }
  return fanins;
}

// Find roots for logic fanin of ends.
VertexSet
Resizer::findFaninRoots(VertexSet &ends)
{
  SearchPredNonReg2 pred(sta_);
  BfsBkwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex *vertex : ends)
    iter.enqueueAdjacentVertices(vertex);

  VertexSet roots;
  while (iter.hasNext()) {
    Vertex *vertex = iter.next();
    if (isRegOutput(vertex)
        || network_->isTopLevelPort(vertex->pin()))
      roots.insert(vertex);
    else
      iter.enqueueAdjacentVertices(vertex);
  }
  return roots;
}

bool
Resizer::isRegOutput(Vertex *vertex)
{
  LibertyPort *port = network_->libertyPort(vertex->pin());
  if (port) {
    LibertyCell *cell = port->libertyCell();
    LibertyCellTimingArcSetIterator arc_set_iter(cell, nullptr, port);
    while (arc_set_iter.hasNext()) {
      TimingArcSet *arc_set = arc_set_iter.next();
      if (arc_set->role()->genericRole() == TimingRole::regClkToQ())
        return true;
    }
  }
  return false;
}

VertexSet
Resizer::findFanouts(VertexSet &reg_outs)
{
  VertexSet fanouts;
  sta::SearchPredNonLatch2 pred(sta_);
  BfsFwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex *reg_out : reg_outs)
    iter.enqueueAdjacentVertices(reg_out);

  while (iter.hasNext()) {
    Vertex *vertex = iter.next();
    if (!isRegister(vertex)) {
      fanouts.insert(vertex);
      iter.enqueueAdjacentVertices(vertex);
    }
  }
  return fanouts;
}

bool
Resizer::isRegister(Vertex *vertex)
{
  LibertyPort *port = network_->libertyPort(vertex->pin());
  if (port) {
    LibertyCell *cell = port->libertyCell();
    return cell && cell->hasSequentials();
  }
  return false;
}

Instance *Resizer::makeInstance(LibertyCell *cell,
                                const char *name,
                                Instance *parent)
{
  debugPrint(logger_, RSZ, "make_instance", 1, "make instance {}", name);
  Instance *inst = db_network_->makeInstance(cell, name, parent);
  dbInst *db_inst = db_network_->staToDb(inst);
  db_inst->setSourceType(odb::dbSourceType::TIMING);
  return inst;
}

} // namespace
