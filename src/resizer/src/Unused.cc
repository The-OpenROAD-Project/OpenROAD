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

// Resizer code on injured reserve.

// K means clustering algorithm.
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

////////////////////////////////////////////////////////////////

class RebufferOption;

typedef Vector<RebufferOption*> RebufferOptionSeq;
enum class RebufferOptionType { sink, junction, wire, buffer };

  // Rebuffer net (for testing).
  // Assumes buffer_cell->isBuffer() is true.
  // resizerPreamble() required.
  void rebuffer(Net *net,
		LibertyCell *buffer_cell);

  // Assumes buffer_cell->isBuffer() is true.
  void rebuffer(const Pin *drvr_pin,
		LibertyCell *buffer_cell);
  RebufferOptionSeq rebufferBottomUp(SteinerTree *tree,
				     SteinerPt k,
				     SteinerPt prev,
				     int level,
				     LibertyCell *buffer_cell);
  void rebufferTopDown(RebufferOption *choice,
		       Net *net,
		       int level,
		       LibertyCell *buffer_cell);
  RebufferOptionSeq
  addWireAndBuffer(RebufferOptionSeq Z,
		   SteinerTree *tree,
		   SteinerPt k,
		   SteinerPt prev,
		   int level,
		   LibertyCell *buffer_cell);
  // RebufferOption factory.
  RebufferOption *makeRebufferOption(RebufferOptionType type,
				     float cap,
				     Requireds requireds,
				     Pin *load_pin,
				     Point location,
				     RebufferOption *ref,
				     RebufferOption *ref2);
  void deleteRebufferOptions();

  int rebuffer_net_count_;
  RebufferOptionSeq rebuffer_options_;
  friend class RebufferOption;

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

// for testing
void
rebuffer_net(Net *net,
	     LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  LibertyLibrarySeq *resize_libs = new LibertyLibrarySeq;
  resize_libs->push_back(buffer_cell->libertyLibrary());
  resizer->resizePreamble(resize_libs);
  resizer->rebuffer(net, buffer_cell);
}

////////////////////////////////////////////////////////////////

void
repair_max_fanout_cmd(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairMaxFanout(buffer_cell);
}

  void repairMaxFanout(LibertyCell *buffer_cell);
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

  if (max_fanout_violation_count > 0)
    printf("Found %d max fanout violations.\n", max_fanout_violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers.\n", inserted_buffer_count_);
    level_drvr_verticies_valid_ = false;
  }
}

////////////////////////////////////////////////////////////////

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
    double load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap_);
    double drive_res = cellDriveResistance(inst_cell);
    double rc_delay0 = drive_res * load_cap;
    // Downsize until RC delay > hold violation.
    for (int k = i - 1; k >= 0; k--) {
      LibertyCell *equiv = (*equiv_cells)[k];
      double drive_res = cellDriveResistance(equiv);
      double rc_delay = drive_res * load_cap;
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
  Point drvr_loc = db_network_->location(drvr_pin);
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
      setLocation(buffer, Point((drvr_loc.getX() + center.getX()) / 2,
				(drvr_loc.getX() + center.getY()) / 2));
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
	uint64_t dist2 = Point::squaredDistance(db_network_->location(load),
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
    Point loc = db_network_->location(pin);
    sum.x() += loc.x();
    sum.y() += loc.y();
  }
  return Point(sum.x() / pins.size(), sum.y() / pins.size());
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

