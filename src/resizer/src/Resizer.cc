// Resizer, LEF/DEF gate resizer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "Machine.hh"
#include "Report.hh"
#include "Debug.hh"
#include "PortDirection.hh"
#include "TimingRole.hh"
#include "Units.hh"
#include "Liberty.hh"
#include "TimingArc.hh"
#include "TimingModel.hh"
#include "Corner.hh"
#include "DcalcAnalysisPt.hh"
#include "Graph.hh"
#include "ArcDelayCalc.hh"
#include "GraphDelayCalc.hh"
#include "Parasitics.hh"
#include "Sdc.hh"
#include "PathVertex.hh"
#include "SearchPred.hh"
#include "Bfs.hh"
#include "Search.hh"
#include "Network.hh"
#include "StaMain.hh"
#include "resizer/SteinerTree.hh"
#include "resizer/Resizer.hh"

// Outstanding issues
//  Instance levelization and resizing to target slew only support single output gates
//  multi-corner support?
//  tcl cmds to set liberty pin cap and limit for testing
//  check one def
//  check lef/liberty library cell ports match
//  option to place buffers between driver and load on long wires
//   to fix max slew/cap violations
// http://vlsicad.eecs.umich.edu/BK/Slots/cache/dropzone.tamu.edu/~zhuoli/GSRC/fast_buffer_insertion.html

namespace sta {

using std::abs;
using std::min;
using std::max;
using std::string;

using odb::dbInst;
using odb::dbPlacementStatus;
using odb::adsRect;

extern "C" {
extern int Resizer_Init(Tcl_Interp *interp);
}

extern const char *resizer_tcl_inits[];

bool
pinIsPlaced(Pin *pin,
	    const dbNetwork *network);
adsPoint
pinLocation(Pin *pin,
	    const dbNetwork *network);

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
  target_load_map_(nullptr),
  level_drvr_verticies_valid_(false),
  tgt_slews_{0.0, 0.0},
  unique_net_index_(1),
  unique_buffer_index_(1),
  resize_count_(0),
  core_area_(0.0),
  design_area_(0.0)
{
}

void
Resizer::init(Tcl_Interp *interp,
	      dbDatabase *db,
	      dbSta *sta)
{
  db_ = db;
  sta_ = sta;
  db_network_ = sta->getDbNetwork();
  copyState(sta);
  // Define swig TCL commands.
  Resizer_Init(interp);
  // Eval encoded sta TCL sources.
  evalTclInit(interp, resizer_tcl_inits);
  // Import exported commands from sta namespace to global namespace.
  Tcl_Eval(interp, "sta::define_sta_cmds");
  Tcl_Eval(interp, "namespace import sta::*");
}

////////////////////////////////////////////////////////////////

double
Resizer::coreArea() const
{
  adsRect rect;
  db_->getChip()->getBlock()->getDieArea(rect);
  return dbuToMeters(rect.dx()) * dbuToMeters(rect.dy());
}

double
Resizer::utilization()
{
  double core_area = coreArea();
  if (core_area > 0.0)
    return designArea() / core_area;
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

void
Resizer::init()
{
  // Init design_area_.
  designArea();
  db_network_ = sta_->getDbNetwork();
  sta_->ensureLevelized();
  // In case graph was made by ensureLevelized.
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
  // Abbreviated copyState
  graph_delay_calc_ = sta_->graphDelayCalc();
  search_ = sta_->search();

  // Disable incremental timing.
  graph_delay_calc_->delaysInvalid();
  search_->arrivalsInvalid();

  wire_res_ = wire_res;
  wire_cap_ = wire_cap;

  initCorner(corner);
  init();
  makeNetParasitics();
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
  inserted_buffer_count_ = 0;
  InstancePinIterator *port_iter(network_->pinIterator(network_->topInstance()));
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    Net *net = network_->net(network_->term(pin));
    if (network_->direction(pin)->isInput()
	&& !isClock(net))
      bufferInput(pin, buffer_cell);
  }
  delete port_iter;
  report_->print("Inserted %d input buffers.\n",
		 inserted_buffer_count_);
}
   
void
Resizer::bufferInput(Pin *top_pin,
		     LibertyCell *buffer_cell)
{
  Term *term = db_network_->term(top_pin);
  Net *input_net = db_network_->net(term);
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  string buffer_out_net_name = makeUniqueNetName();
  string buffer_name = makeUniqueBufferName();
  Instance *parent = db_network_->topInstance();
  Net *buffer_out = db_network_->makeNet(buffer_out_net_name.c_str(), parent);
  Instance *buffer = db_network_->makeInstance(buffer_cell,
					       buffer_name.c_str(),
					       parent);
  if (buffer) {
    setLocation(buffer, pinLocation(top_pin, db_network_));
    inserted_buffer_count_++;

    NetPinIterator *pin_iter(db_network_->pinIterator(input_net));
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      Port *pin_port = db_network_->port(pin);
      sta_->disconnectPin(pin);
      sta_->connectPin(db_network_->instance(pin), pin_port, buffer_out);
    }
    sta_->connectPin(buffer, input, input_net);
    sta_->connectPin(buffer, output, buffer_out);
  }
}

void
Resizer::setLocation(Instance *inst,
		     adsPoint pt)
{
  dbInst *dinst = db_network_->staToDb(inst);
  dinst->setPlacementStatus(dbPlacementStatus::PLACED);
  dinst->setLocation(pt.getX(), pt.getY());
}

void
Resizer::bufferOutputs(LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  InstancePinIterator *port_iter(network_->pinIterator(network_->topInstance()));
  while (port_iter->hasNext()) {
    Pin *pin = port_iter->next();
    if (network_->direction(pin)->isOutput())
      bufferOutput(pin, buffer_cell);
  }
  delete port_iter;
  report_->print("Inserted %d output buffers.\n",
		 inserted_buffer_count_);
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
  string buffer_name = makeUniqueBufferName();
  Instance *parent = network->topInstance();
  Net *buffer_in = network->makeNet(buffer_in_net_name.c_str(), parent);
  Instance *buffer = network->makeInstance(buffer_cell,
					   buffer_name.c_str(),
					   parent);
  if (buffer) {
    setLocation(buffer, pinLocation(top_pin, db_network_));
    inserted_buffer_count_++;

    NetPinIterator *pin_iter(network->pinIterator(output_net));
    while (pin_iter->hasNext()) {
      Pin *pin = pin_iter->next();
      Port *pin_port = network->port(pin);
      sta_->disconnectPin(pin);
      sta_->connectPin(network->instance(pin), pin_port, buffer_in);
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
      report_->warn("max utilization reached.\n");
      break;
    }
  }
  report_->print("Resized %d instances.\n", resize_count_);
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
      // Hands off the clock nets.
      if (!isClock(out_net)) {
	// Includes net parasitic capacitance.
	float load_cap = graph_delay_calc_->loadCap(output, dcalc_ap_);
	LibertyCell *best_cell = nullptr;
	float best_ratio = 0.0;
	auto equiv_cells = sta_->equivCells(cell);
	if (equiv_cells) {
	  for (auto target_cell : *equiv_cells) {
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
  return dbuToMeters(master->getWidth()) * dbuToMeters(master->getHeight());
}

// DBUs are nanometers.
double
Resizer::dbuToMeters(uint dist) const
{
  return dist * 1E-9;
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
    for (auto cell : *dont_use)
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
  for (auto lib : *resize_libs)
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
			 Slew slews[])
{
  LibertyCellIterator cell_iter(library);
  while (cell_iter.hasNext()) {
    auto cell = cell_iter.next();
    findTargetLoad(cell, slews);
  }
}

void
Resizer::findTargetLoad(LibertyCell *cell,
			Slew slews[])
{
  LibertyCellTimingArcSetIterator arc_set_iter(cell);
  float target_load_sum = 0.0;
  int arc_count = 0;
  while (arc_set_iter.hasNext()) {
    auto arc_set = arc_set_iter.next();
    auto role = arc_set->role();
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
  
  for (auto lib : *resize_libs) {
    Slew slews[RiseFall::index_count]{0.0};
    int counts[RiseFall::index_count]{0};
    
    findBufferTargetSlews(lib, slews, counts);
    for (auto rf : RiseFall::rangeIndex()) {
      tgt_slews_[rf] += slews[rf];
      tgt_counts[rf] += counts[rf];
      slews[rf] /= counts[rf];
    }
    debugPrint3(debug_, "resizer", 2, "target_slews %s = %.2e/%.2e\n",
		lib->name(),
		slews[RiseFall::riseIndex()],
		slews[RiseFall::fallIndex()]);
  }

  for (auto rf : RiseFall::rangeIndex())
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
  for (auto buffer : *library->buffers()) {
    if (!dontUse(buffer)) {
      LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      auto arc_sets = buffer->timingArcSets(input, output);
      if (arc_sets) {
	for (auto arc_set : *arc_sets) {
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
  for (auto pin : clk_pins) {
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
Resizer::makeNetParasitics()
{
  NetIterator *net_iter = network_->netIterator(network_->topInstance());
  while (net_iter->hasNext()) {
    Net *net = net_iter->next();
    // Hands off the clock nets.
    if (!isClock(net))
      makeNetParasitics(net);
  }
  delete net_iter;
}

void
Resizer::makeNetParasitics(const Net *net)
{
  SteinerTree *tree = makeSteinerTree(net, false, db_network_);
  if (tree && tree->isPlaced(db_network_)) {
    debugPrint1(debug_, "resizer_parasitics", 1, "net %s\n",
		sdc_network_->pathName(net));
    Parasitic *parasitic = parasitics_->makeParasiticNetwork(net, false,
							     parasitics_ap_);
    int branch_count = tree->branchCount();
    for (int i = 0; i < branch_count; i++) {
      adsPoint pt1, pt2;
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
    delete tree;
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

////////////////////////////////////////////////////////////////

void
Resizer::rebufferNets(bool repair_max_cap,
		      bool repair_max_slew,
		      LibertyCell *buffer_cell)
{
  if (repair_max_cap || repair_max_slew) {
    rebuffer(repair_max_cap, repair_max_slew, buffer_cell);
    report_->print("Inserted %d buffers in %d nets.\n",
		   inserted_buffer_count_,
		   rebuffer_net_count_);
  }
}

class RebufferOption
{
public:
  enum Type { sink, junction, wire, buffer };

  RebufferOption(Type type,
		 float cap,
		 Required required,
		 Pin *load_pin,
		 adsPoint location,
		 RebufferOption *ref,
		 RebufferOption *ref2);
  ~RebufferOption();
  Type type() const { return type_; }
  float cap() const { return cap_; }
  Required required() const { return required_; }
  Required bufferRequired(LibertyCell *buffer_cell,
			  Resizer *resizer) const;
  adsPoint location() const { return location_; }
  Pin *loadPin() const { return load_pin_; }
  RebufferOption *ref() const { return ref_; }
  RebufferOption *ref2() const { return ref2_; }

private:
  Type type_;
  float cap_;
  Required required_;
  Pin *load_pin_;
  adsPoint location_;
  RebufferOption *ref_;
  RebufferOption *ref2_;
};

RebufferOption::RebufferOption(Type type,
			       float cap,
			       Required required,
			       Pin *load_pin,
			       adsPoint location,
			       RebufferOption *ref,
			       RebufferOption *ref2) :
  type_(type),
  cap_(cap),
  required_(required),
  load_pin_(load_pin),
  location_(location),
  ref_(ref),
  ref2_(ref2)
{
}

RebufferOption::~RebufferOption()
{
}

Required
RebufferOption::bufferRequired(LibertyCell *buffer_cell,
			       Resizer *resizer) const
{
  return required_ - resizer->bufferDelay(buffer_cell, cap_);
}

////////////////////////////////////////////////////////////////

void
Resizer::rebuffer(bool repair_max_cap,
		  bool repair_max_slew,
		  LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  rebuffer_net_count_ = 0;
  sta_->findDelays();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    // Hands off the clock tree.
    if (!search_->isClock(vertex)) {
      Pin *drvr_pin = vertex->pin();
      if ((repair_max_cap
	   && hasMaxCapViolation(drvr_pin))
	  || (repair_max_slew
	      && hasMaxSlewViolation(drvr_pin))) {
	rebuffer(drvr_pin, buffer_cell);
	if (overMaxArea()) {
	  report_->warn("max utilization reached.\n");
	  break;
	}
      }
    }
  }
}

bool
Resizer::hasMaxCapViolation(const Pin *drvr_pin)
{
  LibertyPort *port = network_->libertyPort(drvr_pin);
  if (port) {
    float load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap_);
    float cap_limit;
    bool exists;
    port->capacitanceLimit(MinMax::max(), cap_limit, exists);
    return exists && load_cap > cap_limit;
  }
  return false;
}

bool
Resizer::hasMaxSlewViolation(const Pin *drvr_pin)
{
  Vertex *vertex = graph_->pinDrvrVertex(drvr_pin);
  float limit;
  bool exists;
  slewLimit(drvr_pin, MinMax::max(), limit, exists);
  for (auto rf : RiseFall::range()) {
    Slew slew = graph_->slew(vertex, rf, dcalc_ap_->index());
    if (slew > limit)
      return true;
  }
  return false;
}

void
Resizer::slewLimit(const Pin *pin,
		   const MinMax *min_max,
		   // Return values.
		   float &limit,
		   bool &exists) const
			
{
  exists = false;
  Cell *top_cell = network_->cell(network_->topInstance());
  float top_limit;
  bool top_limit_exists;
  sdc_->slewLimit(top_cell, min_max,
		  top_limit, top_limit_exists);

  // Default to top ("design") limit.
  exists = top_limit_exists;
  limit = top_limit;
  if (network_->isTopLevelPort(pin)) {
    Port *port = network_->port(pin);
    float port_limit;
    bool port_limit_exists;
    sdc_->slewLimit(port, min_max, port_limit, port_limit_exists);
    // Use the tightest limit.
    if (port_limit_exists
	&& (!exists
	    || min_max->compare(limit, port_limit))) {
      limit = port_limit;
      exists = true;
    }
  }
  else {
    float pin_limit;
    bool pin_limit_exists;
    sdc_->slewLimit(pin, min_max,
		    pin_limit, pin_limit_exists);
    // Use the tightest limit.
    if (pin_limit_exists
	&& (!exists
	    || min_max->compare(limit, pin_limit))) {
      limit = pin_limit;
      exists = true;
    }

    float port_limit;
    bool port_limit_exists;
    LibertyPort *port = network_->libertyPort(pin);
    if (port) {
      port->slewLimit(min_max, port_limit, port_limit_exists);
      // Use the tightest limit.
      if (port_limit_exists
	  && (!exists
	      || min_max->compare(limit, port_limit))) {
	limit = port_limit;
	exists = true;
      }
    }
  }
}

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
  report_->print("Inserted %d buffers.\n", inserted_buffer_count_);
}

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
      // Verilog connects by net name, so there is no way to distinguish the
      // net from the port.
      && !hasTopLevelOutputPort(net)) {
    SteinerTree *tree = makeSteinerTree(net, true, db_network_);
    if (tree) {
      SteinerPt drvr_pt = tree->drvrPt(network_);
      Required drvr_req = pinRequired(drvr_pin);
      // Make sure the driver is constrained.
      if (!fuzzyInf(drvr_req)) {
	debugPrint1(debug_, "rebuffer", 2, "driver %s\n",
		    sdc_network_->pathName(drvr_pin));
	RebufferOptionSeq Z = rebufferBottomUp(tree, tree->left(drvr_pt),
					       drvr_pt,
					       1, buffer_cell);
	Required Tbest = -INF;
	RebufferOption *best = nullptr;
	for (auto p : Z) {
	  Required Tb = p->required() - gateDelay(drvr_port, p->cap());
	  if (fuzzyGreater(Tb, Tbest)) {
	    Tbest = Tb;
	    best = p;
	  }
	}
	if (best) {
	  int before = inserted_buffer_count_;
	  rebufferTopDown(best, net, 1, buffer_cell);
	  if (inserted_buffer_count_ != before)
	    rebuffer_net_count_++;
	}
      }
    }
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

class RebufferOptionBufferReqGreater
{
public:
  RebufferOptionBufferReqGreater(LibertyCell *buffer_cell,
				 Resizer *resizer);
  bool operator()(RebufferOption *option1,
		  RebufferOption *option2);

protected:
  LibertyCell *buffer_cell_;
  Resizer *resizer_;
};

RebufferOptionBufferReqGreater::RebufferOptionBufferReqGreater(LibertyCell *buffer_cell,
							       Resizer *resizer) :
  buffer_cell_(buffer_cell),
  resizer_(resizer)
{
}
 
bool
RebufferOptionBufferReqGreater::operator()(RebufferOption *option1,
					   RebufferOption *option2)
{
  return fuzzyGreater(option1->bufferRequired(buffer_cell_, resizer_),
		      option2->bufferRequired(buffer_cell_, resizer_));
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
      RebufferOption *z = new RebufferOption(RebufferOption::Type::sink,
					     pinCapacitance(pin),
					     pinRequired(pin),
					     pin,
					     tree->location(k),
					     nullptr, nullptr);
      // %*s format indents level spaces.
      debugPrint5(debug_, "rebuffer", 3, "%*sload %s cap %s req %s\n",
		  level, "",
		  sdc_network_->pathName(pin),
		  units_->capacitanceUnit()->asString(z->cap()),
		  delayAsString(z->required(), this));
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
      for (auto p : Zl) {
	for (auto q : Zr) {
	  RebufferOption *junc = new RebufferOption(RebufferOption::Type::junction,
						    p->cap() + q->cap(),
						    min(p->required(),
							q->required()),
						    nullptr,
						    tree->location(k),
						    p, q);
	  Z.push_back(junc);
	}
      }
      // Prune the options. This is fanout^2.
      // Presort options to hit better options sooner.
      sort(Z, RebufferOptionBufferReqGreater(buffer_cell, this));
      int si = 0;
      for (size_t pi = 0; pi < Z.size(); pi++) {
	auto p = Z[pi];
	if (p) {
	  float Lp = p->cap();
	  // Remove options by shifting down with index si.
	  si = pi + 1;
	  // Because the options are sorted we don't have to look
	  // beyond the first option.
	  for (size_t qi = pi + 1; qi < Z.size(); qi++) {
	    auto q = Z[qi];
	    if (q) {
	      float Lq = q->cap();
	      // We know Tq <= Tp from the sort so we don't need to check req.
	      // If q is the same or worse than p, remove solution q.
	      if (fuzzyLess(Lq, Lp))
		// Copy survivor down.
		Z[si++] = q;
	    }
	  }
	  Z.resize(si);
	}
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
  Required best = -INF;
  RebufferOption *best_ref = nullptr;
  adsPoint k_loc = tree->location(k);
  adsPoint prev_loc = tree->location(prev);
  int wire_length_dbu = abs(k_loc.x() - prev_loc.x())
    + abs(k_loc.y() - prev_loc.y());
  float wire_length = dbuToMeters(wire_length_dbu);
  float wire_cap = wire_length * wire_cap_;
  float wire_res = wire_length * wire_res_;
  float wire_delay = wire_res * wire_cap;
  for (auto p : Z) {
    RebufferOption *z = new RebufferOption(RebufferOption::Type::wire,
					   // account for wire load
					   p->cap() + wire_cap,
					   // account for wire delay
					   p->required() - wire_delay,
					   nullptr,
					   prev_loc,
					   p, nullptr);
    debugPrint7(debug_, "rebuffer", 3, "%*swire %s -> %s wl %d cap %s req %s\n",
		level, "",
		tree->name(prev, sdc_network_),
		tree->name(k, sdc_network_),
		wire_length_dbu,
		units_->capacitanceUnit()->asString(z->cap()),
		delayAsString(z->required(), this));
    Z1.push_back(z);
    // We could add options of different buffer drive strengths here
    // Which would have different delay Dbuf and input cap Lbuf
    // for simplicity we only consider one size of buffer.
    Required rt = z->bufferRequired(buffer_cell, this);
    if (fuzzyGreater(rt, best)) {
      best = rt;
      best_ref = p;
    }
  }
  if (best_ref) {
    RebufferOption *z = new RebufferOption(RebufferOption::Type::buffer,
					   bufferInputCapacitance(buffer_cell),
					   best,
					   nullptr,
					   // Locate buffer at opposite end of wire.
					   prev_loc,
					   best_ref, nullptr);
    debugPrint7(debug_, "rebuffer", 3, "%*sbuffer %s cap %s req %s -> cap %s req %s\n",
		level, "",
		tree->name(prev, sdc_network_),
		units_->capacitanceUnit()->asString(best_ref->cap()),
		delayAsString(best_ref->required(), this),
		units_->capacitanceUnit()->asString(z->cap()),
		delayAsString(z->required(), this));
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
  case RebufferOption::Type::buffer: {
    Instance *parent = db_network_->topInstance();
    string net2_name = makeUniqueNetName();
    string buffer_name = makeUniqueBufferName();
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
    makeNetParasitics(net);
    makeNetParasitics(net2);
    break;
  }
  case RebufferOption::Type::wire:
    debugPrint2(debug_, "rebuffer", 3, "%*swire\n", level, "");
    rebufferTopDown(choice->ref(), net, level + 1, buffer_cell);
    break;
  case RebufferOption::Type::junction: {
    debugPrint2(debug_, "rebuffer", 3, "%*sjunction\n", level, "");
    rebufferTopDown(choice->ref(), net, level + 1, buffer_cell);
    rebufferTopDown(choice->ref2(), net, level + 1, buffer_cell);
    break;
  }
  case RebufferOption::Type::sink: {
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
Resizer::makeUniqueBufferName()
{
  string buffer_name;
  do 
    stringPrint(buffer_name, "buffer%d", unique_buffer_index_++);
  while (network_->findInstance(buffer_name.c_str()));
  return buffer_name;
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

Required
Resizer::pinRequired(const Pin *pin)
{
  Vertex *vertex = graph_->pinLoadVertex(pin);
  return sta_->vertexRequired(vertex, min_max_);
}

float
Resizer::bufferDelay(LibertyCell *buffer_cell,
		     float load_cap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  return gateDelay(output, load_cap);
}

float
Resizer::gateDelay(LibertyPort *out_port,
		   float load_cap)
{
  LibertyCell *cell = out_port->libertyCell();
  // Max rise/fall delays.
  ArcDelay max_delay = -INF;
  LibertyCellTimingArcSetIterator set_iter(cell);
  while (set_iter.hasNext()) {
    TimingArcSet *arc_set = set_iter.next();
    if (arc_set->to() == out_port) {
      TimingArcSetArcIterator arc_iter(arc_set);
      while (arc_iter.hasNext()) {
	TimingArc *arc = arc_iter.next();
	RiseFall *in_rf = arc->fromTrans()->asRiseFall();
	float in_slew = tgt_slews_[in_rf->index()];
	ArcDelay gate_delay;
	Slew drvr_slew;
	arc_delay_calc_->gateDelay(cell, arc, in_slew, load_cap,
				   nullptr, 0.0, pvt_, dcalc_ap_,
				   gate_delay,
				   drvr_slew);
	max_delay = max(max_delay, gate_delay);
      }
    }
  }
  return max_delay;
}

double
Resizer::designArea()
{
  if (design_area_ == 0.0) {
    for (dbInst *inst : db_->getChip()->getBlock()->getInsts()) {
      dbMaster *master = inst->getMaster();
      design_area_ += area(master);
    }
  }
  return design_area_;
}

adsPoint
pinLocation(Pin *pin,
	    const dbNetwork *network)
{
  dbITerm *iterm;
  dbBTerm *bterm;
  network->staToDb(pin, iterm, bterm);
  if (iterm) {
    dbInst *inst = iterm->getInst();
    int x, y;
    inst->getOrigin(x, y);
    return adsPoint(x, y);
  }
  if (bterm) {
    int x, y;
    if (bterm->getFirstPinLocation(x, y))
      return adsPoint(x, y);
  }
  return adsPoint(0, 0);
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
  return status == dbPlacementStatus::PLACED
    || status == dbPlacementStatus::LOCKED
    || status == dbPlacementStatus::FIRM
    || status == dbPlacementStatus::COVER;
}

}
