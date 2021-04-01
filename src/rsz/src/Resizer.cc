/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
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

#include "openroad/OpenRoad.hh"
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
#include "sta/Corner.hh"
#include "sta/PathVertex.hh"
#include "sta/SearchPred.hh"
#include "sta/Bfs.hh"
#include "sta/Search.hh"
#include "sta/PathRef.hh"
#include "sta/PathExpanded.hh"
#include "sta/StaMain.hh"
#include "sta/Fuzzy.hh"

// multi-corner support
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

using utl::RSZ;
using ord::closestPtInRect;

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
using sta::PinSet;
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
using sta::stringPrint;
using sta::Unit;

extern "C" {
extern int Rsz_Init(Tcl_Interp *interp);
}

Resizer::Resizer() :
  StaState(),
  wire_res_(0.0),
  wire_cap_(0.0),
  wire_clk_res_(0.0),
  wire_clk_cap_(0.0),
  max_area_(0.0),
  gui_(nullptr),
  sta_(nullptr),
  db_network_(nullptr),
  db_(nullptr),
  core_exists_(false),
  max_(MinMax::max()),
  have_estimated_parasitics_(false),
  target_load_map_(nullptr),
  level_drvr_vertices_valid_(false),
  tgt_slews_{0.0, 0.0},
  tgt_slew_corner_(nullptr),
  tgt_slew_dcalc_ap_(nullptr),
  unique_net_index_(1),
  unique_inst_index_(1),
  resize_count_(0),
  design_area_(0.0),
  steiner_renderer_(nullptr)
{
}

void
Resizer::init(OpenRoad *openroad,
              Tcl_Interp *interp,
              Logger *logger,
              Gui *gui,
              dbDatabase *db,
              dbSta *sta)
{
  openroad_ = openroad;
  logger_ = logger;
  gui_ = gui;
  db_ = db;
  block_ = nullptr;
  sta_ = sta;
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
      if (removeBuffer(buffer))
        remove_count++;
    }
  }
  level_drvr_vertices_valid_ = false;
  logger_->info(RSZ, 26, "Removed {} buffers.", remove_count);
}

bool
Resizer::removeBuffer(Instance *buffer)
{
  LibertyCell *lib_cell = network_->libertyCell(buffer);
  LibertyPort *in_port, *out_port;
  lib_cell->bufferPorts(in_port, out_port);
  Pin *in_pin = db_network_->findPin(buffer, in_port);
  Pin *out_pin = db_network_->findPin(buffer, out_port);
  Net *in_net = db_network_->net(in_pin);
  Net *out_net = db_network_->net(out_pin);
  bool in_net_ports = hasTopLevelPort(in_net);
  bool out_net_ports = hasTopLevelPort(out_net);
  if (in_net_ports && out_net_ports) {
    // Verilog uses nets as ports, so the surviving net has to be
    // the one connected the port.
    logger_->warn(RSZ, 43, "Cannot remove buffers between net {} and {} because both nets have ports connected to them.",
                  sdc_network_->pathName(in_net),
                  sdc_network_->pathName(out_net));
    return false;
  }
  else {
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
    sta_->deleteInstance(buffer);
    return true;
  }
}

void
Resizer::setWireRC(float wire_res,
                   float wire_cap)
{
  wire_res_ = wire_res;
  wire_cap_ = wire_cap;
}

void
Resizer::setWireClkRC(float wire_res,
                      float wire_cap)
{
  wire_clk_res_ = wire_res;
  wire_clk_cap_ = wire_cap;
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

void
Resizer::findBuffers()
{
  buffer_lowest_drive_ = nullptr;
  float low_drive = -INF;
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary *lib = lib_iter->next();
    for (LibertyCell *buffer : *lib->buffers()) {
      if (!dontUse(buffer)
          && isLinkCell(buffer)) {
        buffer_cells_.push_back(buffer);

        LibertyPort *input, *output;
        buffer->bufferPorts(input, output);
        float buffer_drive = output->driveResistance();
        if (buffer_drive > low_drive) {
          low_drive = buffer_drive;
          buffer_lowest_drive_ = buffer;
        }
      }
    }
  }
  delete lib_iter;
  if (buffer_cells_.empty())
    logger_->error(RSZ, 22, "no buffers found.");
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
  InstancePinIterator *port_iter = network_->pinIterator(network_->topInstance());
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isInput()
        && !sta_->isClock(pin)
        && !isSpecial(net))
      bufferInput(pin, buffer_lowest_drive_);
  }
  delete port_iter;
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 27, "Inserted {} input buffers.", inserted_buffer_count_);
    level_drvr_vertices_valid_ = false;
  }
}
   
void
Resizer::bufferInput(Pin *top_pin,
                     LibertyCell *buffer_cell)
{
  Term *term = db_network_->term(top_pin);
  Net *input_net = db_network_->net(term);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  string buffer_name = makeUniqueInstName("input");
  Instance *parent = db_network_->topInstance();
  Net *buffer_out = makeUniqueNet();
  Instance *buffer = db_network_->makeInstance(buffer_cell,
                                               buffer_name.c_str(),
                                               parent);
  if (buffer) {
    journalMakeBuffer(buffer);
    Point pin_loc = db_network_->location(top_pin);
    Point buf_loc = core_exists_ ? closestPtInRect(core_, pin_loc) : pin_loc;
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
  }
}

void
Resizer::setLocation(Instance *inst,
                     Point pt)
{
  dbInst *dinst = db_network_->staToDb(inst);
  dinst->setPlacementStatus(dbPlacementStatus::PLACED);
  dinst->setLocation(pt.getX(), pt.getY());
}

void
Resizer::bufferOutputs()
{
  init();
  inserted_buffer_count_ = 0;
  InstancePinIterator *port_iter = network_->pinIterator(network_->topInstance());
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isOutput()
        && net
        && !isSpecial(net))
      bufferOutput(pin, buffer_lowest_drive_);
  }
  delete port_iter;
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 28, "Inserted {} output buffers.", inserted_buffer_count_);
    level_drvr_vertices_valid_ = false;
  }
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
  Instance *buffer = network->makeInstance(buffer_cell,
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
  }
}

////////////////////////////////////////////////////////////////

// Repair long wires, max slew, max capacitance, max fanout violations
// The whole enchilada.
void
Resizer::repairDesign(double max_wire_length) // zero for none (meters)
{
  int repair_count, slew_violations, cap_violations;
  int fanout_violations, length_violations;
  repairDesign(max_wire_length,
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

  int max_length = metersToDbu(max_wire_length);
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex *drvr = level_drvr_vertices_[i];
    Pin *drvr_pin = drvr->pin();
    Net *net = network_->isTopLevelPort(drvr_pin)
      ? network_->net(network_->term(drvr_pin))
      : network_->net(drvr_pin);
    if (net
        && !sta_->isClock(drvr_pin)
        // Exclude tie hi/low cells.
        && !isFuncOneZero(drvr_pin)
        && !isSpecial(net)) {
      repairNet(net, drvr, true, true, true, max_length, true,
                repair_count, slew_violations, cap_violations,
                fanout_violations, length_violations);
    }
  }
  ensureWireParasitics();

  if (inserted_buffer_count_ > 0)
    level_drvr_vertices_valid_ = false;
}

// repairDesign but restricted to clock network and
// no max_fanout/max_cap checks.
void
Resizer::repairClkNets(double max_wire_length) // meters
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
  for (Clock *clk : sdc_->clks()) {
    for (const Pin *clk_pin : *sta_->pins(clk)) {
      if (network_->isDriver(clk_pin)) {
        Net *net = network_->isTopLevelPort(clk_pin)
          ? network_->net(network_->term(clk_pin))
          : network_->net(clk_pin);
        Vertex *drvr = graph_->pinDrvrVertex(clk_pin);
        // Do not resize clock tree gates.
        repairNet(net, drvr, false, false, false, max_length, false,
                  repair_count, slew_violations, cap_violations,
                  fanout_violations, length_violations);
      }
    }
  }
  ensureWireParasitics();
  if (length_violations > 0)
    logger_->info(RSZ, 47, "Found {} long wires.", length_violations);
  if (inserted_buffer_count_ > 0) {
    logger_->info(RSZ, 48, "Inserted {} buffers in {} nets.",
                  inserted_buffer_count_,
                  repair_count);
    level_drvr_vertices_valid_ = false;
  }
}

// for debugging
void
Resizer::repairNet(Net *net,
                   double max_wire_length) // meters
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
    repairNet(net, drvr, true, true, true, max_length, true,
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
                   Vertex *drvr,
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
  SteinerTree *tree = makeSteinerTree(net, true, db_network_, logger_);
  if (tree) {
    Pin *drvr_pin = drvr->pin();
    debugPrint(logger_, RSZ, "repair_net", 1, "repair net {}",
               sdc_network_->pathName(drvr_pin));
    ensureWireParasitic(drvr_pin, net);
    graph_delay_calc_->findDelays(drvr);

    LibertyPort *buffer_input_port, *buffer_output_port;
    buffer_lowest_drive_->bufferPorts(buffer_input_port, buffer_output_port);
    float buf_cap_limit;
    bool buf_cap_limit_exists;
    buffer_output_port->capacitanceLimit(max_, buf_cap_limit, buf_cap_limit_exists);

    double drvr_max_cap = INF;
    float max_fanout = INF;
    bool repair_slew = false;
    bool repair_cap = false;
    bool repair_fanout = false;
    bool repair_wire = false;
    if (check_cap) {
      float cap, max_cap1, cap_slack;
      const Corner *corner1;
      const RiseFall *tr;
      sta_->checkCapacitance(drvr_pin, nullptr, max_,
                             corner1, tr, cap, max_cap1, cap_slack);
      if (cap_slack < 0.0) {
        drvr_max_cap = max_cap1;
        cap_violations++;
        repair_cap = true;
      }
    }
    if (check_fanout) {
      float fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, max_,
                        fanout, max_fanout, fanout_slack);
      if (fanout_slack < 0.0) {
        fanout_violations++;
        repair_fanout = true;
      }
    }
    int wire_length = findMaxSteinerDist(drvr, tree);
    if (max_length
        && wire_length > max_length) {
      length_violations++;
      repair_wire = true;
    }
    if (check_slew) {
      float slew, slew_slack, max_slew;
      checkSlew(drvr_pin, slew, max_slew, slew_slack);
      if (slew_slack < 0.0) {
        slew_violations++;
        LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
        if (drvr_port) {
          // Find max load cap that corresponds to max_slew.
          double max_cap1 = findSlewLoadCap(drvr_port, max_slew);
          drvr_max_cap = min(drvr_max_cap, max_cap1);
          debugPrint(logger_, RSZ, "repair_net", 2, "slew={} max_slew={} max_cap={}",
                     delayAsString(slew, this, 3),
                     delayAsString(max_slew, this, 3),
                     units_->capacitanceUnit()->asString(max_cap1, 3));
          repair_slew = true;
        }
      }
    }

    if (repair_slew
        || repair_cap
        || repair_fanout
        || repair_wire) {
      bool using_buffer_cap_limit = false;
      double max_cap = drvr_max_cap;
      if ((repair_cap || repair_slew)
          && buf_cap_limit_exists
          && buf_cap_limit > max_cap) {
        // Don't repair beyond what a min size buffer could drive, since
        // we can insert one at the driver output instead of over optimizing
        // the net for a weak driver.
        max_cap = buf_cap_limit;
        using_buffer_cap_limit = true;
      }

      Point drvr_loc = db_network_->location(drvr->pin());
      debugPrint(logger_, RSZ, "repair_net", 1, "driver {} ({} {}) l={}",
                 sdc_network_->pathName(drvr_pin),
                 units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getX()), 1),
                 units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getY()), 1),
                 units_->distanceUnit()->asString(dbuToMeters(wire_length), 1));
      SteinerPt drvr_pt = tree->steinerPt(drvr_pin);
      int wire_length;
      float pin_cap, fanout;
      PinSeq load_pins;
      repairNet(tree, drvr_pt, SteinerTree::null_pt, net,
                max_cap, max_fanout, max_length, 0,
                wire_length, pin_cap, fanout, load_pins);
      repair_count++;

      double drvr_load = pin_cap + dbuToMeters(wire_length) * wire_cap_;
      if (using_buffer_cap_limit
          && drvr_load > drvr_max_cap) {
        // The remaining cap is more than the driver can handle.
        // We know the min buffer/repeater is capable of driving it,
        // so insert one.
        makeRepeater("drvr", tree, drvr_pt, net, buffer_lowest_drive_, 0,
                     wire_length, pin_cap, fanout, load_pins);
      }
    }
    if (resize_drvr
        && resizeToTargetSlew(drvr_pin))
      resize_count_++;
    delete tree;
  }
}

void
Resizer::checkSlew(const Pin *drvr_pin,
                   // Return values.
                   Slew &slew,
                   float &limit,
                   float &slack)
{
  slack = INF;
  PinConnectedPinIterator *pin_iter = network_->connectedPinIterator(drvr_pin);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    const Corner *corner;
    const RiseFall *tr;
    Slew slew1;
    float limit1, slack1;
    sta_->checkSlew(pin, nullptr, max_, false,
                    corner, tr, slew1, limit1, slack1);
    if (slack1 < slack) {
      slew = slew1;
      limit = limit1;
      slack = slack1;
    }
  }
  delete pin_iter;
}

double
Resizer::findSlewLoadCap(LibertyPort *drvr_port,
                         double slew)
{
  double cap = INF;
  for (Corner *corner : *sta_->corners()) {
    LibertyPort *corner_port = drvr_port->cornerPort(corner->libertyIndex(max_));
    const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
    double corner_cap = findSlewLoadCap(corner_port, slew, dcalc_ap);
    cap = min(cap, corner_cap);
  }
  return cap;
}

// Find the output port load capacitance that results in slew.
double
Resizer::findSlewLoadCap(LibertyPort *drvr_port,
                         double slew,
                         const DcalcAnalysisPt *dcalc_ap)
{
  // cap1 lower bound
  // cap2 upper bound
  double cap1 = 0.0;
  double cap2 = slew / drvr_port->driveResistance() * 2;
  double tol = .01; // 1%
  double diff1 = gateSlewDiff(drvr_port, cap1, slew, dcalc_ap);
  double diff2 = gateSlewDiff(drvr_port, cap2, slew, dcalc_ap);
  // binary search for diff = 0.
  while (abs(cap1 - cap2) > max(cap1, cap2) * tol) {
    if (diff2 < 0.0) {
      cap1 = cap2;
      diff1 = diff2;
      cap2 *= 2;
      diff2 = gateSlewDiff(drvr_port, cap2, slew, dcalc_ap);
    }
    else {
      double cap3 = (cap1 + cap2) / 2.0;
      double diff3 = gateSlewDiff(drvr_port, cap3, slew, dcalc_ap);
      if (diff3 < 0.0) {
        cap1 = cap3;
        diff1 = diff3;
      }
      else {
        cap2 = cap3;
        diff2 = diff3;
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
  const Pvt *pvt = dcalc_ap->operatingConditions();
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
                   float max_cap,
                   float max_fanout,
                   int max_length, // dbu
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
  debugPrint(logger_, RSZ, "repair_net", 2, "%*spt ({} {})",
             level, "",
             units_->distanceUnit()->asString(dbuToMeters(pt_x), 1),
             units_->distanceUnit()->asString(dbuToMeters(pt_y), 1));
  SteinerPt left = tree->left(pt);
  int wire_length_left = 0;
  float pin_cap_left = 0.0;
  float fanout_left = 0.0;
  PinSeq loads_left;
  if (left != SteinerTree::null_pt)
    repairNet(tree, left, pt, net, max_cap, max_fanout, max_length, level + 1,
              wire_length_left, pin_cap_left, fanout_left, loads_left);
  SteinerPt right = tree->right(pt);
  int wire_length_right = 0;
  float pin_cap_right = 0.0;
  float fanout_right = 0.0;
  PinSeq loads_right;
  if (right != SteinerTree::null_pt)
    repairNet(tree, right, pt, net, max_cap, max_fanout, max_length, level + 1,
              wire_length_right, pin_cap_right, fanout_right, loads_right);
  debugPrint(logger_, RSZ, "repair_net", 3, "%*sleft l={} cap={}, right l={} cap={}",
             level, "",
             units_->distanceUnit()->asString(dbuToMeters(wire_length_left), 1),
             units_->capacitanceUnit()->asString(pin_cap_left, 3),
             units_->distanceUnit()->asString(dbuToMeters(wire_length_right), 1),
             units_->capacitanceUnit()->asString(pin_cap_right, 3));
  // Add a buffer to left or right branch to stay under the max cap/length/fanout.
  bool repeater_left = false;
  bool repeater_right = false;
  double cap_left = pin_cap_left + dbuToMeters(wire_length_left) * wire_cap_;
  double cap_right = pin_cap_right + dbuToMeters(wire_length_right) * wire_cap_;
  debugPrint(logger_, RSZ, "repair_net", 3, "%*scap_left={}, right_cap={}",
             level, "",
             units_->capacitanceUnit()->asString(cap_left, 3),
             units_->capacitanceUnit()->asString(cap_right, 3));
  bool cap_violation = (cap_left + cap_right) > max_cap;
  if (cap_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "%*scap violation", level, "");
    if (cap_left > cap_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  bool length_violation = max_length > 0
    && (wire_length_left + wire_length_right) > max_length;
  if (length_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "%*slength violation", level, "");
    if (wire_length_left > wire_length_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  bool fanout_violation = max_fanout > 0
    && (fanout_left + fanout_right) > max_fanout;
  if (fanout_violation) {
    debugPrint(logger_, RSZ, "repair_net", 3, "%*sfanout violation", level, "");
    if (fanout_left > fanout_right)
      repeater_left = true;
    else
      repeater_right = true;
  }

  if (repeater_left)
    makeRepeater("left", tree, pt, net, buffer_lowest_drive_, level,
                 wire_length_left, pin_cap_left, fanout_left, loads_left);
  if (repeater_right)
    makeRepeater("right", tree, pt, net, buffer_lowest_drive_, level,
                 wire_length_right, pin_cap_right, fanout_right, loads_right);

  wire_length = wire_length_left + wire_length_right;
  pin_cap = pin_cap_left + pin_cap_right;
  fanout = fanout_left + fanout_right;

  // Union left/right load pins.
  for (Pin *load_pin : loads_left)
    load_pins.push_back(load_pin);
  for (Pin *load_pin : loads_right)
    load_pins.push_back(load_pin);

  Net *buffer_out = nullptr;
  Pin *load_pin = tree->pin(pt);
  // Steiner pt pin is the net driver if prev_pt is null.
  if (prev_pt != SteinerTree::null_pt) {
    Pin *load_pin = tree->pin(pt);
    if (load_pin) {
      Point load_loc = db_network_->location(load_pin);
      int load_dist = Point::manhattanDistance(load_loc, pt_loc);
      debugPrint(logger_, RSZ, "repair_net", 2, "%*sload {} ({} {}) dist={}",
                 level, "",
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

    Point prev_loc = tree->location(prev_pt);
    int length = Point::manhattanDistance(prev_loc, pt_loc);
    wire_length += length;
    // Back up from pt to prev_pt adding repeaters every max_length.
    int prev_x = prev_loc.getX();
    int prev_y = prev_loc.getY();
    debugPrint(logger_, RSZ, "repair_net", 3, "%*swl={} l={}",
               level, "",
               units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
               units_->distanceUnit()->asString(dbuToMeters(length), 1));
    while ((max_length > 0 && wire_length > max_length)
           || (wire_cap_ > 0.0
               && pin_cap < max_cap
               && (pin_cap + dbuToMeters(wire_length) * wire_cap_) > max_cap)) {
      // Make the wire a bit shorter than necessary to allow for
      // offset from instance origin to pin and detailed placement movement.
      double length_margin = .05;
      // Distance from pt to repeater backward toward prev_pt.
      double buf_dist;
      if (max_length > 0 && wire_length > max_length) {
        buf_dist = length - (wire_length - max_length * (1.0 - length_margin));
      }
      else if (wire_cap_ > 0.0
               && (pin_cap + dbuToMeters(wire_length) * wire_cap_) > max_cap) {
        int cap_length = metersToDbu((max_cap - pin_cap) / wire_cap_);
        buf_dist = length - (wire_length - cap_length * (1.0 - length_margin));
      }
      else
        logger_->critical(RSZ, 23, "repairNet failure.");
      double dx = prev_x - pt_x;
      double dy = prev_y - pt_y;
      double d = buf_dist / length;
      int buf_x = pt_x + d * dx;
      int buf_y = pt_y + d * dy;
      makeRepeater("wire", buf_x, buf_y, net, buffer_lowest_drive_, level,
                   wire_length, pin_cap, fanout, load_pins);
      // Update for the next round.
      length -= buf_dist;
      wire_length = length;
      pt_x = buf_x;
      pt_y = buf_y;
      debugPrint(logger_, RSZ, "repair_net", 3, "%*swl={} l={}",
                 level, "",
                 units_->distanceUnit()->asString(dbuToMeters(wire_length), 1),
                 units_->distanceUnit()->asString(dbuToMeters(length), 1));
    }
  }
}

void
Resizer::makeRepeater(const char *where,
                      SteinerTree *tree,
                      SteinerPt pt,
                      Net *in_net,
                      LibertyCell *buffer_cell,
                      int level,
                      int &wire_length,
                      float &pin_cap,
                      float &fanout,
                      PinSeq &load_pins)
{
  Point pt_loc = tree->location(pt);
  makeRepeater(where, pt_loc.getX(), pt_loc.getY(), in_net, buffer_cell, level,
               wire_length, pin_cap, fanout, load_pins);
}

void
Resizer::makeRepeater(const char *where,
                      int x,
                      int y,
                      Net *in_net,
                      LibertyCell *buffer_cell,
                      int level,
                      int &wire_length,
                      float &pin_cap,
                      float &fanout,
                      PinSeq &load_pins)
{
  Point buf_loc(x, y);
  if (!core_exists_
      || core_.overlaps(buf_loc)) {
    LibertyPort *buffer_input_port, *buffer_output_port;
    buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

    string buffer_name = makeUniqueInstName("repeater");
    debugPrint(logger_, RSZ, "repair_net", 2, "%*s{} {} ({} {})",
               level, "",
               where,
               buffer_name.c_str(),
               units_->distanceUnit()->asString(dbuToMeters(x), 1),
               units_->distanceUnit()->asString(dbuToMeters(y), 1));

    Instance *parent = db_network_->topInstance();
    Net *buffer_out = makeUniqueNet();
    dbNet *buffer_out_db = db_network_->staToDb(buffer_out);
    dbNet *in_net_db = db_network_->staToDb(in_net);
    buffer_out_db->setSigType(in_net_db->getSigType());
    Instance *buffer = db_network_->makeInstance(buffer_cell,
                                                 buffer_name.c_str(),
                                                 parent);
    journalMakeBuffer(buffer);
    setLocation(buffer, buf_loc);
    designAreaIncr(area(db_network_->cell(buffer_cell)));
    inserted_buffer_count_++;

    sta_->connectPin(buffer, buffer_input_port, in_net);
    sta_->connectPin(buffer, buffer_output_port, buffer_out);

    for (Pin *load_pin : load_pins) {
      Port *load_port = network_->port(load_pin);
      Instance *load = network_->instance(load_pin);
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, buffer_out);
    }

    parasiticsInvalid(in_net);
    parasiticsInvalid(buffer_out);

    // Resize repeater as we back up by levels.
    Pin *drvr_pin = network_->findPin(buffer, buffer_output_port);
    resizeToTargetSlew(drvr_pin);
    buffer_cell = network_->libertyCell(buffer);
    buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

    Pin *buf_in_pin = network_->findPin(buffer, buffer_input_port);
    load_pins.clear();
    load_pins.push_back(buf_in_pin);
    wire_length = 0;
    pin_cap = buffer_input_port->capacitance();
    fanout = portFanoutLoad(buffer_input_port);
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::resizeToTargetSlew()
{
  resize_count_ = 0;
  resized_multi_output_insts_.clear();
  // Resize in reverse level order.
  for (int i = level_drvr_vertices_.size() - 1; i >= 0; i--) {
    Vertex *drvr = level_drvr_vertices_[i];
    Pin *drvr_pin = drvr->pin();
    Net *net = network_->net(drvr_pin);
    Instance *inst = network_->instance(drvr_pin);
    if (net
        && !drvr->isConstant()
        && hasFanout(drvr)
        // Hands off the clock nets.
        && !sta_->isClock(drvr_pin)
        // Hands off special nets.
        && !isSpecial(net)) {
      if (resizeToTargetSlew(drvr_pin))
        resize_count_++;
      if (overMaxArea()) {
        logger_->error(RSZ, 24, "Max utilization reached.");
        break;
      }
    }
  }
  ensureWireParasitics();
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
Resizer::resizeToTargetSlew(const Pin *drvr_pin)
{
  NetworkEdit *network = networkEdit();
  Instance *inst = network_->instance(drvr_pin);
  LibertyCell *cell = network_->libertyCell(inst);
  if (cell) {
    LibertyCellSeq *equiv_cells = sta_->equivCells(cell);
    if (equiv_cells) {
      bool revisiting_inst = false;
      if (hasMultipleOutputs(inst)) {
        if (resized_multi_output_insts_.hasKey(inst))
          revisiting_inst = true;
        debugPrint(logger_, RSZ, "resizer", 2, "multiple outputs{}",
                   revisiting_inst ? " - revisit" : "");
        resized_multi_output_insts_.insert(inst);
      }
      bool is_buf_inv = cell->isBuffer() || cell->isInverter();
      ensureWireParasitic(drvr_pin);
      // Includes net parasitic capacitance.
      float load_cap = graph_delay_calc_->loadCap(drvr_pin, tgt_slew_dcalc_ap_);
      if (load_cap > 0.0) {
        LibertyCell *best_cell = cell;
        float target_load = (*target_load_map_)[cell];
        float best_load = target_load;
        float best_dist = targetLoadDist(load_cap, target_load);
        float best_delay = is_buf_inv
          ? bufferDelay(cell, load_cap, tgt_slew_dcalc_ap_)
          : 0.0;
        debugPrint(logger_, RSZ, "resizer", 2, "{} load cap {} dist={:.2e} delay={}",
                   sdc_network_->pathName(drvr_pin),
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
            debugPrint(logger_, RSZ, "resizer", 2, " {} dist={:.2e} delay={}",
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
        if (best_cell != cell) {
          debugPrint(logger_, RSZ, "resizer", 2, "{} {} -> {}",
                     sdc_network_->pathName(drvr_pin),
                     cell->name(),
                     best_cell->name());
          return replaceCell(inst, best_cell, true)
            && !revisiting_inst;
        }
      }
    }
  }
  return false;
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

    // Delete estimated parasitics on all instance pins.
    // Input nets change pin cap, outputs change location (slightly).
    if (have_estimated_parasitics_) {
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
  repairDesign(max_wire_length_,
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
        && !sta_->isClock(drvr_pin)
        // Hands off special nets.
        && !isSpecial(net)) {
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
        findTargetLoad(corner_cell);
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
  return load_cap;
}

void
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
  (*target_load_map_)[cell] = target_load;

  debugPrint(logger_, RSZ, "target_load", 2, "{} target_load = {:.2e}",
             cell->name(),
             target_load);
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
  Instance *parent = db_network_->topInstance();
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
            Instance *tie = sta_->makeInstance(tie_name.c_str(),
                                               tie_cell, top_inst);
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
  if (core_exists_)
    return closestPtInRect(core_, tie_x, tie_y);
  else
    return Point(tie_x, tie_y);
}

////////////////////////////////////////////////////////////////

void
Resizer::repairSetup(float slack_margin)
{
  inserted_buffer_count_ = 0;
  resize_count_ = 0;
  Slack worst_slack;
  Vertex *worst_vertex;
  sta_->worstSlack(max_, worst_slack, worst_vertex);
  debugPrint(logger_, RSZ, "retime", 1, "worst_slack = {}",
             delayAsString(worst_slack, sta_, 3));
  Slack prev_worst_slack = -INF;
  int pass = 1;
  int decreasing_slack_passes = 0;
  while (fuzzyLess(worst_slack, slack_margin)) {
    PathRef worst_path = sta_->vertexWorstSlackPath(worst_vertex, max_);
    repairSetup(worst_path, worst_slack);
    ensureWireParasitics();
    sta_->findRequireds();
    sta_->worstSlack(max_, worst_slack, worst_vertex);
    debugPrint(logger_, RSZ, "retime", 1, "pass {} worst_slack = {}",
               pass,
               delayAsString(worst_slack, sta_, 3));
    if (fuzzyLessEqual(worst_slack, prev_worst_slack)) {
      // Allow slack to increase a few passes to get out of local minima.
      // Do not update prev_worst_slack so it saves the high water mark.
      decreasing_slack_passes++;
      if (decreasing_slack_passes > repair_setup_decreasing_slack_passes_allowed_) {
        // Undo changes that reduced slack.
        journalRestore();
        sta_->worstSlack(max_, worst_slack, worst_vertex);
        debugPrint(logger_, RSZ, "retime", 1,
                   "decreasing slack for %d passes restoring worst_slack {}",
                   repair_setup_decreasing_slack_passes_allowed_,
                   delayAsString(worst_slack, sta_, 3));
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
  // Leave the parasitices up to date.
  ensureWireParasitics();

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
  repairSetup(path, slack);
  // Leave the parasitices up to date.
  ensureWireParasitics();

  if (inserted_buffer_count_ > 0)
    logger_->info(RSZ, 30, "Inserted {} buffers.", inserted_buffer_count_);
  if (resize_count_ > 0)
    logger_->info(RSZ, 31, "Resized {} instances.", resize_count_);
}

void
Resizer::repairSetup(PathRef &path,
                     Slack path_slack)
{
  PathExpanded expanded(&path, sta_);
  if (expanded.size() > 1) {
    int path_length = expanded.size();
    vector<pair<int, Delay>> load_delays;
    int end_index = path_length - 1;
    int start_index = expanded.startIndex();
    PathRef *prev_path = expanded.path(start_index - 1);
    const DcalcAnalysisPt *dcalc_ap = path.dcalcAnalysisPt(sta_);
    int lib_ap = dcalc_ap->libertyIndex();
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
        debugPrint(logger_, RSZ, "retime", 3, "{} load_delay = {}",
                   path_vertex->name(network_),
                   delayAsString(load_delay, sta_, 3));
      }
      prev_path = path;
    }

    sort(load_delays.begin(), load_delays.end(),
         [](pair<int, Delay> pair1,
            pair<int, Delay> pair2) {
           return pair1.second > pair2.second;
         });
    for (auto index_delay : load_delays) {
      int drvr_index = index_delay.first;
      Delay load_delay = index_delay.second;
      PathRef *drvr_path = expanded.path(drvr_index);
      Vertex *drvr_vertex = drvr_path->vertex(sta_);
      Pin *drvr_pin = drvr_vertex->pin();
      PathRef *load_path = expanded.path(drvr_index + 1);
      Vertex *load_vertex = load_path->vertex(sta_);
      Pin *load_pin = load_vertex->pin();
      int fanout = this->fanout(drvr_vertex);
      debugPrint(logger_, RSZ, "retime", 2, "{} fanout = {}",
                 network_->pathName(drvr_pin),
                 fanout);
      if (fanout > 1
          // Rebuffer blows up on large fanout nets.
          && fanout < rebuffer_max_fanout_) {
        int count_before = inserted_buffer_count_;
        rebuffer(drvr_pin);
        int insert_count = inserted_buffer_count_ - count_before;
        debugPrint(logger_, RSZ, "retime", 2, "rebuffer {} inserted {}",
                   network_->pathName(drvr_pin),
                   insert_count);
        if (insert_count > 0)
          break;
      }
      // Don't split loads on low fanout nets.
      if (fanout > split_load_min_fanout_) {
        // Divide and conquer.
        debugPrint(logger_, RSZ, "retime", 2, "split loads {} -> {}",
                   network_->pathName(drvr_pin),
                   network_->pathName(load_pin));
        splitLoads(drvr_path, path_slack);
        break;
      }
      LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
      float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap);
      int in_index = drvr_index - 1;
      PathRef *in_path = expanded.path(in_index);
      Pin *in_pin = in_path->pin(sta_);
      LibertyPort *in_port = network_->libertyPort(in_pin);

      float prev_drive;
      if (drvr_index >= 2) {
        int prev_drvr_index = drvr_index - 2;
        PathRef *prev_drvr_path = expanded.path(prev_drvr_index);
        Pin *prev_drvr_pin = prev_drvr_path->pin(sta_);
        prev_drive = 0.0;
        LibertyPort *prev_drvr_port = network_->libertyPort(prev_drvr_pin);
        if (prev_drvr_port) {
          LibertyPort *corner_prev_drvr_port = prev_drvr_port->cornerPort(lib_ap);
          prev_drive = prev_drvr_port->driveResistance();
        }
      }
      else
        prev_drive = 0.0;
      debugPrint(logger_, RSZ, "retime", 2, "resize {}",
                 network_->pathName(drvr_pin));
      LibertyCell *upsize = upsizeCell(in_port, drvr_port, load_cap,
                                       prev_drive, dcalc_ap);
      if (upsize) {
        Instance *drvr = network_->instance(drvr_pin);
        debugPrint(logger_, RSZ, "retime", 2, "resize {} -> {}",
                   network_->pathName(drvr_pin),
                   upsize->name());
        if (replaceCell(drvr, upsize, true))
          resize_count_++;
        break;
      }
    }
  }
}

void
Resizer::splitLoads(PathRef *drvr_path,
                    Slack drvr_slack)
{
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
    debugPrint(logger_, RSZ, "retime", 3, " fanin {} slack_margin = {}",
               network_->pathName(fanout_vertex->pin()),
               delayAsString(slack_margin, sta_, 3));
    fanout_slacks.push_back(pair<Vertex*, Slack>(fanout_vertex, slack_margin));
  }

  sort(fanout_slacks.begin(), fanout_slacks.end(),
       [](pair<Vertex*, Slack> pair1,
          pair<Vertex*, Slack> pair2) {
         return pair1.second > pair2.second;
       });

  Pin *drvr_pin = drvr_vertex->pin();
  Net *net = network_->net(drvr_pin);
  Instance *drvr = network_->instance(drvr_pin);
  LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
  LibertyCell *drvr_cell = network_->libertyCell(drvr);

  string buffer_name = makeUniqueInstName("split");
  Instance *parent = db_network_->topInstance();
  LibertyCell *buffer_cell = buffer_lowest_drive_;
  Instance *buffer = db_network_->makeInstance(buffer_cell,
                                               buffer_name.c_str(),
                                               parent);
  journalMakeBuffer(buffer);
  inserted_buffer_count_++;
  designAreaIncr(area(db_network_->cell(buffer_cell)));

  Net *in_net = makeUniqueNet();
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
    LibertyPort *load_port = network_->libertyPort(load_pin);
    Instance *load = network_->instance(load_pin);

    sta_->disconnectPin(load_pin);
    sta_->connectPin(load, load_port, out_net);
  }
  Pin *buffer_out_pin = network_->findPin(buffer, output);
  resizeToTargetSlew(buffer_out_pin);
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

////////////////////////////////////////////////////////////////

void
Resizer::repairHold(float slack_margin,
                    bool allow_setup_violations,
                    // Max buffer count as percent of design instance count.
                    float max_buffer_percent)
{
  init();
  LibertyCell *buffer_cell = findHoldBuffer();
  Unit *time_unit = units_->timeUnit();
  logger_->info(RSZ, 59, "Using {} with {}{}{} delay for hold repairs.",
                buffer_cell->name(),
                time_unit->asString(bufferHoldDelay(buffer_cell)),
                time_unit->scaleAbreviation(),
                time_unit->suffix());
  sta_->findRequireds();
  VertexSet *ends = sta_->search()->endpoints();
  int max_buffer_count = max_buffer_percent * network_->instanceCount();
  repairHold(ends, buffer_cell, slack_margin,
             allow_setup_violations, max_buffer_count);

  // Leave the parasitices up to date.
  ensureWireParasitics();
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
  repairHold(&ends, buffer_cell, slack_margin, allow_setup_violations,
             max_buffer_count);
  // Leave the parasitices up to date.
  ensureWireParasitics();
}

// Find the buffer with the most delay in the fastest corner.
LibertyCell *
Resizer::findHoldBuffer()
{
  LibertyCell *max_buffer = nullptr;
  float max_delay = 0.0;
  LibertyCell *max_delay_cell = nullptr;
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
  ArcDelay delays[RiseFall::index_count];
  bufferHoldDelays(buffer, delays);
  return min(delays[RiseFall::riseIndex()],
             delays[RiseFall::fallIndex()]);
}

// Min self delay across corners; buffer -> buffer
void
Resizer::bufferHoldDelays(LibertyCell *buffer,
                          // Return values.
                          ArcDelay delays[RiseFall::index_count])
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
                 "pass {} worst slack {} failures %lu inserted {}",
                 pass,
                 delayAsString(worst_slack, sta_, 3),
                 hold_failures .size(),
                 repair_count);
      sta_->findRequireds();
      findHoldViolations(ends, slack_margin, worst_slack, hold_failures);
      pass++;
    }
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
  Search *search = sta_->search();
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
    ensureWireParasitics();
    Slack hold_slack = sta_->vertexSlack(vertex, MinMax::min());
    if (hold_slack < slack_margin
        // Hands off special nets.
        && !isSpecial(net)) {
      // Only add delay to loads with hold violations.
      PinSeq load_pins;
      Delay insert_delay[RiseFall::index_count] = {INF, INF};
      bool loads_have_out_port = false;
      VertexOutEdgeIterator edge_iter(vertex, graph_);
      while (edge_iter.hasNext()) {
        Edge *edge = edge_iter.next();
        Vertex *fanout = edge->to(graph_);
        Slacks slacks;
        sta_->vertexSlacks(fanout, slacks);
        bool buffer_fanout = true;
        for (int rf_index : RiseFall::rangeIndex()) {
          Slack hold_slack = slacks[rf_index][MinMax::minIndex()] - slack_margin;
          if (hold_slack < 0.0) {
            Delay delay = allow_setup_violations
              ? -hold_slack
              // Don't add delay that leads to a setup violation.
              : min(-hold_slack, slacks[rf_index][MinMax::maxIndex()]);
            if (delay > 0.0)
              insert_delay[rf_index] = min(insert_delay[rf_index], delay);
            else
              // no room to buffer
              buffer_fanout = false;
          }
        }
        if (buffer_fanout) {
          Pin *fanout_pin = fanout->pin();
          load_pins.push_back(fanout_pin);
          if (network_->direction(fanout_pin)->isAnyOutput()
              && network_->isTopLevelPort(fanout_pin))
            loads_have_out_port = true;
        }
      }
      if (!load_pins.empty()) {
        int buffer_count = 0;
        ArcDelay buffer_delays[RiseFall::index_count];
        bufferHoldDelays(buffer_cell, buffer_delays);
        for (int rf_index : RiseFall::rangeIndex()) {
          int rf_count = std::ceil(insert_delay[rf_index]/buffer_delays[rf_index]);
          buffer_count = max(buffer_count, rf_count);
        }
        debugPrint(logger_, RSZ, "repair_hold", 2,
                   " {} hold={} inserted %d for {}/{} loads",
                   vertex->name(sdc_network_),
                   delayAsString(hold_slack, this, 3),
                   buffer_count,
                   load_pins.size(),
                   fanout(vertex));
        makeHoldDelay(vertex, buffer_count, load_pins,
                      loads_have_out_port, buffer_cell);
        repair_count += buffer_count;
        if (inserted_buffer_count_ > max_buffer_count
            || overMaxArea())
          return repair_count;
      }
    }
  }
  return repair_count;
}

VertexSet
Resizer::findHoldFanins(VertexSet &ends)
{
  Search *search = sta_->search();
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
  Point drvr_pin_loc = db_network_->location(drvr_pin);
  // Stay inside the core.
  Point drvr_loc = core_exists_ ? closestPtInRect(core_, drvr_pin_loc) : drvr_pin_loc;
  Point load_center = findCenter(load_pins);
  int dx = (drvr_loc.x() - load_center.x()) / (buffer_count + 1);
  int dy = (drvr_loc.y() - load_center.y()) / (buffer_count + 1);

  Net *buf_in_net = in_net;
  // drvr_pin->in_net->hold_buffer1->net2->hold_buffer2->out_net...->load_pins
  for (int i = 0; i < buffer_count; i++) {
    Net *buf_out_net = (i == buffer_count - 1) ? out_net : makeUniqueNet();
    // drvr_pin->drvr_net->hold_buffer->net2->load_pins
    string buffer_name = makeUniqueInstName("hold");
    Instance *buffer = db_network_->makeInstance(buffer_cell,
                                                 buffer_name.c_str(),
                                                 parent);
    journalMakeBuffer(buffer);
    inserted_buffer_count_++;
    designAreaIncr(area(db_network_->cell(buffer_cell)));

    LibertyPort *input, *output;
    buffer_cell->bufferPorts(input, output);
    sta_->connectPin(buffer, input, buf_in_net);
    sta_->connectPin(buffer, output, buf_out_net);
    Point buffer_loc(drvr_loc.x() + dx * i,
                     drvr_loc.y() + dy * i);
    setLocation(buffer, buffer_loc);
    buf_in_net = buf_out_net;
    parasiticsInvalid(buf_out_net);
  }

  for (Pin *load_pin : load_pins) {
    Instance *load = db_network_->instance(load_pin);
    Port *load_port = db_network_->port(load_pin);
    sta_->disconnectPin(load_pin);
    sta_->connectPin(load, load_port, out_net);
  }
}

Point
Resizer::findCenter(PinSeq &pins)
{
  Point sum(0, 0);
  for (Pin *pin : pins) {
    Point loc = db_network_->location(pin);
    sum.x() += loc.x();
    sum.y() += loc.y();
  }
  return Point(sum.x() / pins.size(), sum.y() / pins.size());
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
  int i = 0;
  for (Vertex *drvr : drvrs) {
    Pin *drvr_pin = drvr->pin();
    if (!network_->isTopLevelPort(drvr_pin)) {
      double wire_length = dbuToMeters(maxLoadManhattenDistance(drvr));
      double steiner_length = dbuToMeters(findMaxSteinerDist(drvr));
      double delay = wire_length * wire_res_ * wire_length * wire_cap_ * 0.5;
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
      Net *net = network_->net(pin);
      // Hands off the clock nets.
      if (!sta_->isClock(pin)
          && !vertex->isConstant()
          && !vertex->isDisabledConstraint())
        drvr_dists.push_back(DrvrDist(vertex, maxLoadManhattenDistance(vertex)));
    }
  }
  sort(drvr_dists, [this](const DrvrDist &drvr_dist1,
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
      Net *net = network_->net(pin);
      // Hands off the clock nets.
      if (!sta_->isClock(pin)
          && !vertex->isConstant())
        drvr_dists.push_back(DrvrDist(vertex, findMaxSteinerDist(vertex)));
    }
  }
  sort(drvr_dists, [this](const DrvrDist &drvr_dist1,
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
  Net *net = network_->net(drvr_pin);
  SteinerTree *tree = makeSteinerTree(net, true, db_network_, logger_);
  if (tree) {
    int dist = findMaxSteinerDist(drvr, tree);
    delete tree;
    return dist;
  }
  return 0;
}

int
Resizer::findMaxSteinerDist(Vertex *drvr,
                            SteinerTree *tree)
{
  Pin *drvr_pin = drvr->pin();
  SteinerPt drvr_pt = tree->steinerPt(drvr_pin);
  return findMaxSteinerDist(tree, drvr_pt, 0);
}

// DFS of steiner tree.
int
Resizer::findMaxSteinerDist(SteinerTree *tree,
                            SteinerPt pt,
                            int dist_from_drvr)
{
  Pin *pin = tree->pin(pt);
  if (pin && db_network_->isLoad(pin))
    return dist_from_drvr;
  else {
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
  for (LibertyCell *buffer_cell : buffer_cells_) {
    double buffer_length = findMaxWireLength(buffer_cell);
    max_length = min(max_length, buffer_length);
  }
  return max_length;
}

// Find the max wire length before it is faster to split the wire
// in half with a buffer (in meters).
double
Resizer::findMaxWireLength(LibertyCell *buffer_cell)
{
  ensureBlock();
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  return findMaxWireLength(drvr_port);
}

double
Resizer::findMaxWireLength(LibertyPort *drvr_port)
{
  LibertyCell *cell = drvr_port->libertyCell();
  double drvr_r = drvr_port->driveResistance();
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  double wire_length2 = drvr_r / wire_res_;
  double tol = .01; // 1%
  double diff1 = splitWireDelayDiff(wire_length1, cell);
  double diff2 = splitWireDelayDiff(wire_length2, cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff2 < 0.0) {
      wire_length1 = wire_length2;
      diff1 = diff2;
      wire_length2 *= 2;
      diff2 = splitWireDelayDiff(wire_length2, cell);
    }
    else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff3 = splitWireDelayDiff(wire_length3, cell);
      if (diff3 < 0.0) {
        wire_length1 = wire_length3;
        diff1 = diff3;
      }
      else {
        wire_length2 = wire_length3;
        diff2 = diff3;
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

  Instance *top_inst = network_->topInstance();
  // Tmp net for parasitics to live on.
  Net *net = sta->makeNet("wire", top_inst);
  LibertyCell *drvr_cell = drvr_port->libertyCell();
  LibertyCell *load_cell = load_port->libertyCell();
  Instance *drvr = sta->makeInstance("drvr", drvr_cell, top_inst);
  Instance *load = sta->makeInstance("load", load_cell, top_inst);
  sta->connectPin(drvr, drvr_port, net);
  sta->connectPin(load, load_port, net);
  Pin *drvr_pin = network_->findPin(drvr, drvr_port);
  Pin *load_pin = network_->findPin(load, load_port);

  // Max rise/fall delays.
  delay = -INF;
  slew = -INF;

  for (Corner *corner : *sta_->corners()) {
    const ParasiticAnalysisPt *parasitics_ap =
      corner->findParasiticAnalysisPt(max_);
    const DcalcAnalysisPt *dcalc_ap = corner->findDcalcAnalysisPt(max_);
    const Pvt *pvt = dcalc_ap->operatingConditions();

    makeWireParasitic(net, drvr_pin, load_pin, wire_length, parasitics_ap);
    // Let delay calc reduce parasitic network as it sees fit.
    Parasitic *drvr_parasitic = arc_delay_calc_->findParasitic(drvr_pin, RiseFall::rise(),
                                                               dcalc_ap);

    LibertyCellTimingArcSetIterator set_iter(drvr_cell);
    while (set_iter.hasNext()) {
      TimingArcSet *arc_set = set_iter.next();
      if (arc_set->to() == drvr_port) {
        TimingArcSetArcIterator arc_iter(arc_set);
        while (arc_iter.hasNext()) {
          TimingArc *arc = arc_iter.next();
          RiseFall *in_rf = arc->fromTrans()->asRiseFall();
          int out_rf_index = arc->toTrans()->asRiseFall()->index();
          double in_slew = tgt_slews_[in_rf->index()];
          ArcDelay gate_delay;
          Slew drvr_slew;
          arc_delay_calc_->gateDelay(drvr_cell, arc, in_slew, 0.0,
                                     drvr_parasitic, 0.0, pvt, dcalc_ap,
                                     gate_delay,
                                     drvr_slew);
          ArcDelay wire_delay;
          Slew load_slew;
          arc_delay_calc_->loadDelay(load_pin, wire_delay, load_slew);
          delay = max(delay, gate_delay + wire_delay);
          slew = max(slew, load_slew);
        }
      }
    }
    arc_delay_calc_->finishDrvrPin();
    parasitics_->deleteParasitics(net, dcalc_ap->parasiticAnalysisPt());
  }

  // Cleanup the turds.
  sta->deleteInstance(drvr);
  sta->deleteInstance(load);
  sta->deleteNet(net);
  delete sta;
  dbBlock::destroy(block);
}

Parasitic *
Resizer::makeWireParasitic(Net *net,
                           Pin *drvr_pin,
                           Pin *load_pin,
                           double wire_length, // meters
                           const ParasiticAnalysisPt *parasitics_ap)
{
  Parasitic *parasitic = parasitics_->makeParasiticNetwork(net, false,
                                                           parasitics_ap);
  ParasiticNode *n1 = parasitics_->ensureParasiticNode(parasitic, drvr_pin);
  ParasiticNode *n2 = parasitics_->ensureParasiticNode(parasitic, load_pin);
  double wire_cap = wire_length * wire_cap_;
  double wire_res = wire_length * wire_res_;
  parasitics_->incrCap(n1, wire_cap / 2.0, parasitics_ap);
  parasitics_->makeResistor(nullptr, n1, n2, wire_res, parasitics_ap);
  parasitics_->incrCap(n2, wire_cap / 2.0, parasitics_ap);
  return parasitic;
}

////////////////////////////////////////////////////////////////

double
Resizer::findMaxSlewWireLength(LibertyPort *drvr_port,
                               LibertyPort *load_port,
                               double max_slew)
{
  ensureBlock();
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  double wire_length2 = std::sqrt(max_slew / (wire_res_ * wire_cap_));
  double tol = .01; // 1%
  double diff1 = maxSlewWireDiff(drvr_port, load_port, wire_length1, max_slew);
  double diff2 = maxSlewWireDiff(drvr_port, load_port, wire_length2, max_slew);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff2 < 0.0) {
      wire_length1 = wire_length2;
      diff1 = diff2;
      wire_length2 *= 2;
      diff2 = maxSlewWireDiff(drvr_port, load_port, wire_length2, max_slew);
    }
    else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff3 = maxSlewWireDiff(drvr_port, load_port, wire_length3, max_slew);
      if (diff3 < 0.0) {
        wire_length1 = wire_length3;
        diff1 = diff3;
      }
      else {
        wire_length2 = wire_length3;
        diff2 = diff3;
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
  if (core_exists_) {
    design_area_ = 0.0;
    for (dbInst *inst : block_->getInsts()) {
      dbMaster *master = inst->getMaster();
      design_area_ += area(master);
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

bool
Resizer::isSpecial(Net *net)
{
  dbNet *db_net = db_network_->staToDb(net);
  return db_net->isSpecial();
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
        Instance *clone = sta_->makeInstance(clone_name.c_str(),
                                             inv_cell, top_inst);
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
    sta_->deleteInstance(inv);
  }
}

////////////////////////////////////////////////////////////////

// Journal to roll back changes (OpenDB not up to the task).
void
Resizer::journalBegin()
{
  debugPrint(logger_, RSZ, "resize_journal", 1, "begin");
  resized_inst_map_.clear();
  inserted_buffers_.clear();
}

void
Resizer::journalInstReplaceCellBefore(Instance *inst)
{
  LibertyCell *lib_cell = network_->libertyCell(inst);
  debugPrint(logger_, RSZ, "resize_journal", 1, "replace {} ({})",
             network_->pathName(inst),
             lib_cell->name());
  resized_inst_map_[inst] = lib_cell;
}

void
Resizer::journalMakeBuffer(Instance *buffer)
{
  debugPrint(logger_, RSZ, "resize_journal", 1, "make_buffer {}",
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
      debugPrint(logger_, RSZ, "resize_journal", 1, "restore {} ({})",
                 network_->pathName(inst),
                 lib_cell->name());
      replaceCell(inst, lib_cell, false);
      resize_count_--;
    }
  }
  for (Instance *buffer : inserted_buffers_) {
    debugPrint(logger_, RSZ, "resize_journal", 1, "remove {}",
               network_->pathName(buffer));
    removeBuffer(buffer);
    inserted_buffer_count_--;
  }
}

////////////////////////////////////////////////////////////////

class SteinerRenderer : public gui::Renderer
{
public:
  SteinerRenderer(Resizer *resizer);
  void highlight(SteinerTree *tree);
  virtual void drawObjects(gui::Painter& /* painter */) override;

private:
  Resizer *resizer_;
  SteinerTree *tree_;
};

// Highlight guide in the gui.
void
Resizer::highlightSteiner(const Net *net)
{
  if (gui_) {
    if (steiner_renderer_ == nullptr) {
      steiner_renderer_ = new SteinerRenderer(this);
      gui_->registerRenderer(steiner_renderer_);
    }
    SteinerTree *tree = makeSteinerTree(net, false, db_network_, logger_);
    if (tree)
      steiner_renderer_->highlight(tree);
  }
}

SteinerRenderer::SteinerRenderer(Resizer *resizer) :
  resizer_(resizer),
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
      Pin *pin1, *pin2;
      int steiner_pt1, steiner_pt2;
      int wire_length;
      tree_->branch(i, pt1, pin1, steiner_pt1, pt2, pin2, steiner_pt2, wire_length);
      painter.drawLine(pt1, pt2);
    }
  }
}

} // namespace
