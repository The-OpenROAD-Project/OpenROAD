/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// All rights reserved.
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

#include "resizer/Resizer.hh"

#include "sta/Report.hh"
#include "sta/Debug.hh"
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
#include "sta/StaMain.hh"
#include "openroad/OpenRoad.hh"
#include "openroad/Error.hh"
#include "resizer/SteinerTree.hh"
#include "opendb/dbTransform.h"

// Outstanding issues
//  multi-corner support?
//  option to place buffers between driver and load on long wires
//   to fix max slew/cap violations
// http://vlsicad.eecs.umich.edu/BK/Slots/cache/dropzone.tamu.edu/~zhuoli/GSRC/fast_buffer_insertion.html

namespace sta {

using std::abs;
using std::min;
using std::max;
using std::string;
using std::to_string;

using ord::warn;
using ord::closestPtInRect;

using odb::dbInst;
using odb::dbPlacementStatus;
using odb::Rect;
using odb::dbOrientType;
using odb::dbTransform;
using odb::dbMPin;
using odb::dbBox;

extern "C" {
extern int Resizer_Init(Tcl_Interp *interp);
}

extern const char *resizer_tcl_inits[];

typedef array<Delay, RiseFall::index_count> Delays;
typedef array<Slack, RiseFall::index_count> Slacks;

////////////////////////////////////////////////////////////////

Resizer::Resizer() :
  StaState(),
  wire_res_(0.0),
  wire_cap_(0.0),
  corner_(nullptr),
  max_area_(0.0),
  sta_(nullptr),
  db_network_(nullptr),
  db_(nullptr),
  core_exists_(false),
  min_max_(nullptr),
  dcalc_ap_(nullptr),
  pvt_(nullptr),
  parasitics_ap_(nullptr),
  clk_nets_valid_(false),
  have_estimated_parasitics_(false),
  target_load_map_(nullptr),
  level_drvr_verticies_valid_(false),
  tgt_slews_{0.0, 0.0},
  unique_net_index_(1),
  unique_inst_index_(1),
  resize_count_(0),
  design_area_(0.0)
{
}

void
Resizer::init(Tcl_Interp *interp,
	      dbDatabase *db,
	      dbSta *sta)
{
  db_ = db;
  block_ = nullptr;
  sta_ = sta;
  db_network_ = sta->getDbNetwork();
  copyState(sta);
  // Define swig TCL commands.
  Resizer_Init(interp);
  // Eval encoded sta TCL sources.
  evalTclInit(interp, resizer_tcl_inits);
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
    design_area_ = findDesignArea();
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
  ensureLevelDrvrVerticies();
  ensureClkNets();
  ensureCorner();
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
      LibertyPort *input_port, *output_port;
      lib_cell->bufferPorts(input_port, output_port);
      Instance *buffer = db_network_->dbToSta(inst);
      Pin *input_pin = db_network_->findPin(buffer, input_port);
      Pin *output_pin = db_network_->findPin(buffer, output_port);
      Net *input_net = db_network_->net(input_pin);
      Net *output_net = db_network_->net(output_pin);
      if (!hasTopLevelPort(input_net)
	  && !hasTopLevelPort(output_net)) {
	NetPinIterator *pin_iter = db_network_->pinIterator(output_net);
	while (pin_iter->hasNext()) {
	  Pin *pin = pin_iter->next();
	  if (pin != output_pin) {
	    Instance *pin_inst = db_network_->instance(pin);
	    Port *pin_port = db_network_->port(pin);
	    sta_->disconnectPin(pin);
	    sta_->connectPin(pin_inst, pin_port, input_net);
	  }
	}
	delete pin_iter;
	sta_->deleteNet(output_net);
	sta_->deleteInstance(buffer);
	remove_count++;
      }
    }
  }
  printf("Removed %d buffers.\n", remove_count);
}

void
Resizer::setWireRC(float wire_res,
		   float wire_cap,
		   Corner *corner)
{
  initCorner(corner);
  // Abbreviated copyState
  graph_delay_calc_ = sta_->graphDelayCalc();
  search_ = sta_->search();
  graph_ = sta_->ensureGraph();

  sta_->ensureLevelized();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  wire_res_ = wire_res;
  wire_cap_ = wire_cap;
}

void
Resizer::ensureCorner()
{
  if (corner_ == nullptr)
    initCorner(sta_->cmdCorner());
}

void
Resizer::initCorner(Corner *corner)
{
  corner_ = corner;
  min_max_ = MinMax::max();
  dcalc_ap_ = corner->findDcalcAnalysisPt(min_max_);
  pvt_ = dcalc_ap_->operatingConditions();
  parasitics_ap_ = corner->findParasiticAnalysisPt(min_max_);
}

void
Resizer::ensureLevelDrvrVerticies()
{
  if (!level_drvr_verticies_valid_) {
    level_drvr_verticies_.clear();
    VertexIterator vertex_iter(graph_);
    while (vertex_iter.hasNext()) {
      Vertex *vertex = vertex_iter.next();
      if (vertex->isDriver(network_))
	level_drvr_verticies_.push_back(vertex);
    }
    sort(level_drvr_verticies_, VertexLevelLess(network_));
    level_drvr_verticies_valid_ = true;
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::resizePreamble(LibertyLibrarySeq *resize_libs)
{
  init();
  makeEquivCells(resize_libs);
  findTargetLoads(resize_libs);
}

////////////////////////////////////////////////////////////////

void
Resizer::bufferInputs(LibertyCell *buffer_cell)
{
  init();
  inserted_buffer_count_ = 0;
  InstancePinIterator *port_iter(network_->pinIterator(network_->topInstance()));
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isInput()
	&& net
	&& !isClock(net)
	&& !isSpecial(net))
      bufferInput(pin, buffer_cell);
  }
  delete port_iter;
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d input buffers.\n", inserted_buffer_count_);
    level_drvr_verticies_valid_ = false;
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
  string buffer_out_name = makeUniqueNetName();
  string buffer_name = makeUniqueInstName("input");
  Instance *parent = db_network_->topInstance();
  Net *buffer_out = db_network_->makeNet(buffer_out_name.c_str(), parent);
  Instance *buffer = db_network_->makeInstance(buffer_cell,
					       buffer_name.c_str(),
					       parent);
  if (buffer) {
    Point pin_loc = db_network_->location(top_pin);
    Point buf_loc = closestPtInRect(core_, pin_loc);
    setLocation(buffer, buf_loc);
    design_area_ += area(db_network_->cell(buffer_cell));
    inserted_buffer_count_++;

    NetPinIterator *pin_iter(db_network_->pinIterator(input_net));
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      // Leave input port pin connected to input_net.
      if (pin != top_pin) {
	sta_->disconnectPin(pin);
	Port *pin_port = db_network_->port(pin);
	sta_->connectPin(db_network_->instance(pin), pin_port, buffer_out);
      }
    }
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
Resizer::bufferOutputs(LibertyCell *buffer_cell)
{
  init();
  inserted_buffer_count_ = 0;
  InstancePinIterator *port_iter(network_->pinIterator(network_->topInstance()));
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isOutput()
	&& net
	&& !isSpecial(net))
      bufferOutput(pin, buffer_cell);
  }
  delete port_iter;
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d output buffers.\n", inserted_buffer_count_);
    level_drvr_verticies_valid_ = false;
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
  string buffer_in_net_name = makeUniqueNetName();
  string buffer_name = makeUniqueInstName("output");
  Instance *parent = network->topInstance();
  Net *buffer_in = network->makeNet(buffer_in_net_name.c_str(), parent);
  Instance *buffer = network->makeInstance(buffer_cell,
					   buffer_name.c_str(),
					   parent);
  if (buffer) {
    setLocation(buffer, db_network_->location(top_pin));
    design_area_ += area(db_network_->cell(buffer_cell));
    inserted_buffer_count_++;

    NetPinIterator *pin_iter(network->pinIterator(output_net));
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      if (pin != top_pin) {
	// Leave output port pin connected to output_net.
	sta_->disconnectPin(pin);
	Port *pin_port = network->port(pin);
	sta_->connectPin(network->instance(pin), pin_port, buffer_in);
      }
    }
    sta_->connectPin(buffer, input, buffer_in);
    sta_->connectPin(buffer, output, output_net);
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::resizeToTargetSlew()
{
  resize_count_ = 0;
  // Resize in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *drvr = level_drvr_verticies_[i];
      Pin *drvr_pin = drvr->pin();
      Net *net = network_->net(drvr_pin);
      Instance *inst = network_->instance(drvr_pin);
      if (net
	  && !drvr->isConstant()
	  && hasFanout(drvr)
	  // Hands off the clock nets.
	  && !isClock(net)
	  // Hands off special nets.
	  && !isSpecial(net)) {
      resizeToTargetSlew(drvr_pin);
      if (overMaxArea()) {
	warn("Max utilization reached.");
	break;
      }
    }
  }
  printf("Resized %d instances.\n", resize_count_);
}

bool
Resizer::hasFanout(Vertex *drvr)
{
  VertexOutEdgeIterator edge_iter(drvr, graph_);
  return edge_iter.hasNext();
}

void
Resizer::makeEquivCells(LibertyLibrarySeq *resize_libs)
{
  // Map cells from all libraries to resize_libs.
  LibertyLibrarySeq map_libs;
  LibertyLibraryIterator *lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    LibertyLibrary *lib = lib_iter->next();
    map_libs.push_back(lib);
  }
  delete lib_iter;
  sta_->makeEquivCells(resize_libs, &map_libs);
}

void
Resizer::resizeToTargetSlew(const Pin *drvr_pin)
{
  NetworkEdit *network = networkEdit();
  Instance *inst = network_->instance(drvr_pin);
  LibertyCell *cell = network_->libertyCell(inst);
  if (cell) {
    // Includes net parasitic capacitance.
    float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap_);
    if (load_cap > 0.0) {
      LibertyCell *best_cell = nullptr;
      float best_ratio = 0.0;
      LibertyCellSeq *equiv_cells = sta_->equivCells(cell);
      if (equiv_cells) {
	for (LibertyCell *target_cell : *equiv_cells) {
	  if (!dontUse(target_cell)) {
	    float target_load = (*target_load_map_)[target_cell];
	    float ratio = target_load / load_cap;
	    if (ratio > 1.0)
	      ratio = 1.0 / ratio;
	    if (ratio > best_ratio) {
	      best_ratio = ratio;
	      best_cell = target_cell;
	    }
	  }
	}
	if (best_cell && best_cell != cell) {
	  debugPrint3(debug_, "resizer", 2, "%s %s -> %s\n",
		      sdc_network_->pathName(inst),
		      cell->name(),
		      best_cell->name());
	  const char *best_cell_name = best_cell->name();
	  dbMaster *best_master = db_->findMaster(best_cell_name);
	  // Replace LEF with LEF so ports stay aligned in instance.
	  if (best_master) {
	    dbInst *dinst = db_network_->staToDb(inst);
	    dbMaster *master = dinst->getMaster();
	    design_area_ -= area(master);
	    Cell *best_cell1 = db_network_->dbToSta(best_master);
	    sta_->replaceCell(inst, best_cell1);
	    resize_count_++;
	    design_area_ += area(best_master);
	  }
	}
      }
    }
  }
}

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
Resizer::findTargetLoads(LibertyLibrarySeq *resize_libs)
{
  // Find target slew across all buffers in the libraries.
  findBufferTargetSlews(resize_libs);
  if (target_load_map_ == nullptr)
    target_load_map_ = new CellTargetLoadMap;
  for (LibertyLibrary *lib : *resize_libs)
    findTargetLoads(lib, tgt_slews_);
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
Resizer::findTargetLoads(LibertyLibrary *library,
			 TgtSlews &slews)
{
  LibertyCellIterator cell_iter(library);
  while (cell_iter.hasNext()) {
    LibertyCell *cell = cell_iter.next();
    findTargetLoad(cell, slews);
  }
}

void
Resizer::findTargetLoad(LibertyCell *cell,
			TgtSlews &slews)
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
	RiseFall *in_rf = arc->fromTrans()->asRiseFall();
	RiseFall *out_rf = arc->toTrans()->asRiseFall();
	float arc_target_load = findTargetLoad(cell, arc,
					       slews[in_rf->index()],
					       slews[out_rf->index()]);
	target_load_sum += arc_target_load;
	arc_count++;
      }
    }
  }
  float target_load = (arc_count > 0) ? target_load_sum / arc_count : 0.0;
  (*target_load_map_)[cell] = target_load;
  debugPrint2(debug_, "resizer", 3, "%s target_load = %.2e\n",
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
    float cap_init = 1.0e-12;  // 1pF
    float cap_tol = cap_init * .001; // .1%
    float load_cap = cap_init;
    float cap_step = cap_init;
    while (cap_step > cap_tol) {
      ArcDelay arc_delay;
      Slew arc_slew;
      model->gateDelay(cell, pvt_, in_slew, load_cap, 0.0, false,
		       arc_delay, arc_slew);
      if (arc_slew > out_slew) {
	load_cap -= cap_step;
	cap_step /= 2.0;
      }
      load_cap += cap_step;
    }
    return load_cap;
  }
  return 0.0;
}

////////////////////////////////////////////////////////////////

Slew
Resizer::targetSlew(const RiseFall *rf)
{
  return tgt_slews_[rf->index()];
}

// Find target slew across all buffers in the libraries.
void
Resizer::findBufferTargetSlews(LibertyLibrarySeq *resize_libs)
{
  tgt_slews_[RiseFall::riseIndex()] = 0.0;
  tgt_slews_[RiseFall::fallIndex()] = 0.0;
  int tgt_counts[RiseFall::index_count]{0};
  
  for (LibertyLibrary *lib : *resize_libs) {
    Slew slews[RiseFall::index_count]{0.0};
    int counts[RiseFall::index_count]{0};
    
    findBufferTargetSlews(lib, slews, counts);
    for (int rf : RiseFall::rangeIndex()) {
      tgt_slews_[rf] += slews[rf];
      tgt_counts[rf] += counts[rf];
      slews[rf] /= counts[rf];
    }
    debugPrint3(debug_, "resizer", 2, "target_slews %s = %.2e/%.2e\n",
		lib->name(),
		slews[RiseFall::riseIndex()],
		slews[RiseFall::fallIndex()]);
  }

  for (int rf : RiseFall::rangeIndex())
    tgt_slews_[rf] /= tgt_counts[rf];

  debugPrint2(debug_, "resizer", 1, "target_slews = %.2e/%.2e\n",
	      tgt_slews_[RiseFall::riseIndex()],
	      tgt_slews_[RiseFall::fallIndex()]);
}

void
Resizer::findBufferTargetSlews(LibertyLibrary *library,
			       // Return values.
			       Slew slews[],
			       int counts[])
{
  for (LibertyCell *buffer : *library->buffers()) {
    if (!dontUse(buffer)) {
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
	    float in_cap = input->capacitance(in_rf, min_max_);
	    float load_cap = in_cap * 10.0; // "factor debatable"
	    ArcDelay arc_delay;
	    Slew arc_slew;
	    model->gateDelay(buffer, pvt_, 0.0, load_cap, 0.0, false,
			     arc_delay, arc_slew);
	    model->gateDelay(buffer, pvt_, arc_slew, load_cap, 0.0, false,
			     arc_delay, arc_slew);
	    slews[out_rf->index()] += arc_slew;
	    counts[out_rf->index()]++;
	  }
	}
      }
    }
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::ensureClkNets()
{
  if (!clk_nets_valid_) {
    findClkNets();
    clk_nets_valid_ = true;
  }
}

// Find clock nets.
// This is not as reliable as Search::isClock but is much cheaper.
void
Resizer::findClkNets()
{
  ClkArrivalSearchPred srch_pred(this);
  BfsFwdIterator bfs(BfsIndex::other, &srch_pred, this);
  PinSet clk_pins;
  search_->findClkVertexPins(clk_pins);
  for (Pin *pin : clk_pins) {
    Vertex *vertex, *bidirect_drvr_vertex;
    graph_->pinVertices(pin, vertex, bidirect_drvr_vertex);
    bfs.enqueue(vertex);
    if (bidirect_drvr_vertex)
      bfs.enqueue(bidirect_drvr_vertex);
  }  
  while (bfs.hasNext()) {
    Vertex *vertex = bfs.next();
    const Pin *pin = vertex->pin();
    Net *net = network_->net(pin);
    if (net)
      clk_nets_.insert(net);
    bfs.enqueueAdjacentVertices(vertex);
  }
}

bool
Resizer::isClock(Net *net)
{
  return clk_nets_.hasKey(net);
}

////////////////////////////////////////////////////////////////

void
Resizer::estimateWireParasitics()
{
  if (wire_cap_ > 0.0) {
    ensureClkNets();

    NetIterator *net_iter = network_->netIterator(network_->topInstance());
    while (net_iter->hasNext()) {
      Net *net = net_iter->next();
      // Estimate parastices for clocks also for when they are propagated.
      if (!network_->isPower(net)
	  && !network_->isGround(net))
	estimateWireParasitic(net);
    }
    delete net_iter;
    have_estimated_parasitics_ = true;
  }
}

void
Resizer::estimateWireParasitic(const dbNet *net)
{
  estimateWireParasitic(db_network_->dbToSta(net));
}
 
void
Resizer::estimateWireParasitic(const Net *net)
{
  // Do not add parasitics on ports.
  // When the input drives a pad instance with huge input
  // cap the elmore delay is gigantic.
  if (!hasTopLevelPort(net)) {
    SteinerTree *tree = makeSteinerTree(net, false, db_network_);
    if (tree) {
      debugPrint1(debug_, "resizer_parasitics", 1, "net %s\n",
		  sdc_network_->pathName(net));
      Parasitic *parasitic = parasitics_->makeParasiticNetwork(net, false,
							       parasitics_ap_);
      int branch_count = tree->branchCount();
      for (int i = 0; i < branch_count; i++) {
	Point pt1, pt2;
	Pin *pin1, *pin2;
	int steiner_pt1, steiner_pt2;
	int wire_length_dbu;
	tree->branch(i,
		     pt1, pin1, steiner_pt1,
		     pt2, pin2, steiner_pt2,
		     wire_length_dbu);
	ParasiticNode *n1 = findParasiticNode(tree, parasitic, net, pin1, steiner_pt1);
	ParasiticNode *n2 = findParasiticNode(tree, parasitic, net, pin2, steiner_pt2);
	if (n1 != n2) {
	  if (wire_length_dbu == 0)
	    // Use a small resistor to keep the connectivity intact.
	    parasitics_->makeResistor(nullptr, n1, n2, 1.0e-3, parasitics_ap_);
	  else {
	    float wire_length = dbuToMeters(wire_length_dbu);
	    float wire_cap = wire_length * wire_cap_;
	    float wire_res = wire_length * wire_res_;
	    // Make pi model for the wire.
	    debugPrint5(debug_, "resizer_parasitics", 2,
			" pi %s c2=%s rpi=%s c1=%s %s\n",
			parasitics_->name(n1),
			units_->capacitanceUnit()->asString(wire_cap / 2.0),
			units_->resistanceUnit()->asString(wire_res),
			units_->capacitanceUnit()->asString(wire_cap / 2.0),
			parasitics_->name(n2));
	    parasitics_->incrCap(n1, wire_cap / 2.0, parasitics_ap_);
	    parasitics_->makeResistor(nullptr, n1, n2, wire_res, parasitics_ap_);
	    parasitics_->incrCap(n2, wire_cap / 2.0, parasitics_ap_);
	  }
	}
      }
      ReduceParasiticsTo reduce_to = ReduceParasiticsTo::pi_elmore;
      const OperatingConditions *op_cond = sdc_->operatingConditions(MinMax::max());
      parasitics_->reduceTo(parasitic, net, reduce_to, op_cond,
			    corner_, MinMax::max(), parasitics_ap_);
      parasitics_->deleteParasiticNetwork(net, parasitics_ap_);
      delete tree;
    }
  }
}

ParasiticNode *
Resizer::findParasiticNode(SteinerTree *tree,
			   Parasitic *parasitic,
			   const Net *net,
			   const Pin *pin,
			   int steiner_pt)
{
  if (pin == nullptr)
    // If the steiner pt is on top of a pin, use the pin instead.
    pin = tree->steinerPtAlias(steiner_pt);
  if (pin)
    return parasitics_->ensureParasiticNode(parasitic, pin);
  else 
    return parasitics_->ensureParasiticNode(parasitic, net, steiner_pt);
}

bool
Resizer::hasTopLevelPort(const Net *net)
{
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isTopLevelPort(pin))
      return true;
  }
  return false;
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
  Instance *top_inst = network_->topInstance();
  LibertyCell *tie_cell = tie_port->libertyCell();
  InstanceSeq insts;
  findCellInstances(tie_cell, insts);
  int tie_count = 0;
  Instance *parent = db_network_->topInstance();
  int separation_dbu = metersToDbu(separation);
  for (Instance *inst : insts) {
    Pin *drvr_pin = network_->findPin(inst, tie_port);
    const char *inst_name = network_->name(inst);
    Net *net = network_->net(drvr_pin);
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
	string load_net_name = makeUniqueNetName();
	Net *load_net = db_network_->makeNet(load_net_name.c_str(), top_inst);

	// Connect tie inst output.
	sta_->connectPin(tie, tie_port, load_net);

	// Connect load to tie output net.
	sta_->disconnectPin(load);
	Port *load_port = network_->port(load);
	sta_->connectPin(load_inst, load_port, load_net);

	design_area_ += area(db_network_->cell(tie_cell));
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

  if (tie_count > 0) {
    printf("Inserted %d tie %s instances.\n",
	   tie_count,
	   tie_cell->name());
    level_drvr_verticies_valid_ = false;
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
Resizer::repairHoldViolations(LibertyCell *buffer_cell)
{
  init();
  sta_->findRequireds();
  Search *search = sta_->search();
  VertexSet hold_failures;
  // Find endpoints with hold violation.
  for (Vertex *end : *sta_->search()->endpoints()) {
    if (!search->isClock(end)
	&& sta_->vertexSlack(end, MinMax::min()) < 0.0)
      hold_failures.insert(end);
  }
  if (hold_failures.size() > 0)
    printf("Found %lu endpoints with hold violations.\n",
	   hold_failures.size());
  else
    printf("No hold violations found.\n");
  if (debug_->check("repair_hold", 2)) {
    for (Vertex *end : hold_failures)
      printf(" %s\n", end->name(sdc_network_));
  }
  repairHoldViolations(hold_failures, buffer_cell);
}

// For testing/debug.
void
Resizer::repairHoldViolations(Pin *end_pin,
			      LibertyCell *buffer_cell)
{
  Vertex *end = graph_->pinLoadVertex(end_pin);
  VertexSet ends;
  ends.insert(end);

  init();
  sta_->findRequireds();
  repairHoldViolations(ends, buffer_cell);
}

void
Resizer::repairHoldViolations(VertexSet &ends,
			      LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  Slack worst_slack;
  Vertex *worst_vertex;
  sta_->worstSlack(MinMax::min(), worst_slack, worst_vertex);
  debugPrint1(debug_, "repair_hold", 1, "worst_slack=%s\n",
	      units_->timeUnit()->asString(worst_slack, 3));
  int repair_count = 1;

  int pass = 1;
  while (worst_slack < 0.0
	 // Make sure we are making progress.
	 && repair_count > 0) {
    debugPrint1(debug_, "repair_hold", 1, "pass %d\n", pass);
    repair_count = repairHoldPass(ends, buffer_cell);
    sta_->findRequireds();
    sta_->worstSlack(MinMax::min(), worst_slack, worst_vertex);
    debugPrint1(debug_, "repair_hold", 1, "worst_slack=%s\n",
		units_->timeUnit()->asString(worst_slack, 3));
    pass++;
  }
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d hold buffers.\n", inserted_buffer_count_);
    level_drvr_verticies_valid_ = false;
  }
}

int
Resizer::repairHoldPass(VertexSet &ends,
			LibertyCell *buffer_cell)
{
  VertexWeightMap weight_map;
  findFaninWeights(ends, weight_map);
  VertexSeq fanins;
  sortFaninsByWeight(weight_map, fanins);
  
  int repair_count = 0;
  for(int i = 0; i < fanins.size() && repair_count < 10 ; i++) {
    Vertex *vertex = fanins[i];
    Slack hold_slack = sta_->vertexSlack(vertex, MinMax::min());
    if (hold_slack < 0) {
      debugPrint4(debug_, "repair_hold", 2, " %s w=%s gap=%s #%d\n",
		  vertex->name(sdc_network_),
		  delayAsString(weight_map[vertex], this),
		  delayAsString(slackGap(vertex), this),
		  i);
      Pin *drvr_pin = vertex->pin();
      Net *net = network_->isTopLevelPort(drvr_pin)
	? network_->net(network_->term(drvr_pin))
	: network_->net(drvr_pin);
      // Hands off special nets.
      if (!isSpecial(net)) {
	makeHoldDelay(drvr_pin, hold_slack, buffer_cell);
	repair_count++;
	if (overMaxArea()) {
	  warn("max utilization reached.");
	  break;
	}
      }
    }
  }
  return repair_count;
}

void
Resizer::makeHoldDelay(Pin *drvr_pin,
		       Slack hold_slack,
		       LibertyCell *buffer_cell)
{
  Instance *parent = db_network_->topInstance();
  string net2_name = makeUniqueNetName();
  string buffer_name = makeUniqueInstName("hold");
  Net *net2 = db_network_->makeNet(net2_name.c_str(), parent);
  Instance *buffer = db_network_->makeInstance(buffer_cell,
					       buffer_name.c_str(),
					       parent);
  inserted_buffer_count_++;
  design_area_ += area(db_network_->cell(buffer_cell));

  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  debugPrint3(debug_, "repair_hold", 3, "insert %s -> %s -> %s\n",
	      sdc_network_->pathName(drvr_pin),
	      buffer_name.c_str(),
	      net2_name.c_str())
  Instance *drvr = db_network_->instance(drvr_pin);
  Port *drvr_port = db_network_->port(drvr_pin);
  Net *net = network_->isTopLevelPort(drvr_pin)
    ? db_network_->net(db_network_->term(drvr_pin))
    : db_network_->net(drvr_pin);
  sta_->disconnectPin(drvr_pin);
  sta_->connectPin(drvr, drvr_port, net2);
  sta_->connectPin(buffer, input, net2);
  sta_->connectPin(buffer, output, net);
  setLocation(buffer, db_network_->location(drvr_pin));
  if (have_estimated_parasitics_) {
    estimateWireParasitic(net);
    estimateWireParasitic(net2);
  }
}

void
Resizer::findFaninWeights(VertexSet &ends,
			  // Return value.
			  VertexWeightMap &weight_map)
{
  Search *search = sta_->search();
  SearchPredNonReg2 pred(sta_);
  BfsBkwdIterator iter(BfsIndex::other, &pred, this);
  for (Vertex *vertex : ends)
    iter.enqueue(vertex);

  while (iter.hasNext()) {
    Vertex *vertex = iter.next();
    if (!search->isClock(vertex)) {
      if (!search->isEndpoint(vertex)
	  && vertex->isDriver(db_network_))
	weight_map[vertex] += sta_->vertexSlack(vertex, MinMax::min());
      iter.enqueueAdjacentVertices(vertex);
    }
  }
}

void
Resizer::sortFaninsByWeight(VertexWeightMap &weight_map,
			    // Return value.
			    VertexSeq &fanins)
{
  for(auto vertex_weight : weight_map) {
    Vertex *vertex = vertex_weight.first;
    fanins.push_back(vertex);
  }
  sort(fanins, [&](Vertex *v1, Vertex *v2)
	       { float w1 = weight_map[v1];
		 float w2 = weight_map[v2];
		 if (fuzzyEqual(w1, w2)) {
		   float gap1 = slackGap(v1);
		   float gap2 = slackGap(v2);
		   // Break ties based on the hold/setup gap.
		   if (fuzzyEqual(gap1, gap2))
		     return v1->level() > v2->level();
		   else
		     return gap1 > gap2;
		 }
		 else
		   return w1 < w2;});
}

// Gap between min setup and hold slacks.
// This says how much head room there is for adding delay to fix a old violation
// before violating a setup check.
float
Resizer::slackGap(Vertex *vertex)
{
  Slack slacks[RiseFall::index_count][MinMax::index_count];
  sta_->vertexSlacks(vertex, slacks);
  return min(slacks[RiseFall::riseIndex()][MinMax::maxIndex()]
	     - slacks[RiseFall::riseIndex()][MinMax::minIndex()],
	     slacks[RiseFall::fallIndex()][MinMax::maxIndex()]
	     - slacks[RiseFall::fallIndex()][MinMax::minIndex()]);
}

static float
cellDriveResistance(const LibertyCell *cell)
{
  LibertyCellPortBitIterator port_iter(cell);
  while (port_iter.hasNext()) {
    LibertyPort *port = port_iter.next();
    if (port->direction()->isOutput())
      return port->driveResistance();
  }
  return 0.0;
}

////////////////////////////////////////////////////////////////

// Repair long wires, max slew, max capacitance, max fanout violations
// The whole enchilada.
void
Resizer::repairDesign(double max_wire_length, // meters
		      LibertyCell *buffer_cell)
{
  init();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  inserted_buffer_count_ = 0;
  VertexSeq drvrs;
  findLongWires(drvrs);
  int repair_count = 0;
  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int max_length = metersToDbu(max_wire_length);
  for (Vertex *drvr : drvrs) {
    Pin *drvr_pin = drvr->pin();
    Net *net = network_->net(drvr_pin);
    if (net
	&& !isClock(net)
	// Exclude tie hi/low cells.
	&& !isFuncOneZero(drvr_pin)
	&& !hasTopLevelPort(net)
	&& !isSpecial(net)) {
      repairNet(net, drvr, true, true, true, max_length, buffer_cell,
		repair_count, slew_violations, cap_violations,
		fanout_violations, length_violations);
    }
  }
  if (slew_violations > 0)
    printf("Found %d slew violations.\n", slew_violations);
  if (fanout_violations > 0)
    printf("Found %d fanout violations.\n", fanout_violations);
  if (cap_violations > 0)
    printf("Found %d capacitance violations.\n", cap_violations);
  if (length_violations > 0)
    printf("Found %d long wires.\n", length_violations);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
	   inserted_buffer_count_,
	   repair_count);
    level_drvr_verticies_valid_ = false;
  }
}

// repairDesign but restricted to clock network and
// no max_fanout/max_cap checks.
void
Resizer::repairClkNets(double max_wire_length, // meters
		       LibertyCell *buffer_cell)
{
  init();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  inserted_buffer_count_ = 0;
  int repair_count = 0;
  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int max_length = metersToDbu(max_wire_length);
  for (Net *net : clk_nets_) {
    PinSet *drivers = network_->drivers(net);
    if (drivers) {
      PinSet::Iterator drvr_iter(drivers);
      Pin *drvr_pin = drvr_iter.next();
      Vertex *drvr = graph_->pinDrvrVertex(drvr_pin);
      repairNet(net, drvr, false, false, false, max_length, buffer_cell,
		repair_count, slew_violations, cap_violations,
		fanout_violations, length_violations);
    }
  }
  if (length_violations > 0)
    printf("Found %d long wires.\n", length_violations);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
	   inserted_buffer_count_,
	   repair_count);
    level_drvr_verticies_valid_ = false;
  }
}

// for debugging
void
Resizer::repairNet(Net *net,
		   double max_wire_length, // meters
		   LibertyCell *buffer_cell)
{
  init();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();
  sta_->checkSlewLimitPreamble();
  sta_->checkCapacitanceLimitPreamble();
  sta_->checkFanoutLimitPreamble();

  inserted_buffer_count_ = 0;
  int repair_count = 0;
  int slew_violations = 0;
  int cap_violations = 0;
  int fanout_violations = 0;
  int length_violations = 0;
  int max_length = metersToDbu(max_wire_length);
  PinSet *drivers = network_->drivers(net);
  if (drivers) {
    PinSet::Iterator drvr_iter(drivers);
    Pin *drvr_pin = drvr_iter.next();
    Vertex *drvr = graph_->pinDrvrVertex(drvr_pin);
    repairNet(net, drvr, true, true, true, max_length, buffer_cell,
	      repair_count, slew_violations, cap_violations,
	      fanout_violations, length_violations);
  }
  if (slew_violations > 0)
    printf("Found %d slew violations.\n", slew_violations);
  if (fanout_violations > 0)
    printf("Found %d fanout violations.\n", fanout_violations);
  if (cap_violations > 0)
    printf("Found %d capacitance violations.\n", cap_violations);
  if (length_violations > 0)
    printf("Found %d long wires.\n", length_violations);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
	   inserted_buffer_count_,
	   repair_count);
    level_drvr_verticies_valid_ = false;
  }
}

void
Resizer::repairNet(Net *net,
		   Vertex *drvr,
		   bool check_slew,
		   bool check_cap,
		   bool check_fanout,
		   int max_length, // dbu
		   LibertyCell *buffer_cell,
		   int &repair_count,
		   int &slew_violations,
		   int &cap_violations,
		   int &fanout_violations,
		   int &length_violations)
{
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
  if (tree) {
    Pin *drvr_pin = drvr->pin();
    bool repair = false;
    float max_cap = INF;
    float max_fanout = INF;
    if (check_slew) {
      float slew, slew_slack, max_slew;
      const Corner *corner1;
      const RiseFall *tr;
      sta_->checkSlew(drvr_pin, corner_, MinMax::max(), false,
		      corner1, tr, slew, max_slew, slew_slack);
      if (slew_slack < 0.0) {
	slew_violations++;
	LibertyPort *drvr_port = network_->libertyPort(drvr_pin);
	if (drvr_port) {
	  // Find max load cap that corresponds to max_slew.
	  max_cap = findSlewLoadCap(drvr_port, max_slew);
	  repair = true;
	}
      }
    }
    if (check_cap) {
      float cap, max_cap1, cap_slack;
      const Corner *corner1;
      const RiseFall *tr;
      sta_->checkCapacitance(drvr_pin, corner_, MinMax::max(),
			     corner1, tr, cap, max_cap1, cap_slack);
      if (cap_slack < 0.0) {
	max_cap = min(max_cap, max_cap1);
	cap_violations++;
	repair = true;
      }
    }
    if (check_fanout) {
      float fanout, fanout_slack;
      sta_->checkFanout(drvr_pin, MinMax::max(),
			fanout, max_fanout, fanout_slack);
      if (fanout_slack < 0.0) {
	fanout_violations++;
	repair = true;
      }
    }
    int wire_length = findMaxSteinerDist(drvr, tree);
    if (max_length
	&& wire_length > max_length) {
      length_violations++;
      repair = true;
    }
    if (repair) {
      Point drvr_loc = db_network_->location(drvr->pin());
      debugPrint4(debug_, "repair_net", 1, "driver %s (%s %s) l=%s\n",
		  sdc_network_->pathName(drvr_pin),
		  units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getX()), 0),
		  units_->distanceUnit()->asString(dbuToMeters(drvr_loc.getY()), 0),
		  units_->distanceUnit()->asString(dbuToMeters(wire_length), 0));
      SteinerPt drvr_pt = tree->steinerPt(drvr_pin);
      int ignore1;
      float ignore2, ignore3;
      PinSeq ignore4;
      repairNet(tree, drvr_pt, SteinerTree::null_pt, net,
		max_cap, max_fanout, max_length, buffer_cell, 0,
		ignore1, ignore2, ignore3, ignore4);
      repair_count++;
    }
  }
}

// Find the output port load capacitance that results in slew.
double
Resizer::findSlewLoadCap(LibertyPort *drvr_port,
			 double slew)
{
  // cap1 lower bound
  // cap2 upper bound
  double cap1 = 0.0;
  double cap2 = slew / drvr_port->driveResistance() * 2;
  double tol = .01; // 1%
  double diff1 = gateSlewDiff(drvr_port, cap1, slew);
  double diff2 = gateSlewDiff(drvr_port, cap2, slew);
  // binary search for diff = 0.
  while (abs(cap1 - cap2) > max(cap1, cap2) * tol) {
    if (diff2 < 0.0) {
      cap1 = cap2;
      diff1 = diff2;
      cap2 *= 2;
      diff2 = gateSlewDiff(drvr_port, cap2, slew);
    }
    else {
      double cap3 = (cap1 + cap2) / 2.0;
      double diff3 = gateSlewDiff(drvr_port, cap3, slew);
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
		      double slew)
{
  ArcDelay delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(drvr_port, load_cap, delays, slews);
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
		   LibertyCell *buffer_cell,
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
  debugPrint4(debug_, "repair_net", 1, "%*spt (%s %s)\n",
	      level, "",
	      units_->distanceUnit()->asString(dbuToMeters(pt_x), 0),
	      units_->distanceUnit()->asString(dbuToMeters(pt_y), 0));
  SteinerPt left = tree->left(pt);
  int wire_length_left = 0;
  float pin_cap_left = 0.0;
  float fanout_left = 0.0;
  PinSeq loads_left;
  if (left != SteinerTree::null_pt)
    repairNet(tree, left, pt, net, max_cap, max_fanout, max_length,
	      buffer_cell, level + 1,
	      wire_length_left, pin_cap_left, fanout_left, loads_left);
  SteinerPt right = tree->right(pt);
  int wire_length_right = 0;
  float pin_cap_right = 0.0;
  float fanout_right = 0.0;
  PinSeq loads_right;
  if (right != SteinerTree::null_pt)
    repairNet(tree, right, pt, net, max_cap, max_fanout, max_length,
	      buffer_cell, level + 1,
	      wire_length_right, pin_cap_right, fanout_right, loads_right);
  debugPrint6(debug_, "repair_net", 1, "%*sleft=%s %s right=%s %s\n",
	      level, "",
	      units_->distanceUnit()->asString(dbuToMeters(wire_length_left), 0),
	      units_->capacitanceUnit()->asString(pin_cap_left, 2),
	      units_->distanceUnit()->asString(dbuToMeters(wire_length_right), 0),
	      units_->capacitanceUnit()->asString(pin_cap_right, 2));

  LibertyPort *buffer_input_port, *buffer_output_port;
  buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

  bool repeater_left = false;
  bool repeater_right = false;
  // Add a buffer to left or right branch to stay under the max length.
  double cap_left = pin_cap_left + dbuToMeters(wire_length_left) * wire_cap_;
  double cap_right = pin_cap_right + dbuToMeters(wire_length_right) * wire_cap_;
  if ((cap_left + cap_right) > max_cap) {
    if (cap_left > cap_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  if (max_length > 0
       && (wire_length_left + wire_length_right) > max_length) {
    if (wire_length_left > wire_length_right)
      repeater_left = true;
    else
      repeater_right = true;
  }
  if (max_fanout > 0
       && (fanout_left + fanout_right) > max_fanout) {
    if (fanout_left > fanout_right)
      repeater_left = true;
    else
      repeater_right = true;
  }

  if (repeater_left)
    makeRepeater(tree, pt, net, buffer_cell, level,
		 wire_length_left, pin_cap_left, fanout_left, loads_left);
  if (repeater_right)
    makeRepeater(tree, pt, net, buffer_cell, level,
		 wire_length_right, pin_cap_right, fanout_right, loads_right);

  wire_length = wire_length_left + wire_length_right;
  pin_cap = pin_cap_left + pin_cap_right;
  fanout = fanout_left + fanout_right;

  // Union left/right load pins.
  load_pins = loads_left;
  for (Pin *load_pin : loads_right)
    load_pins.push_back(load_pin);

  Net *buffer_out = nullptr;
  // Steiner pt pin is the net driver if prev_pt is null.
  if (prev_pt != SteinerTree::null_pt) {
    Pin *load_pin = tree->pin(pt);
    if (load_pin) {
      Point load_loc = db_network_->location(load_pin);
      debugPrint5(debug_, "repair_net", 1, "%*sload %s (%s %s)\n",
		  level, "",
		  sdc_network_->pathName(load_pin),
		  units_->distanceUnit()->asString(dbuToMeters(load_loc.getX()), 0),
		  units_->distanceUnit()->asString(dbuToMeters(load_loc.getY()), 0));
      LibertyPort *load_port = network_->libertyPort(load_pin);
      if (load_port) {
	pin_cap += portCapacitance(load_port);
	fanout += portFanoutLoad(load_port);
      }
      else
	fanout += 1;
      load_pins.push_back(load_pin);
    }

    // Back up from pt to prev_pt adding repeaters every max_length.
    if (max_length > 0) {
      Point prev_loc = tree->location(prev_pt);
      int length = Point::manhattanDistance(prev_loc, pt_loc);
      wire_length += length;
      int prev_x = prev_loc.getX();
      int prev_y = prev_loc.getY();
      debugPrint4(debug_, "repair_net", 1, "%*swl=%s l=%s\n",
		  level, "",
		  units_->distanceUnit()->asString(dbuToMeters(wire_length), 0),
		  units_->distanceUnit()->asString(dbuToMeters(length), 0));
      while (wire_length > max_length) {
	// Distance from pt to repeater backward toward prev_pt.
	double buf_dist = length - (wire_length - max_length);
	double dx = prev_x - pt_x;
	double dy = prev_y - pt_y;
	double d = buf_dist / length;
	int buf_x = pt_x + d * dx;
	int buf_y = pt_y + d * dy;
	makeRepeater(buf_x, buf_y, net, buffer_cell, level,
		     wire_length, pin_cap, fanout, load_pins);
	// Update for the next round.
	length -= buf_dist;
	wire_length = length;
	pt_x = buf_x;
	pt_y = buf_y;
	debugPrint4(debug_, "repair_net", 1, "%*swl=%s l=%s\n",
		    level, "",
		    units_->distanceUnit()->asString(dbuToMeters(wire_length), 0),
		    units_->distanceUnit()->asString(dbuToMeters(length), 0));
      }
    }
  }
  else if (have_estimated_parasitics_)
    // Estimate parasitics at original driver net (tree root).
    estimateWireParasitic(net);
}

void
Resizer::makeRepeater(SteinerTree *tree,
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
  makeRepeater(pt_loc.getX(), pt_loc.getY(), in_net, buffer_cell, level,
	       wire_length, pin_cap, fanout, load_pins);
}

void
Resizer::makeRepeater(int x,
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
    debugPrint5(debug_, "repair_net", 2, "%*s%s (%s %s)\n",
		level, "",
		buffer_name.c_str(),
		units_->distanceUnit()->asString(dbuToMeters(x), 0),
		units_->distanceUnit()->asString(dbuToMeters(y), 0));

    string buffer_out_name = makeUniqueNetName();
    Instance *parent = db_network_->topInstance();
    Net *buffer_out = db_network_->makeNet(buffer_out_name.c_str(), parent);
    Instance *buffer = db_network_->makeInstance(buffer_cell,
						 buffer_name.c_str(),
						 parent);
    setLocation(buffer, buf_loc);
    design_area_ += area(db_network_->cell(buffer_cell));
    inserted_buffer_count_++;

    sta_->connectPin(buffer, buffer_input_port, in_net);
    sta_->connectPin(buffer, buffer_output_port, buffer_out);

    for (Pin *load_pin : load_pins) {
      Port *load_port = network_->port(load_pin);
      Instance *load = network_->instance(load_pin);
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load, load_port, buffer_out);
    }
    if (have_estimated_parasitics_)
      estimateWireParasitic(buffer_out);

    Pin *buf_in_pin = network_->findPin(buffer, buffer_input_port);
    load_pins.clear();
    load_pins.push_back(buf_in_pin);
    wire_length = 0;
    pin_cap = portCapacitance(buffer_input_port);
    fanout = portFanoutLoad(buffer_input_port);
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::reportLongWires(int count,
			 int digits)
{
  graph_ = sta_->ensureGraph();
  VertexSeq drvrs;
  findLongWires(drvrs);
  report_->print("Driver    length delay\n");
  for (int i = 0; i < count && i < drvrs.size(); i++) {
    Vertex *drvr = drvrs[i];
    Pin *drvr_pin = drvr->pin();
    if (!network_->isTopLevelPort(drvr_pin)) {
      double wire_length = dbuToMeters(maxLoadManhattenDistance(drvr));
      double steiner_length = dbuToMeters(findMaxSteinerDist(drvr));
      double delay = wire_length * wire_res_ * wire_length * wire_cap_ * 0.5;
      report_->print("%s manhtn %s steiner %s %s\n",
		     sdc_network_->pathName(drvr_pin),
		     units_->distanceUnit()->asString(wire_length, 0),
		     units_->distanceUnit()->asString(steiner_length, 0),
		     units_->timeUnit()->asString(delay, digits));
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
      if (!isClock(net)
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
      if (!isClock(net)
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
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
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
Resizer::bufferInputCapacitance(LibertyCell *buffer_cell)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  return portCapacitance(input);
}

float
Resizer::pinCapacitance(const Pin *pin)
{
  LibertyPort *port = network_->libertyPort(pin);
  if (port)
    return portCapacitance(port);
  else
    return 0.0;
}

float
Resizer::portCapacitance(const LibertyPort *port)
{
  float cap1 = port->capacitance(RiseFall::rise(), min_max_);
  float cap2 = port->capacitance(RiseFall::fall(), min_max_);
  return max(cap1, cap2);
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

Requireds
Resizer::pinRequireds(const Pin *pin)
{
  Vertex *vertex = graph_->pinLoadVertex(pin);
  PathAnalysisPt *path_ap = corner_->findPathAnalysisPt(min_max_);
  Requireds requireds;
  for (RiseFall *rf : RiseFall::range()) {
    int rf_index = rf->index();
    Required required = sta_->vertexRequired(vertex, rf, path_ap);
    if (fuzzyInf(required))
      // Unconstrained pin.
      required = 0.0;
    requireds[rf_index] = required;
  }
  return requireds;
}

float
Resizer::bufferDelay(LibertyCell *buffer_cell,
		     RiseFall *rf,
		     float load_cap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  ArcDelay gate_delays[RiseFall::index_count];
  Slew slews[RiseFall::index_count];
  gateDelays(output, load_cap, gate_delays, slews);
  return gate_delays[rf->index()];
}

// Rise/fall delays across all timing arcs into drvr_port.
// Uses target slew for input slew.
void
Resizer::gateDelays(LibertyPort *drvr_port,
		    float load_cap,
		    // Return values.
		    ArcDelay delays[RiseFall::index_count],
		    Slew slews[RiseFall::index_count])
{
  for (auto rf_index : RiseFall::rangeIndex()) {
    delays[rf_index] = -INF;
    slews[rf_index] = -INF;
  }
  LibertyCell *cell = drvr_port->libertyCell();
  LibertyCellTimingArcSetIterator set_iter(cell);
  while (set_iter.hasNext()) {
    TimingArcSet *arc_set = set_iter.next();
    if (arc_set->to() == drvr_port) {
      TimingArcSetArcIterator arc_iter(arc_set);
      while (arc_iter.hasNext()) {
	TimingArc *arc = arc_iter.next();
	RiseFall *in_rf = arc->fromTrans()->asRiseFall();
	int out_rf_index = arc->toTrans()->asRiseFall()->index();
	float in_slew = tgt_slews_[in_rf->index()];
	ArcDelay gate_delay;
	Slew drvr_slew;
	arc_delay_calc_->gateDelay(cell, arc, in_slew, load_cap,
				   nullptr, 0.0, pvt_, dcalc_ap_,
				   gate_delay,
				   drvr_slew);
	delays[out_rf_index] = max(delays[out_rf_index], gate_delay);
	slews[out_rf_index] = max(slews[out_rf_index], drvr_slew);
      }
    }
  }
}

////////////////////////////////////////////////////////////////

// Find the max wire length before it is faster to split the wire
// in half with a buffer (in meters).
double
Resizer::findMaxWireLength(LibertyCell *buffer_cell)
{
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  double drvr_r = max(drvr_port->driveResistance(RiseFall::rise(), MinMax::max()),
		      drvr_port->driveResistance(RiseFall::fall(), MinMax::max()));
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  double wire_length2 = drvr_r / wire_res_;
  double tol = .01; // 1%
  double diff1 = splitWireDelayDiff(wire_length1, buffer_cell);
  double diff2 = splitWireDelayDiff(wire_length2, buffer_cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff2 < 0.0) {
      wire_length1 = wire_length2;
      diff1 = diff2;
      wire_length2 *= 2;
      diff2 = splitWireDelayDiff(wire_length2, buffer_cell);
    }
    else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff3 = splitWireDelayDiff(wire_length3, buffer_cell);
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

// Buffer delay plus wire delay.
// Uses target slew for input slew.
void
Resizer::bufferWireDelay(LibertyCell *buffer_cell,
			 double wire_length, // meters
			 // Return values.
			 Delay &delay,
			 Slew &slew)
{
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  Instance *top_inst = network_->topInstance();
  // Tmp net for parasitics to live on.
  Net *net = sta_->makeNet("wire", top_inst);
  Instance *drvr = sta_->makeInstance("drvr", buffer_cell, top_inst);
  Instance *load = sta_->makeInstance("load", buffer_cell, top_inst);
  sta_->connectPin(drvr, drvr_port, net);
  sta_->connectPin(drvr, load_port, net);
  Pin *drvr_pin = network_->findPin(drvr, drvr_port);
  Pin *load_pin = network_->findPin(load, load_port);

  Parasitic *parasitic = makeWireParasitic(net, drvr_pin, load_pin, wire_length);
  // Let delay calc reduce parasitic network as it sees fit.
  Parasitic *drvr_parasitic = arc_delay_calc_->findParasitic(drvr_pin, RiseFall::rise(),
							     dcalc_ap_);

  // Max rise/fall delays.
  delay = -INF;
  slew = -INF;
  LibertyCellTimingArcSetIterator set_iter(buffer_cell);
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
	arc_delay_calc_->gateDelay(buffer_cell, arc, in_slew, 0.0,
				   drvr_parasitic, 0.0, pvt_, dcalc_ap_,
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
  // Cleanup the turds.
  arc_delay_calc_->finishDrvrPin();
  parasitics_->deleteParasiticNetwork(net, dcalc_ap_->parasiticAnalysisPt());
  sta_->deleteInstance(drvr);
  sta_->deleteInstance(load);
  sta_->deleteNet(net);
}

Parasitic *
Resizer::makeWireParasitic(Net *net,
			   Pin *drvr_pin,
			   Pin *load_pin,
			   double wire_length) // meters
{
  Parasitic *parasitic = parasitics_->makeParasiticNetwork(net, false,
							   parasitics_ap_);
  ParasiticNode *n1 = parasitics_->ensureParasiticNode(parasitic, drvr_pin);
  ParasiticNode *n2 = parasitics_->ensureParasiticNode(parasitic, load_pin);
  double wire_cap = wire_length * wire_cap_;
  double wire_res = wire_length * wire_res_;
  parasitics_->incrCap(n1, wire_cap / 2.0, parasitics_ap_);
  parasitics_->makeResistor(nullptr, n1, n2, wire_res, parasitics_ap_);
  parasitics_->incrCap(n2, wire_cap / 2.0, parasitics_ap_);
  return parasitic;
}

////////////////////////////////////////////////////////////////

double
Resizer::findMaxSlewWireLength(double max_slew,
			       LibertyCell *buffer_cell)
{
  // wire_length1 lower bound
  // wire_length2 upper bound
  double wire_length1 = 0.0;
  double wire_length2 = std::sqrt(max_slew / (wire_res_ * wire_cap_));
  double tol = .01; // 1%
  double diff1 = maxSlewWireDiff(wire_length1, max_slew, buffer_cell);
  double diff2 = maxSlewWireDiff(wire_length2, max_slew, buffer_cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff2 < 0.0) {
      wire_length1 = wire_length2;
      diff1 = diff2;
      wire_length2 *= 2;
      diff2 = maxSlewWireDiff(wire_length2, max_slew, buffer_cell);
    }
    else {
      double wire_length3 = (wire_length1 + wire_length2) / 2.0;
      double diff3 = maxSlewWireDiff(wire_length3, max_slew, buffer_cell);
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
Resizer::maxSlewWireDiff(double wire_length,
			 double max_slew,
			 LibertyCell *buffer_cell)
{
  Delay delay;
  Slew slew;
  bufferWireDelay(buffer_cell, wire_length, delay, slew);
  return slew - max_slew;
}

////////////////////////////////////////////////////////////////

double
Resizer::designArea()
{
  ensureBlock();
  return design_area_;
}

void
Resizer::designAreaIncr(float delta) {
  design_area_ += delta;
}

double
Resizer::findDesignArea()
{
  double design_area = 0.0;
  for (dbInst *inst : block_->getInsts()) {
    dbMaster *master = inst->getMaster();
    design_area += area(master);
  }
  return design_area;
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

void
Resizer::writeNetSVG(Net *net,
		     const char *filename)
{
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
  if (tree)
    tree->writeSVG(sdc_network_, filename);
}

}
