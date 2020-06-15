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
//  Instance levelization and resizing to target slew only support single output gates
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

// Referenced by MakeSteinerTree also.
bool
pinIsPlaced(Pin *pin,
	    const dbNetwork *network);
Point
pinLocation(const Pin *pin,
	    const dbNetwork *network);

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
  min_max_(nullptr),
  dcalc_ap_(nullptr),
  pvt_(nullptr),
  parasitics_ap_(nullptr),
  clk_nets__valid_(false),
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
  ensureBlock();
  // Abbreviated copyState
  db_network_ = sta_->getDbNetwork();
  sta_->ensureLevelized();
  graph_ = sta_->graph();
  ensureLevelDrvrVerticies();
  ensureClkNets();
  ensureCorner();
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
  if (inserted_buffer_count_ > 0)
    level_drvr_verticies_valid_ = false;
  printf("Inserted %d input buffers.\n", inserted_buffer_count_);
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
    setLocation(buffer, pinLocation(top_pin, db_network_));
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
  if (core_exists_) {
    Point inside = closestPtInRect(core_, pt);
    dinst->setLocation(inside.getX(), inside.getY());
  }
  else
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
  if (inserted_buffer_count_ > 0)
    level_drvr_verticies_valid_ = false;
  printf("Inserted %d output buffers.\n", inserted_buffer_count_);
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
    setLocation(buffer, pinLocation(top_pin, db_network_));
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
    Vertex *vertex = level_drvr_verticies_[i];
    Pin *drvr_pin = vertex->pin();
    Instance *inst = network_->instance(drvr_pin);
    resizeToTargetSlew(inst);
    if (overMaxArea()) {
      warn("Max utilization reached.");
      break;
    }
  }
  printf("Resized %d instances.\n", resize_count_);
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
Resizer::resizeToTargetSlew(Instance *inst)
{
  NetworkEdit *network = networkEdit();
  LibertyCell *cell = network_->libertyCell(inst);
  if (cell) {
    Pin *output = singleOutputPin(inst);
    // Only resize single output gates for now.
    if (output) {
      Net *out_net = network->net(output);
      if (out_net
	  // Hands off the clock nets.
	  && !isClock(out_net)
	  // Exclude tie hi/low cells.
	  && !isFuncOneZero(output)
	  // Hands off special nets.
	  && !isSpecial(out_net)) {
	// Includes net parasitic capacitance.
	float load_cap = graph_delay_calc_->loadCap(output, dcalc_ap_);
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
  }
}

Pin *
Resizer::singleOutputPin(const Instance *inst)
{
  Pin *output = nullptr;
  InstancePinIterator *pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->direction(pin)->isOutput()) {
      if (output) {
	// Already found one.
	delete pin_iter;
	return nullptr;
      }
      output = pin;
    }
  }
  delete pin_iter;
  return output;
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
  if (!clk_nets__valid_) {
    findClkNets();
    clk_nets__valid_ = true;
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
      // Hands off the clock nets.
      if (!isClock(net)
	  && !network_->isPower(net)
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
  SteinerTree *tree = makeSteinerTree(net, false, db_network_);
  if (tree
      // Do not add parasitics on input ports.
      // When the input drives a pad instance with huge input
      // cap the elmore delay is gigantic.
      && !hasInputPort(tree)) {
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
  }
  delete tree;
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
Resizer::hasInputPort(SteinerTree *tree)
{
  for (Pin *pin : tree->pins()) {
    if (network_->isTopLevelPort(pin)
	&& network_->direction(pin)->isAnyInput())
      return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////

void
Resizer::writeNetSVG(Net *net,
		     const char *filename)
{
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
  if (tree)
    tree->writeSVG(sdc_network_, filename);
}

////////////////////////////////////////////////////////////////

class RebufferOption
{
public:
  RebufferOption(RebufferOptionType type,
		 float cap,
		 Requireds requireds,
		 Pin *load_pin,
		 Point location,
		 RebufferOption *ref,
		 RebufferOption *ref2);
  ~RebufferOption();
  void print(int level,
	     Resizer *resizer);
  void printTree(Resizer *resizer);
  void printTree(int level,
		 Resizer *resizer);
  RebufferOptionType type() const { return type_; }
  float cap() const { return cap_; }
  Required required(RiseFall *rf) { return requireds_[rf->index()]; }
  Required requiredMin();
  Requireds bufferRequireds(LibertyCell *buffer_cell,
			    Resizer *resizer) const;
  Required bufferRequired(LibertyCell *buffer_cell,
			  Resizer *resizer) const;
  Point location() const { return location_; }
  Pin *loadPin() const { return load_pin_; }
  RebufferOption *ref() const { return ref_; }
  RebufferOption *ref2() const { return ref2_; }
  int bufferCount() const;

private:
  RebufferOptionType type_;
  float cap_;
  Requireds requireds_;
  Pin *load_pin_;
  Point location_;
  RebufferOption *ref_;
  RebufferOption *ref2_;
};

RebufferOption::RebufferOption(RebufferOptionType type,
			       float cap,
			       Requireds requireds,
			       Pin *load_pin,
			       Point location,
			       RebufferOption *ref,
			       RebufferOption *ref2) :
  type_(type),
  cap_(cap),
  requireds_(requireds),
  load_pin_(load_pin),
  location_(location),
  ref_(ref),
  ref2_(ref2)
{
}

RebufferOption::~RebufferOption()
{
}

void
RebufferOption::printTree(Resizer *resizer)
{
  printTree(0, resizer);
}

void
RebufferOption::printTree(int level,
			  Resizer *resizer)
{
  print(level, resizer);
  switch (type_) {
  case RebufferOptionType::sink:
    break;
  case RebufferOptionType::buffer:
  case RebufferOptionType::wire:
    ref_->printTree(level + 1, resizer);
    break;
  case RebufferOptionType::junction:
    ref_->printTree(level + 1, resizer);
    ref2_->printTree(level + 1, resizer);
    break;
  }
}

void
RebufferOption::print(int level,
		      Resizer *resizer)
{
  Network *sdc_network = resizer->sdcNetwork();
  Units *units = resizer->units();
  switch (type_) {
  case RebufferOptionType::sink:
    // %*s format indents level spaces.
    printf("%*sload %s (%d, %d) cap %s req %s\n",
	   level, "",
	   sdc_network->pathName(load_pin_),
	   location_.x(), location_.y(),
	   units->capacitanceUnit()->asString(cap_),
	   delayAsString(requiredMin(), resizer));
    break;
  case RebufferOptionType::wire:
    printf("%*swire (%d, %d) cap %s req %s\n",
	   level, "",
	   location_.x(), location_.y(),
	   units->capacitanceUnit()->asString(cap_),
	   delayAsString(requiredMin(), resizer));
    break;
  case RebufferOptionType::buffer:
    printf("%*sbuffer (%d, %d) cap %s req %s\n",
	   level, "",
	   location_.x(), location_.y(),
	   units->capacitanceUnit()->asString(cap_),
	   delayAsString(requiredMin(), resizer));
    break;
  case RebufferOptionType::junction:
    printf("%*sjunction (%d, %d) cap %s req %s\n",
	   level, "",
	   location_.x(), location_.y(),
	   units->capacitanceUnit()->asString(cap_),
	   delayAsString(requiredMin(), resizer));
    break;
  }
}

Required
RebufferOption::requiredMin()
{
  return min(requireds_[RiseFall::riseIndex()],
	     requireds_[RiseFall::fallIndex()]);
}

// Required times at input of buffer_cell driving this option.
Requireds
RebufferOption::bufferRequireds(LibertyCell *buffer_cell,
				Resizer *resizer) const
{
  Requireds requireds = {requireds_[RiseFall::riseIndex()]
			 - resizer->bufferDelay(buffer_cell, RiseFall::rise(), cap_),
			 requireds_[RiseFall::fallIndex()]
			 - resizer->bufferDelay(buffer_cell, RiseFall::fall(), cap_)};
  return requireds;
}

Required
RebufferOption::bufferRequired(LibertyCell *buffer_cell,
			       Resizer *resizer) const
{
  Requireds requireds = bufferRequireds(buffer_cell, resizer);
  return min(requireds[RiseFall::riseIndex()],
	     requireds[RiseFall::fallIndex()]);
}

int
RebufferOption::bufferCount() const
{
  switch (type_) {
  case RebufferOptionType::buffer:
    return ref_->bufferCount() + 1;
  case RebufferOptionType::wire:
    return ref_->bufferCount();
  case RebufferOptionType::junction:
    return ref_->bufferCount() + ref2_->bufferCount();
  case RebufferOptionType::sink:
    return 0;
  }
  return 0;
}

RebufferOption *
Resizer::makeRebufferOption(RebufferOptionType type,
			    float cap,
			    Requireds requireds,
			    Pin *load_pin,
			    Point location,
			    RebufferOption *ref,
			    RebufferOption *ref2)
{
  RebufferOption *option = new RebufferOption(type, cap, requireds,
					      load_pin, location, ref, ref2);

  rebuffer_options_.push_back(option);
  return option;
}

void
Resizer::deleteRebufferOptions()
{
  rebuffer_options_.deleteContentsClear();
}

////////////////////////////////////////////////////////////////

bool use_rebuffer = false;

// Assumes resizePreamble has been called.
void
Resizer::repairMaxCap(LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  int repaired_net_count = 0;
  int violation_count = 0;

  sta_->checkCapacitanceLimitPreamble();
  sta_->findDelays();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    Pin *drvr_pin = vertex->pin();
    // Hands off the clock tree.
    Net *net = network_->net(drvr_pin);
    if (net
	&& !isClock(net)
	// Exclude tie hi/low cells.
	&& !isFuncOneZero(drvr_pin)
	// Hands off special nets.
	&& !isSpecial(net)) {
      const Corner *ignore_corner;
      const RiseFall *ignore_rf;
      float cap, limit, slack;
      sta_->checkCapacitance(drvr_pin, corner_, MinMax::max(), 
			     ignore_corner, ignore_rf, cap, limit, slack);
      if (slack < 0.0 && limit > 0.0) {
	violation_count++;
	repaired_net_count++;
	Pin *drvr_pin = vertex->pin();
	if (use_rebuffer)
	  rebuffer(drvr_pin, buffer_cell);
	else {
	  float limit_ratio = cap / limit;
	  int buffer_count = ceil(limit_ratio);
	  int buffer_fanout = ceil(fanout(drvr_pin) / static_cast<double>(buffer_count));
	  bufferLoads(drvr_pin, buffer_count, buffer_fanout,
		      buffer_cell, "max_cap");
	  if (overMaxArea()) {
	    warn("max utilization reached.");
	    break;
	  }
	}
      }
    }
  }
  
  if (violation_count > 0)
    printf("Found %d max capacitance violations.\n", violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
	   inserted_buffer_count_,
	   repaired_net_count);
    level_drvr_verticies_valid_ = false;
  }
}

void
Resizer::repairMaxSlew(LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  int repaired_net_count = 0;
  int violation_count = 0;

  sta_->findDelays();
  sta_->checkSlewLimitPreamble();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    const Pin *drvr_pin = vertex->pin();
    Net *net = network_->net(vertex->pin());
    if (net
	// Hands off the clock tree.
	&& !isClock(net)
	// Exclude tie hi/low cells.
	&& !isFuncOneZero(drvr_pin)
	// Hands off special nets.
	&& !isSpecial(net)) {
      // Max slew can be specified for input and output pin in liberty
      // so all the pins have to be checked.
      bool violation = false;
      float limit_ratio;
      NetPinIterator *pin_iter = network_->pinIterator(net);
      while (pin_iter->hasNext()) {
	Pin *pin = pin_iter->next();
	const Corner *ignore_corner;
	const RiseFall *ignore_rf;
	Slew slew;
	float limit, slack;
	sta_->checkSlew(pin, corner_, MinMax::max(), false,
			ignore_corner, ignore_rf, slew, limit, slack);
	if (slack < 0.0 && limit > 0.0) {
	  violation = true;
	  limit_ratio = slew / limit;
	  violation_count++;
	  break;
	}
      }
      delete pin_iter;
      if (violation) {
	Pin *drvr_pin = vertex->pin();
	if (use_rebuffer)
	  rebuffer(drvr_pin, buffer_cell);
	else {
	  int buffer_count = ceil(limit_ratio);
	  int buffer_fanout = ceil(fanout(drvr_pin) / static_cast<double>(buffer_count));
	  bufferLoads(drvr_pin, buffer_count, buffer_fanout, buffer_cell, "max_slew");
	  repaired_net_count++;
	  if (overMaxArea()) {
	    warn("max utilization reached.");
	    break;
	  }
	}
      }
    }
  }
  
  if (violation_count > 0)
    printf("Found %d max slew violations.\n", violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
	   inserted_buffer_count_,
	   repaired_net_count);
    level_drvr_verticies_valid_ = false;
  }
}

void
Resizer::repairMaxFanout(LibertyCell *buffer_cell)
{
  int max_fanout_violation_count = 0;
  inserted_buffer_count_ = 0;

  init();
  sta_->checkFanoutLimitPreamble();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    Pin *drvr_pin = vertex->pin();
    Net *net = network_->net(drvr_pin);
    // Hands off the clock tree.
    if (!network_->isTopLevelPort(drvr_pin)
	&& !isClock(net)
	// Exclude tie hi/low cells.
	&& !isFuncOneZero(drvr_pin)
	&& net
	&& !isSpecial(net)) {
      float fanout, max_fanout, slack;
      sta_->checkFanout(drvr_pin, MinMax::max(),
			fanout, max_fanout, slack);
      if (slack < 0.0) {
	max_fanout_violation_count++;
	if (use_rebuffer)
	  rebuffer(drvr_pin, buffer_cell);
	else {
	  int buffer_count = ceil(fanout / max_fanout);
	  int buffer_fanout = ceil(fanout / static_cast<double>(buffer_count));
	  bufferLoads(drvr_pin, buffer_count, buffer_fanout,
		      buffer_cell, "max_fanout");
	  if (overMaxArea()) {
	    warn("max utilization reached.");
	    break;
	  }
	}
      }
    }
  }

  if (max_fanout_violation_count > 0)
    printf("Found %d max fanout violations.\n", max_fanout_violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers.\n", inserted_buffer_count_);
    level_drvr_verticies_valid_ = false;
  }
}

void
Resizer::bufferLoads(Pin *drvr_pin,
		     int buffer_count,
		     int max_fanout,
		     LibertyCell *buffer_cell,
		     const char *reason)
{
  debugPrint1(debug_, "buffer_loads", 2, "driver %s\n",
	      sdc_network_->pathName(drvr_pin));
  GroupedPins grouped_loads;
  groupLoadsSteiner(drvr_pin, buffer_count, max_fanout, grouped_loads);
  //reportGroupedLoads(grouped_loads);
  Vector<Instance*> buffers(buffer_count);
  Net *net = network_->net(drvr_pin);
  Instance *top_inst = db_network_->topInstance();
  LibertyPort *buffer_in, *buffer_out;
  buffer_cell->bufferPorts(buffer_in, buffer_out);
  for (int i = 0; i < buffer_count; i++) {
    PinSeq &loads = grouped_loads[i];
    if (loads.size()) {
      Point center = findCenter(loads);
      string load_net_name = makeUniqueNetName();
      Net *load_net = db_network_->makeNet(load_net_name.c_str(), top_inst);
      string inst_name = makeUniqueInstName(reason);
      Instance *buffer = db_network_->makeInstance(buffer_cell,
						   inst_name.c_str(),
						   top_inst);
      setLocation(buffer, center);
      inserted_buffer_count_++;
      design_area_ += area(db_network_->cell(buffer_cell));

      sta_->connectPin(buffer, buffer_in, net);
      sta_->connectPin(buffer, buffer_out, load_net);

      for (Pin *load : loads) {
	Instance *load_inst = network_->instance(load);
	Port *load_port = network_->port(load);
	sta_->disconnectPin(load);
	sta_->connectPin(load_inst, load_port, load_net);
      }
      if (have_estimated_parasitics_)
	estimateWireParasitic(load_net);
    }
  }
  if (have_estimated_parasitics_)
    estimateWireParasitic(net);
}

// K means clustering algorithm.
// not used
void
Resizer::groupLoadsCluster(Pin *drvr_pin,
			   int group_count,
			   int group_size,
			   // Return value.
			   GroupedPins &grouped_loads)
{
  Vector<Point> centers;
  grouped_loads.resize(group_count);
  centers.resize(group_count);

  PinSeq loads;
  findLoads(drvr_pin, loads);
  // Find the bbox of the loads.
  Rect bbox;
  bbox.mergeInit();
  for (Pin *load : loads) {
    Point loc = pinLocation(load, db_network_);
    Rect r(loc.x(), loc.y(), loc.x(), loc.y());
    bbox.merge(r);
  }
  // Choose initial centers on bbox diagonal.
  for(int i = 0; i < group_count; i++) {
    Point center(bbox.xMin() + i * bbox.dx() / group_count,
		    bbox.yMin() + i * bbox.dy() / group_count);
    centers[i] = center;
  }

  for (int j = 0; j < 10; j++) {
    for (int i = 0; i < group_count; i++)
      grouped_loads[i].clear();

    // Assign each load to the group with the nearest center.
    for (Pin *load : loads) {
      Point loc = pinLocation(load, db_network_);
      int64_t min_dist2 = std::numeric_limits<int64_t>::max();
      int min_index = 0;
      for(int i = 0; i < group_count; i++) {
	Point center = centers[i];
	int64_t dist2 = Point::squaredDistance(loc, center);
	if (dist2 < min_dist2) {
	  min_dist2 = dist2;
	  min_index = i;
	}
      }
      grouped_loads[min_index].push_back(load);
    }

    // Find the center of each group.
    for (int i = 0; i < group_count; i++) {
      Vector<Pin*> &loads = grouped_loads[i];
      Point center = findCenter(loads);
      centers[i] = center;
    }
  }
}

void
Resizer::groupLoadsSteiner(Pin *drvr_pin,
			   int group_count,
			   int group_size,
			   // Return value.
			   GroupedPins &grouped_loads)
{
  Net* net = sdc_network_->net(drvr_pin);
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
  grouped_loads.resize(group_count);
  if (tree) {
    SteinerPt drvr_pt = tree->drvrPt(db_network_);
    int group_index = 0;
    groupLoadsSteiner(tree, drvr_pt, group_size, group_index, grouped_loads);
  }
  else {
    PinSeq loads;
    findLoads(drvr_pin, loads);
    int group_index = 0;
    for (Pin *load : loads) {
      Vector<Pin*> &loads = grouped_loads[group_index];
      loads.push_back(load);
      if (loads.size() == group_size)
	group_index++;
    }
  }
}

// DFS of steiner tree collecting leaf pins into groups.
void
Resizer::groupLoadsSteiner(SteinerTree *tree,
			   SteinerPt pt,
			   int group_size,
			   int &group_index,
			   GroupedPins &grouped_loads)
{
  Pin *pin = tree->pin(pt);
  if (pin && db_network_->isLoad(pin)) {
    Vector<Pin*> &loads = grouped_loads[group_index];
    loads.push_back(pin);
    if (loads.size() == group_size)
      group_index++;
  }
  else {
    SteinerPt left = tree->left(pt);
    if (left != SteinerTree::null_pt)
      groupLoadsSteiner(tree, left, group_size, group_index, grouped_loads);
    SteinerPt right = tree->right(pt);
    if (right != SteinerTree::null_pt)
      groupLoadsSteiner(tree, right, group_size, group_index, grouped_loads);
  }
}

void
Resizer::reportGroupedLoads(GroupedPins &grouped_loads)
{
  int i = 0;
  for (Vector<Pin*> &loads : grouped_loads) {
    if (!loads.empty()) {
      printf("Group %d %lu members\n", i, loads.size());
      Point center = findCenter(loads);
      double max_dist = 0.0;
      double sum_dist = 0.0;
      for (Pin *load : loads) {
	uint64_t dist2 = Point::squaredDistance(pinLocation(load, db_network_),
						   center);
	double dist = std::sqrt(dist2);
	printf(" %.2e %s\n", dbuToMeters(dist), db_network_->pathName(load));
	sum_dist += dist;
	if (dist > max_dist)
	  max_dist = dist;
      }
      double avg_dist = std::sqrt(sum_dist / loads.size());
      printf(" avg dist %.2e\n", dbuToMeters(avg_dist));
      printf(" max dist %.2e\n", dbuToMeters(max_dist));
      i++;
    }
  }
}

Point
Resizer::findCenter(PinSeq &pins)
{
  Point sum(0, 0);
  for (Pin *pin : pins) {
    Point loc = pinLocation(pin, db_network_);
    sum.x() += loc.x();
    sum.y() += loc.y();
  }
  return Point(sum.x() / pins.size(), sum.y() / pins.size());
}

////////////////////////////////////////////////////////////////

// Repair tie hi/low net driver fanout by duplicating the
// tie hi/low instances.
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
	Instance *load_inst = network_->instance(load);
	string tie_name = makeUniqueInstName(inst_name, true);
	Instance *tie = sta_->makeInstance(tie_name.c_str(),
					   tie_cell, top_inst);
	Point tie_loc = tieLocation(load, separation_dbu);
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
  dbInst *db_inst = db_network_->staToDb(network_->instance(load));
  dbBox *bbox = db_inst->getBBox();
  Point load_loc = pinLocation(load, db_network_);
  int load_x = load_loc.getX();
  int load_y = load_loc.getY();
  int left_dist = abs(load_x - bbox->xMin());
  int right_dist = abs(load_x - bbox->xMax());
  int bot_dist = abs(load_y - bbox->yMin());
  int top_dist = abs(load_y - bbox->yMax());
  if (left_dist < right_dist
      && left_dist < bot_dist
      && left_dist < top_dist)
    // left
    return Point(load_x - separation, load_y);
  if (right_dist < left_dist
      && right_dist < bot_dist
      && right_dist < top_dist)
    // right
    return Point(load_x + separation, load_y);
  if (bot_dist < left_dist
      && bot_dist < right_dist
      && bot_dist < top_dist)
    // bot
    return Point(load_x, load_y - separation);
  if (top_dist < left_dist
      && top_dist < right_dist
      && top_dist < bot_dist)
    // top
    return Point(load_x, load_y + separation);
  // tie
  return Point(load_x, load_y);
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
  if (debug_->check("repair_hold", 2)) {
    printf("Failing endpoints %lu\n", hold_failures.size());
    for (Vertex *failing_end : hold_failures)
      printf(" %s\n", failing_end->name(sdc_network_));
  }
  repairHoldViolations(hold_failures, buffer_cell);
}

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

  int pass = 1;
  while (worst_slack < 0.0
	 && pass < 10) {
    repairHoldPass(ends, buffer_cell);
    sta_->findRequireds();
    sta_->worstSlack(MinMax::min(), worst_slack, worst_vertex);
    pass++;
  }
  printf("Inserted %d hold buffers.\n", inserted_buffer_count_);
}

void
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
      debugPrint3(debug_, "repair_hold", 2, " %s w=%s gap=%s\n",
		  vertex->name(sdc_network_),
		  delayAsString(weight_map[vertex], this),
		  delayAsString(slackGap(vertex), this));
      Pin *drvr_pin = vertex->pin();
      Net *net = network_->isTopLevelPort(drvr_pin)
	? network_->net(network_->term(drvr_pin))
	: network_->net(drvr_pin);
      // Hands off special nets.
      if (!isSpecial(net)) {
	repairHoldBuffer(drvr_pin, hold_slack, buffer_cell);
	repair_count++;
	if (overMaxArea()) {
	  warn("max utilization reached.");
	  break;
	}
      }
    }
  }
}

void
Resizer::repairHoldBuffer(Pin *drvr_pin,
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
  setLocation(buffer, pinLocation(drvr_pin, db_network_));
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

// This does not work well because downsizing the gate can lower the
// path delay by reducing the load capacitance on an earlier stage.
void
Resizer::repairHoldResize(Pin *drvr_pin,
			  Slack hold_slack)
{
  Instance *inst = network_->instance(drvr_pin);
  LibertyCell *inst_cell = network_->libertyCell(inst);
  LibertyCellSeq *equiv_cells = sta_->equivCells(inst_cell);
  bool found_cell = false;
  int i;
  for (i = 0; i < equiv_cells->size(); i++) {
    LibertyCell *equiv = (*equiv_cells)[i];
    if (equiv == inst_cell) {
      found_cell = true;
      break;
    }
  }
  // Equiv cells are sorted by drive strength.
  // If inst_cell is the first equiv it is already the lowest drive option.
  if (found_cell && i > 0) {
    float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap_);
    float drive_res = cellDriveResistance(inst_cell);
    float rc_delay0 = drive_res * load_cap;
    // Downsize until RC delay > hold violation.
    for (int k = i - 1; k >= 0; k--) {
      LibertyCell *equiv = (*equiv_cells)[k];
      float drive_res = cellDriveResistance(equiv);
      float rc_delay = drive_res * load_cap;
      if (rc_delay - rc_delay0 > -hold_slack
	  // Last chance baby.
	  || k == 0) {
	debugPrint4(debug_, "resizer", 3, "%s %s -> %s +%s\n",
		    sdc_network_->pathName(inst),
		    inst_cell->name(),
		    equiv->name(),
		    delayAsString(rc_delay - rc_delay0, this));
	sta_->replaceCell(inst, equiv);
	break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////

void
Resizer::repairLongWires(float max_length, // meters
			 LibertyCell *buffer_cell)
{
  graph_ = sta_->ensureGraph();
  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  inserted_buffer_count_ = 0;
  VertexSeq drvrs;
  findLongWires(drvrs);
  int repair_count = 0;
  int max_length_dbu = metersToDbu(max_length);
  for (Vertex *drvr : drvrs) {
    Point drvr_loc = pinLocation(drvr->pin(), db_network_);
    VertexOutEdgeIterator edge_iter(drvr, graph_);
    while (edge_iter.hasNext()) {
      Edge *edge = edge_iter.next();
      Vertex *load = edge->to(graph_);
      Pin *drvr_pin = drvr->pin();
      Pin *load_pin = load->pin();
      if (!(edge->isDisabledConstraint()
	    || load->isDisabledConstraint()
	    || network_->isTopLevelPort(drvr_pin)
	    || network_->isTopLevelPort(load_pin))) {
	Point load_loc = pinLocation(load->pin(), db_network_);
	float length = Point::manhattanDistance(load_loc, drvr_loc);
	if (length > max_length_dbu) {
	  repairLongWire(drvr, load, max_length_dbu, buffer_cell);
	  repair_count++;
	}
	else
	  // Drivers are sorted so we are done.
	  break;
      }
    }
  }
  printf("Repaired %d long wires.\n", repair_count);
  printf("Inserted %d buffers.\n", inserted_buffer_count_);
}

void
Resizer::repairLongWire(Vertex *drvr,
			Vertex *load,
			int max_length_dbu,
			LibertyCell *buffer_cell)
{
  Pin *drvr_pin = drvr->pin();
  Pin *load_pin = load->pin();
  Point drvr_loc = pinLocation(drvr_pin, db_network_);
  Point load_loc = pinLocation(load_pin, db_network_);
  int64_t length = Point::manhattanDistance(drvr_loc, load_loc);
  int buffer_count = length / max_length_dbu;
  if (buffer_count > 0) {
    LibertyPort *buffer_input_port, *buffer_output_port;
    buffer_cell->bufferPorts(buffer_input_port, buffer_output_port);

    debugPrint3(debug_, "repair_wire", 1, "%s -> %s %s\n",
		sdc_network_->pathName(drvr_pin),
		sdc_network_->pathName(load_pin),
		units_->distanceUnit()->asString(dbuToMeters(length), 0));

    Net *net = network_->net(drvr_pin);
    Instance *parent = db_network_->topInstance();
    int spacing = length / (buffer_count + 1);
    int drvr_x = drvr_loc.getX();
    int drvr_y = drvr_loc.getY();
    double dx = load_loc.getX() - drvr_x;
    double dy = load_loc.getY() - drvr_y;
    int space_x = spacing * dx / length;
    int space_y = spacing * dy / length;
    Net *prev_net = net;
    for (int i = 0; i < buffer_count; i++) {
      string buffer_name = makeUniqueInstName("repeater");
      string buffer_out_name = makeUniqueNetName();
      Net *buffer_out = db_network_->makeNet(buffer_out_name.c_str(), parent);
      Instance *buffer = db_network_->makeInstance(buffer_cell,
						   buffer_name.c_str(),
						   parent);
      Point buffer_loc(drvr_x + (i + 1) * space_x,
		       drvr_y + (i + 1) * space_y);
      setLocation(buffer, buffer_loc);
      design_area_ += area(db_network_->cell(buffer_cell));
      inserted_buffer_count_++;

      sta_->connectPin(buffer, buffer_input_port, prev_net);
      sta_->connectPin(buffer, buffer_output_port, buffer_out);

      Instance *load_inst = db_network_->instance(load_pin);
      Port *load_port = db_network_->port(load_pin);
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load_inst, load_port, buffer_out);

      debugPrint3(debug_, "repair_wire", 2, " %s (%s %s)\n",
		  buffer_name.c_str(),
		  units_->distanceUnit()->asString(dbuToMeters(buffer_loc.getX()), 0),
		  units_->distanceUnit()->asString(dbuToMeters(buffer_loc.getY()), 0));
      if (have_estimated_parasitics_)
	estimateWireParasitic(prev_net);
      prev_net = buffer_out;
    }
    if (have_estimated_parasitics_) {
      estimateWireParasitic(prev_net);
      estimateWireParasitic(net);
    }
  }
}

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
    float wire_length = maxLoadManhattenDistance(drvr);
    float steiner_length = dbuToMeters(findMaxSteinerDist(drvr));
    float delay = wire_length * wire_res_ * wire_length * wire_cap_ * 0.5;
    report_->print("%s manhtn %s steiner %s %s\n",
		   sdc_network_->pathName(drvr_pin),
		   units_->distanceUnit()->asString(wire_length, 0),
		   units_->distanceUnit()->asString(steiner_length, 0),
		   units_->timeUnit()->asString(delay, digits));
  }
}

void
Resizer::findLongWires(VertexSeq &drvrs)
{
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
	drvrs.push_back(vertex);
    }
  }
  sort(drvrs, [this](Vertex *drvr1,
		     Vertex *drvr2) {
		return maxLoadManhattenDistance(drvr1)
		  > maxLoadManhattenDistance(drvr2);
	      });
}

typedef std::pair<Vertex*, int> DrvrDist;

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
// the driver to loads in dbu.
int
Resizer::findMaxSteinerDist(Vertex *drvr)
{
  Pin *pin = drvr->pin();
  Net *net = network_->net(pin);
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
  if (tree) {
    SteinerPt drvr_pt = tree->drvrPt(db_network_);
    return findMaxSteinerDist(tree, drvr_pt, 0);
  }
  return 0;
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
  double max_dist = -INF;
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isDriver(pin)) {
      Vertex *drvr = graph_->pinDrvrVertex(pin);
      if (drvr) {
	double dist = maxLoadManhattenDistance(drvr);
	max_dist = max(max_dist, dist);
      }
    }
  }
  return max_dist;
}

double
Resizer::maxLoadManhattenDistance(Vertex *drvr)
{
  int64_t max_dist = 0;
  Point drvr_loc = pinLocation(drvr->pin(), db_network_);
  VertexOutEdgeIterator edge_iter(drvr, graph_);
  while (edge_iter.hasNext()) {
    Edge *edge = edge_iter.next();
    Vertex *load = edge->to(graph_);
    Point load_loc = pinLocation(load->pin(), db_network_);
    int64_t dist = Point::manhattanDistance(drvr_loc, load_loc);
    if (dist > max_dist)
      max_dist = dist;
  }
  return dbuToMeters(max_dist);
}

////////////////////////////////////////////////////////////////

// For testing.
void
Resizer::rebuffer(Net *net,
		  LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  rebuffer_net_count_ = 0;
  PinSet *drvrs = network_->drivers(net);
  PinSet::Iterator drvr_iter(drvrs);
  if (drvr_iter.hasNext()) {
    Pin *drvr = drvr_iter.next();
    rebuffer(drvr, buffer_cell);
  }
  printf("Inserted %d buffers.\n", inserted_buffer_count_);
}

// not used.
void
Resizer::rebuffer(const Pin *drvr_pin,
		  LibertyCell *buffer_cell)
{
  Net *net;
  LibertyPort *drvr_port;
  if (network_->isTopLevelPort(drvr_pin)) {
    net = network_->net(network_->term(drvr_pin));
    // Should use sdc external driver here.
    LibertyPort *input;
    buffer_cell->bufferPorts(input, drvr_port);
  }
  else {
    net = network_->net(drvr_pin);
    drvr_port = network_->libertyPort(drvr_pin);
  }
  if (drvr_port
      && net
      // Verilog connects by net name, so there is no way to distinguish the
      // net from the port.
      && !hasTopLevelOutputPort(net)) {
    SteinerTree *tree = makeSteinerTree(net, true, db_network_);
    if (tree) {
      SteinerPt drvr_pt = tree->drvrPt(network_);
      debugPrint1(debug_, "rebuffer", 2, "driver %s\n",
		  sdc_network_->pathName(drvr_pin));
      RebufferOptionSeq Z = rebufferBottomUp(tree, tree->left(drvr_pt),
					     drvr_pt,
					     1, buffer_cell);
      Required best_slack = -INF;
      RebufferOption *best_option = nullptr;
      int best_index = 0;
      int i = 1;
      for (RebufferOption *p : Z) {
	// Find required for drvr_pin into option.
	ArcDelay gate_delays[RiseFall::index_count];
	gateDelays(drvr_port, p->cap(), gate_delays);
	Slacks slacks{p->required(RiseFall::rise()) - gate_delays[RiseFall::riseIndex()],
		      p->required(RiseFall::fall()) - gate_delays[RiseFall::fallIndex()]};
	RiseFall *rf = (slacks[RiseFall::riseIndex()] < slacks[RiseFall::fallIndex()])
	  ? RiseFall::rise()
	  : RiseFall::fall();
	int rf_index = rf->index();
	debugPrint7(debug_, "rebuffer", 3,
		    "option %d: %d buffers req %s %s - %ss = %ss cap %s\n",
		    i,
		    p->bufferCount(),
		    rf->asString(),
		    delayAsString(p->required(rf), this),
		    delayAsString(gate_delays[rf_index], this),
		    delayAsString(slacks[rf_index], this),
		    units_->capacitanceUnit()->asString(p->cap()));
	if (debug_->check("rebuffer", 4))
	  p->printTree(this);
	if (fuzzyGreater(slacks[rf_index], best_slack)) {
	  best_slack = slacks[rf_index];
	  best_option = p;
	  best_index = i;
	}
	i++;
      }
      if (best_option) {
	debugPrint1(debug_, "rebuffer", 3, "best option %d\n", best_index);
	int before = inserted_buffer_count_;
	rebufferTopDown(best_option, net, 1, buffer_cell);
	if (inserted_buffer_count_ != before)
	  rebuffer_net_count_++;
      }
      deleteRebufferOptions();
    }
    delete tree;
  }
}

bool
Resizer::hasTopLevelOutputPort(Net *net)
{
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isTopLevelPort(pin)
	&& network_->direction(pin)->isOutput()) {
      delete pin_iter;
      return true;
    }
  }
  delete pin_iter;
  return false;
}

// The routing tree is represented a binary tree with the sinks being the leaves
// of the tree, the junctions being the Steiner nodes and the root being the
// source of the net.
RebufferOptionSeq
Resizer::rebufferBottomUp(SteinerTree *tree,
			  SteinerPt k,
			  SteinerPt prev,
			  int level,
			  LibertyCell *buffer_cell)
{
  if (k != SteinerTree::null_pt) {
    Pin *pin = tree->pin(k);
    if (pin && network_->isLoad(pin)) {
      // Load capacitance and required time.
      RebufferOption *z = makeRebufferOption(RebufferOptionType::sink,
					     pinCapacitance(pin),
					     pinRequireds(pin),
					     pin,
					     tree->location(k),
					     nullptr, nullptr);
      if (debug_->check("rebuffer", 4))
	z->print(level, this);
      RebufferOptionSeq Z;
      Z.push_back(z);
      return addWireAndBuffer(Z, tree, k, prev, level, buffer_cell);
    }
    else if (pin == nullptr) {
      // Steiner pt.
      RebufferOptionSeq Zl = rebufferBottomUp(tree, tree->left(k), k,
					      level + 1, buffer_cell);
      RebufferOptionSeq Zr = rebufferBottomUp(tree, tree->right(k), k,
					      level + 1, buffer_cell);
      RebufferOptionSeq Z;
      // Combine the options from both branches.
      for (RebufferOption *p : Zl) {
	for (RebufferOption *q : Zr) {
	  Requireds junc_reqs{min(p->required(RiseFall::rise()),
				  q->required(RiseFall::rise())),
			      min(p->required(RiseFall::fall()),
				  q->required(RiseFall::fall()))};
	  RebufferOption *junc = makeRebufferOption(RebufferOptionType::junction,
						    p->cap() + q->cap(),
						    junc_reqs,
						    nullptr,
						    tree->location(k),
						    p, q);
	  Z.push_back(junc);
	}
      }
      // Prune the options. This is fanout^2.
      // Presort options to hit better options sooner.
      sort(Z, [](RebufferOption *option1,
		 RebufferOption *option2)
	      {   return fuzzyGreater(option1->requiredMin(),
				      option2->requiredMin());
	      });
      int si = 0;
      for (size_t pi = 0; pi < Z.size(); pi++) {
	RebufferOption *p = Z[pi];
	float Lp = p->cap();
	// Remove options by shifting down with index si.
	si = pi + 1;
	// Because the options are sorted we don't have to look
	// beyond the first option.
	for (size_t qi = pi + 1; qi < Z.size(); qi++) {
	  RebufferOption *q = Z[qi];
	  float Lq = q->cap();
	  // We know Tq <= Tp from the sort so we don't need to check req.
	  // If q is the same or worse than p, remove solution q.
	  if (fuzzyLess(Lq, Lp))
	    // Copy survivor down.
	    Z[si++] = q;
	}
	Z.resize(si);
      }
      return addWireAndBuffer(Z, tree, k, prev, level, buffer_cell);
    }
  }
  return RebufferOptionSeq();
}

RebufferOptionSeq
Resizer::addWireAndBuffer(RebufferOptionSeq Z,
			  SteinerTree *tree,
			  SteinerPt k,
			  SteinerPt prev,
			  int level,
			  LibertyCell *buffer_cell)
{
  RebufferOptionSeq Z1;
  Required best_req = -INF;
  RebufferOption *best_option = nullptr;
  Point k_loc = tree->location(k);
  Point prev_loc = tree->location(prev);
  int wire_length_dbu = abs(k_loc.x() - prev_loc.x())
    + abs(k_loc.y() - prev_loc.y());
  float wire_length = dbuToMeters(wire_length_dbu);
  float wire_cap = wire_length * wire_cap_;
  float wire_res = wire_length * wire_res_;
  float wire_delay = wire_res * wire_cap;
  for (RebufferOption *p : Z) {
    // account for wire delay
    Requireds reqs{p->required(RiseFall::rise()) - wire_delay,
		   p->required(RiseFall::fall()) - wire_delay};
    RebufferOption *z = makeRebufferOption(RebufferOptionType::wire,
					   // account for wire load
					   p->cap() + wire_cap,
					   reqs,
					   nullptr,
					   prev_loc,
					   p, nullptr);
    if (debug_->check("rebuffer", 3)) {
      printf("%*swire %s -> %s wl %d\n",
	     level, "",
	     tree->name(prev, sdc_network_),
	     tree->name(k, sdc_network_),
	     wire_length_dbu);
      z->print(level, this);
    }
    Z1.push_back(z);
    // We could add options of different buffer drive strengths here
    // Which would have different delay Dbuf and input cap Lbuf
    // for simplicity we only consider one size of buffer.
    Required req = z->bufferRequired(buffer_cell, this);
    if (fuzzyGreater(req, best_req)) {
      best_req = req;
      best_option = z;
    }
  }
  if (best_option) {
    Requireds requireds = best_option->bufferRequireds(buffer_cell, this);
    RebufferOption *z = makeRebufferOption(RebufferOptionType::buffer,
					   bufferInputCapacitance(buffer_cell),
					   requireds,
					   nullptr,
					   // Locate buffer at opposite end of wire.
					   prev_loc,
					   best_option, nullptr);
    if (debug_->check("rebuffer", 3)) {
      printf("%*sbuffer %s cap %s req %s ->\n",
	     level, "",
	     tree->name(prev, sdc_network_),
	     units_->capacitanceUnit()->asString(best_option->cap()),
	     delayAsString(best_req, this));
      z->print(level, this);
    }
    Z1.push_back(z);
  }
  return Z1;
}

// Return inserted buffer count.
void
Resizer::rebufferTopDown(RebufferOption *choice,
			 Net *net,
			 int level,
			 LibertyCell *buffer_cell)
{
  switch(choice->type()) {
  case RebufferOptionType::buffer: {
    Instance *parent = db_network_->topInstance();
    string net2_name = makeUniqueNetName();
    string buffer_name = makeUniqueInstName("rebuffer");
    Net *net2 = db_network_->makeNet(net2_name.c_str(), parent);
    Instance *buffer = db_network_->makeInstance(buffer_cell,
						 buffer_name.c_str(),
						 parent);
    inserted_buffer_count_++;
    design_area_ += area(db_network_->cell(buffer_cell));
    level_drvr_verticies_valid_ = false;
    LibertyPort *input, *output;
    buffer_cell->bufferPorts(input, output);
    debugPrint5(debug_, "rebuffer", 3, "%*sinsert %s -> %s -> %s\n",
		level, "",
		sdc_network_->pathName(net),
		buffer_name.c_str(),
		net2_name.c_str());
    sta_->connectPin(buffer, input, net);
    sta_->connectPin(buffer, output, net2);
    setLocation(buffer, choice->location());
    rebufferTopDown(choice->ref(), net2, level + 1, buffer_cell);
    if (have_estimated_parasitics_) {
      estimateWireParasitic(net);
      estimateWireParasitic(net2);
    }
    break;
  }
  case RebufferOptionType::wire:
    debugPrint2(debug_, "rebuffer", 3, "%*swire\n", level, "");
    rebufferTopDown(choice->ref(), net, level + 1, buffer_cell);
    break;
  case RebufferOptionType::junction: {
    debugPrint2(debug_, "rebuffer", 3, "%*sjunction\n", level, "");
    rebufferTopDown(choice->ref(), net, level + 1, buffer_cell);
    rebufferTopDown(choice->ref2(), net, level + 1, buffer_cell);
    break;
  }
  case RebufferOptionType::sink: {
    Pin *load_pin = choice->loadPin();
    Net *load_net = network_->net(load_pin);
    if (load_net != net) {
      Instance *load_inst = db_network_->instance(load_pin);
      Port *load_port = db_network_->port(load_pin);
      debugPrint4(debug_, "rebuffer", 3, "%*sconnect load %s to %s\n",
		  level, "",
		  sdc_network_->pathName(load_pin),
		  sdc_network_->pathName(net));
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load_inst, load_port, net);
    }
    break;
  }
  }
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
  gateDelays(output, load_cap, gate_delays);
  return gate_delays[rf->index()];
}

// Rise/fall delays across all timing arcs into drvr_port.
// Uses target slew for input slew.
void
Resizer::gateDelays(LibertyPort *drvr_port,
		    float load_cap,
		    // Return values.
		    ArcDelay delays[RiseFall::index_count])
{
  delays[RiseFall::riseIndex()] = -INF;
  delays[RiseFall::fallIndex()] = -INF;
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
      }
    }
  }
}

////////////////////////////////////////////////////////////////

// Find the max wire length before it is faster to split the wire
// in half with a buffer.
float
Resizer::findMaxWireLength(LibertyCell *buffer_cell)
{
  LibertyPort *load_port, *drvr_port;
  buffer_cell->bufferPorts(load_port, drvr_port);
  float drvr_r = max(drvr_port->driveResistance(RiseFall::rise(), MinMax::max()),
		     drvr_port->driveResistance(RiseFall::fall(), MinMax::max()));
  // wire_length1 lower bound
  // wire_length2 upper bound
  float wire_length1 = 0.0;
  // Initial guess with wire resistance same as driver resistance.
  float wire_length2 = drvr_r / wire_res_;
  float tol = .01; // 1%
  float diff1 = splitWireDelayDiff(wire_length1, buffer_cell);
  float diff2 = splitWireDelayDiff(wire_length2, buffer_cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff2 < 0.0) {
      wire_length1 = wire_length2;
      diff1 = diff2;
      wire_length2 *= 2;
      diff2 = splitWireDelayDiff(wire_length2, buffer_cell);
    }
    else {
      float wire_length3 = (wire_length1 + wire_length2) / 2.0;
      float diff3 = splitWireDelayDiff(wire_length3, buffer_cell);
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
float
Resizer::splitWireDelayDiff(float wire_length,
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
			 float wire_length, // meters
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
	float in_slew = tgt_slews_[in_rf->index()];
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
			   float wire_length) // meters
{
  Parasitic *parasitic = parasitics_->makeParasiticNetwork(net, false,
							   parasitics_ap_);
  ParasiticNode *n1 = parasitics_->ensureParasiticNode(parasitic, drvr_pin);
  ParasiticNode *n2 = parasitics_->ensureParasiticNode(parasitic, load_pin);
  float wire_cap = wire_length * wire_cap_;
  float wire_res = wire_length * wire_res_;
  parasitics_->incrCap(n1, wire_cap / 2.0, parasitics_ap_);
  parasitics_->makeResistor(nullptr, n1, n2, wire_res, parasitics_ap_);
  parasitics_->incrCap(n2, wire_cap / 2.0, parasitics_ap_);
  return parasitic;
}

////////////////////////////////////////////////////////////////

float
Resizer::findMaxSlewWireLength(float max_slew,
			       LibertyCell *buffer_cell)
{
  // wire_length1 lower bound
  // wire_length2 upper bound
  float wire_length1 = 0.0;
  float wire_length2 = std::sqrt(max_slew / (wire_res_ * wire_cap_));
  float tol = .01; // 1%
  float diff1 = maxSlewWireDiff(wire_length1, max_slew, buffer_cell);
  float diff2 = maxSlewWireDiff(wire_length2, max_slew, buffer_cell);
  // binary search for diff = 0.
  while (abs(wire_length1 - wire_length2) > max(wire_length1, wire_length2) * tol) {
    if (diff2 < 0.0) {
      wire_length1 = wire_length2;
      diff1 = diff2;
      wire_length2 *= 2;
      diff2 = maxSlewWireDiff(wire_length2, max_slew, buffer_cell);
    }
    else {
      float wire_length3 = (wire_length1 + wire_length2) / 2.0;
      float diff3 = maxSlewWireDiff(wire_length3, max_slew, buffer_cell);
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
float
Resizer::maxSlewWireDiff(float wire_length,
			 float max_slew,
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

Point
pinLocation(const Pin *pin,
	    const dbNetwork *network)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  network->staToDb(pin, iterm, bterm);
  if (iterm) {
    int x, y;
    if (iterm->getAvgXY(&x, &y))
      return Point(x, y);
    else {
      dbInst *inst = iterm->getInst();
      int x, y;
      inst->getOrigin(x, y);
      return Point(x, y);
    }
  }
  if (bterm) {
    int x, y;
    if (bterm->getFirstPinLocation(x, y))
      return Point(x, y);
  }
  return Point(0, 0);
}

bool
pinIsPlaced(Pin *pin,
	    const dbNetwork *network)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  network->staToDb(pin, iterm, bterm);
  dbPlacementStatus status = dbPlacementStatus::UNPLACED;
  if (iterm) {
    dbInst *inst = iterm->getInst();
    status = inst->getPlacementStatus();
  }
  if (bterm)
    status = bterm->getFirstPinPlacementStatus();
  return status.isPlaced();
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

void
Resizer::findLoads(Pin *drvr_pin,
		   PinSeq &loads)
{
  PinSeq drvrs;
  PinSet visited_drvrs;
  FindNetDrvrLoads visitor(drvr_pin, visited_drvrs, loads, drvrs, network_);
  network_->visitConnectedPins(drvr_pin, visitor);
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

}
