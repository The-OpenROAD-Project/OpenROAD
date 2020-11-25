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

#include "resizer/Resizer.hh"

#include "sta/Debug.hh"
#include "sta/Units.hh"
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Graph.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"

using std::min;
using std::max;

namespace sta {

class RebufferOption
{
public:
  RebufferOption(RebufferOptionType type,
                 float cap,
                 Requireds requireds,
                 Pin *load_pin,
                 Point location,
                 LibertyCell *buffer,
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
  LibertyCell *bufferCell() const { return buffer_cell_; }
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
  LibertyCell *buffer_cell_;
  RebufferOption *ref_;
  RebufferOption *ref2_;
};

RebufferOption::RebufferOption(RebufferOptionType type,
                               float cap,
                               Requireds requireds,
                               Pin *load_pin,
                               Point location,
                               LibertyCell *buffer_cell,
                               RebufferOption *ref,
                               RebufferOption *ref2) :
  type_(type),
  cap_(cap),
  requireds_(requireds),
  load_pin_(load_pin),
  location_(location),
  buffer_cell_(buffer_cell),
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
    printf("%*sbuffer (%d, %d) %s cap %s req %s\n",
           level, "",
           location_.x(), location_.y(),
           buffer_cell_->name(),
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
                            LibertyCell *buffer_cell,
                            RebufferOption *ref,
                            RebufferOption *ref2)
{
  RebufferOption *option = new RebufferOption(type, cap, requireds,
                                              load_pin, location,
                                              buffer_cell,
                                              ref, ref2);

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
        // Find slack for drvr_pin into option.
        ArcDelay gate_delays[RiseFall::index_count];
        Slew slews[RiseFall::index_count];
        gateDelays(drvr_port, p->cap(), gate_delays, slews);
        Slack slacks[RiseFall::index_count] =
          {p->required(RiseFall::rise()) - gate_delays[RiseFall::riseIndex()],
           p->required(RiseFall::fall()) - gate_delays[RiseFall::fallIndex()]};
        RiseFall *rf = (slacks[RiseFall::riseIndex()] < slacks[RiseFall::fallIndex()])
          ? RiseFall::rise()
          : RiseFall::fall();
        int rf_index = rf->index();
        debugPrint7(debug_, "rebuffer", 3,
                    "option %3d: %2d buffers req %s %s - %s = %s cap %s\n",
                    i,
                    p->bufferCount(),
                    rf->asString(),
                    delayAsString(p->required(rf), this, 3),
                    delayAsString(gate_delays[rf_index], this, 3),
                    delayAsString(slacks[rf_index], this, 3),
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
        if (debug_->check("rebuffer", 4))
          best_option->printTree(this);
        int before = inserted_buffer_count_;
        rebufferTopDown(best_option, net, 1);
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
                                             nullptr,
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
                                                    nullptr,
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

float
Resizer::pinCapacitance(const Pin *pin)
{
  LibertyPort *port = network_->libertyPort(pin);
  if (port)
    return portCapacitance(port);
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

RebufferOptionSeq
Resizer::addWireAndBuffer(RebufferOptionSeq Z,
                          SteinerTree *tree,
                          SteinerPt k,
                          SteinerPt prev,
                          int level,
                          LibertyCell *buffer_cell)
{
  RebufferOptionSeq Z1;
  Point k_loc = tree->location(k);
  Point prev_loc = tree->location(prev);
  int wire_length_dbu = abs(k_loc.x() - prev_loc.x())
    + abs(k_loc.y() - prev_loc.y());
  float wire_length = dbuToMeters(wire_length_dbu);
  float wire_cap = wire_length * wire_cap_;
  float wire_res = wire_length * wire_res_;
  float wire_delay = wire_res * wire_cap;
  LibertyCellSeq *buffer_cells = sta_->equivCells(buffer_cell);    
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
                                           nullptr,
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
  }
  if (!Z1.empty()) {
    RebufferOptionSeq buffered_options;
    for (LibertyCell *buffer_cell : *buffer_cells) {
      Required best_req = -INF;
      RebufferOption *best_option = nullptr;
      for (RebufferOption *z : Z1) {
        Required req = z->bufferRequired(buffer_cell, this);
        if (fuzzyGreater(req, best_req)) {
          best_req = req;
          best_option = z;
        }
      }
      Requireds requireds = best_option->bufferRequireds(buffer_cell, this);
      RebufferOption *z = makeRebufferOption(RebufferOptionType::buffer,
                                             bufferInputCapacitance(buffer_cell),
                                             requireds,
                                             nullptr,
                                             // Locate buffer at opposite end of wire.
                                             prev_loc,
                                             buffer_cell,
                                             best_option, nullptr);
      if (debug_->check("rebuffer", 3)) {
        printf("%*sbuffer %s cap %s req %s ->\n",
               level, "",
               tree->name(prev, sdc_network_),
               units_->capacitanceUnit()->asString(best_option->cap()),
               delayAsString(best_req, this));
        z->print(level, this);
      }
      buffered_options.push_back(z);
    }
    for (RebufferOption *z : buffered_options)
      Z1.push_back(z);
  }
  return Z1;
}

// Return inserted buffer count.
void
Resizer::rebufferTopDown(RebufferOption *choice,
                         Net *net,
                         int level)
{
  switch(choice->type()) {
  case RebufferOptionType::buffer: {
    Instance *parent = db_network_->topInstance();
    string net2_name = makeUniqueNetName();
    string buffer_name = makeUniqueInstName("rebuffer");
    Net *net2 = db_network_->makeNet(net2_name.c_str(), parent);
    LibertyCell *buffer_cell = choice->bufferCell();
    Instance *buffer = db_network_->makeInstance(buffer_cell,
                                                 buffer_name.c_str(),
                                                 parent);
    inserted_buffer_count_++;
    design_area_ += area(db_network_->cell(buffer_cell));
    level_drvr_verticies_valid_ = false;
    LibertyPort *input, *output;
    buffer_cell->bufferPorts(input, output);
    debugPrint6(debug_, "rebuffer", 3, "%*sinsert %s -> %s (%s) -> %s\n",
                level, "",
                sdc_network_->pathName(net),
                buffer_name.c_str(),
                buffer_cell->name(),
                net2_name.c_str());
    sta_->connectPin(buffer, input, net);
    sta_->connectPin(buffer, output, net2);
    setLocation(buffer, choice->location());
    rebufferTopDown(choice->ref(), net2, level + 1);
    parasitics_->deleteParasitics(net, parasitics_ap_);
    break;
  }
  case RebufferOptionType::wire:
    debugPrint2(debug_, "rebuffer", 3, "%*swire\n", level, "");
    rebufferTopDown(choice->ref(), net, level + 1);
    break;
  case RebufferOptionType::junction: {
    debugPrint2(debug_, "rebuffer", 3, "%*sjunction\n", level, "");
    rebufferTopDown(choice->ref(), net, level + 1);
    rebufferTopDown(choice->ref2(), net, level + 1);
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
      parasitics_->deleteParasitics(load_net, parasitics_ap_);
    }
    break;
  }
  }
}

} // namespace sta
