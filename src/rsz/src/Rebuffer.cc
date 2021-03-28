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

#include "sta/Units.hh"
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Graph.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"

namespace rsz {

using std::min;
using std::max;

using utl::RSZ;

using sta::Units;
using sta::Port;
using sta::PinSeq;
using sta::PinSet;
using sta::NetConnectedPinIterator;
using sta::PathAnalysisPt;
using sta::fuzzyGreater;
using sta::fuzzyLess;
using sta::fuzzyInf;
using sta::INF;

class BufferedNet
{
public:
  BufferedNet(BufferedNetType type,
              float cap,
              Requireds requireds,
              Pin *load_pin,
              Point location,
              LibertyCell *buffer,
              BufferedNet *ref,
              BufferedNet *ref2);
  ~BufferedNet();
  void print(int level,
             Resizer *resizer);
  void printTree(Resizer *resizer);
  void printTree(int level,
                 Resizer *resizer);
  BufferedNetType type() const { return type_; }
  float cap() const { return cap_; }
  Required required(RiseFall *rf) { return requireds_[rf->index()]; }
  // Min of rise/fall requireds.
  Required required();
  // Required times at input of buffer_cell driving this option.
  Requireds bufferRequireds(LibertyCell *buffer_cell,
                            Resizer *resizer) const;
  // Required times at input of buffer_cell driving this option.
  Required bufferRequired(LibertyCell *buffer_cell,
                          Resizer *resizer) const;
  // driver   driver pin location
  // junction steiner point location connecting ref/ref2
  // wire     location opposite end of wire to location(ref_)
  // buffer   buffer driver pin location
  // load     load pin location
  Point location() const { return location_; }
  // Downstream buffer count.
  int bufferCount() const;
  // buffer
  LibertyCell *bufferCell() const { return buffer_cell_; }
  // load
  Pin *loadPin() const { return load_pin_; }
  // junction  left
  // buffer    wire
  // wire      end of wire
  BufferedNet *ref() const { return ref_; }
  // junction  right
  BufferedNet *ref2() const { return ref2_; }

private:
  BufferedNetType type_;
  // Capacitance looking into Net.
  float cap_;
  Point location_;
  // Rise/fall required times.
  Requireds requireds_;
  // Type load.
  Pin *load_pin_;
  // Type buffer.
  LibertyCell *buffer_cell_;
  BufferedNet *ref_;
  BufferedNet *ref2_;
};

////////////////////////////////////////////////////////////////

void
Resizer::rebuffer(const Pin *drvr_pin)
{
  Net *net;
  LibertyPort *drvr_port;
  if (network_->isTopLevelPort(drvr_pin)) {
    net = network_->net(network_->term(drvr_pin));
    LibertyCell *buffer_cell = buffer_lowest_drive_;
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
    SteinerTree *tree = makeSteinerTree(net, true, db_network_, logger_);
    if (tree) {
      SteinerPt drvr_pt = tree->drvrPt(network_);
      debugPrint(logger_, RSZ, "rebuffer", 2, "driver {}",
                 sdc_network_->pathName(drvr_pin));
      BufferedNetSeq Z = rebufferBottomUp(tree, tree->left(drvr_pt),
                                             drvr_pt, 1);
      Required best_slack = -INF;
      BufferedNet *best_option = nullptr;
      int best_index = 0;
      int i = 1;
      for (BufferedNet *p : Z) {
        // Find slack for drvr_pin into option.
        ArcDelay gate_delays[RiseFall::index_count];
        Slew slews[RiseFall::index_count];
        gateDelays(drvr_port, p->cap(), dcalc_ap_, gate_delays, slews);
        Slack slacks[RiseFall::index_count] =
          {p->required(RiseFall::rise()) - gate_delays[RiseFall::riseIndex()],
           p->required(RiseFall::fall()) - gate_delays[RiseFall::fallIndex()]};
        RiseFall *rf = (slacks[RiseFall::riseIndex()] < slacks[RiseFall::fallIndex()])
          ? RiseFall::rise()
          : RiseFall::fall();
        int rf_index = rf->index();
        debugPrint(logger_, RSZ, "rebuffer", 3,
                   "option {:3d}: {:2d} buffers req {} {} - {} = {} cap {}",
                   i,
                   p->bufferCount(),
                   rf->asString(),
                   delayAsString(p->required(rf), this, 3),
                   delayAsString(gate_delays[rf_index], this, 3),
                   delayAsString(slacks[rf_index], this, 3),
                   units_->capacitanceUnit()->asString(p->cap()));
        if (logger_->debugCheck(RSZ, "rebuffer", 4))
          p->printTree(this);
        if (fuzzyGreater(slacks[rf_index], best_slack)) {
          best_slack = slacks[rf_index];
          best_option = p;
          best_index = i;
        }
        i++;
      }
      if (best_option) {
        debugPrint(logger_, RSZ, "rebuffer", 3, "best option {}", best_index);
        if (logger_->debugCheck( RSZ, "rebuffer", 4))
          best_option->printTree(this);
        int before = inserted_buffer_count_;
        rebufferTopDown(best_option, net, 1);
        if (inserted_buffer_count_ != before)
          rebuffer_net_count_++;
      }
      rebuffer_options_.deleteContentsClear();
    }
    delete tree;
  }
}

// For testing.
void
Resizer::rebuffer(Net *net)
{
  inserted_buffer_count_ = 0;
  rebuffer_net_count_ = 0;
  PinSet *drvrs = network_->drivers(net);
  PinSet::Iterator drvr_iter(drvrs);
  if (drvr_iter.hasNext()) {
    Pin *drvr = drvr_iter.next();
    rebuffer(drvr);
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
BufferedNetSeq
Resizer::rebufferBottomUp(SteinerTree *tree,
                          SteinerPt k,
                          SteinerPt prev,
                          int level)
{
  if (k != SteinerTree::null_pt) {
    Pin *pin = tree->pin(k);
    if (pin && network_->isLoad(pin)) {
      // Load capacitance and required time.
      BufferedNet *z = makeBufferedNet(BufferedNetType::load,
                                       pinCapacitance(pin),
                                       pinRequireds(pin),
                                       pin,
                                       tree->location(k),
                                       nullptr,
                                       nullptr, nullptr);
      if (logger_->debugCheck(RSZ, "rebuffer", 4))
        z->print(level, this);
      BufferedNetSeq Z;
      Z.push_back(z);
      return addWireAndBuffer(Z, tree, k, prev, level);
    }
    else if (pin == nullptr) {
      // Steiner pt.
      BufferedNetSeq Zl = rebufferBottomUp(tree, tree->left(k), k, level + 1);
      BufferedNetSeq Zr = rebufferBottomUp(tree, tree->right(k), k, level + 1);
      BufferedNetSeq Z;
      // Combine the options from both branches.
      for (BufferedNet *p : Zl) {
        for (BufferedNet *q : Zr) {
          Requireds junc_reqs{min(p->required(RiseFall::rise()),
                                  q->required(RiseFall::rise())),
                              min(p->required(RiseFall::fall()),
                                  q->required(RiseFall::fall()))};
          BufferedNet *junc = makeBufferedNet(BufferedNetType::junction,
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
      sort(Z, [](BufferedNet *option1,
                 BufferedNet *option2)
              {   return fuzzyGreater(option1->required(),
                                      option2->required());
              });
      int si = 0;
      for (size_t pi = 0; pi < Z.size(); pi++) {
        BufferedNet *p = Z[pi];
        float Lp = p->cap();
        // Remove options by shifting down with index si.
        si = pi + 1;
        // Because the options are sorted we don't have to look
        // beyond the first option.
        for (size_t qi = pi + 1; qi < Z.size(); qi++) {
          BufferedNet *q = Z[qi];
          float Lq = q->cap();
          // We know Tq <= Tp from the sort so we don't need to check req.
          // If q is the same or worse than p, remove solution q.
          if (fuzzyLess(Lq, Lp))
            // Copy survivor down.
            Z[si++] = q;
        }
        Z.resize(si);
      }
      return addWireAndBuffer(Z, tree, k, prev, level);
    }
  }
  return BufferedNetSeq();
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
  Requireds requireds;
  for (RiseFall *rf : RiseFall::range()) {
    int rf_index = rf->index();
    Required required = sta_->vertexRequired(vertex, rf, max_);
    if (fuzzyInf(required))
      // Unconstrained pin.
      required = 0.0;
    requireds[rf_index] = required;
  }
  return requireds;
}

BufferedNetSeq
Resizer::addWireAndBuffer(BufferedNetSeq Z,
                          SteinerTree *tree,
                          SteinerPt k,
                          SteinerPt prev,
                          int level)
{
  BufferedNetSeq Z1;
  Point k_loc = tree->location(k);
  Point prev_loc = tree->location(prev);
  int wire_length_dbu = abs(k_loc.x() - prev_loc.x())
    + abs(k_loc.y() - prev_loc.y());
  float wire_length = dbuToMeters(wire_length_dbu);
  float wire_cap = wire_length * wire_cap_;
  float wire_res = wire_length * wire_res_;
  float wire_delay = wire_res * wire_cap;
  for (BufferedNet *p : Z) {
    // account for wire delay
    Requireds reqs{p->required(RiseFall::rise()) - wire_delay,
                   p->required(RiseFall::fall()) - wire_delay};
    BufferedNet *z = makeBufferedNet(BufferedNetType::wire,
                                     // account for wire load
                                     p->cap() + wire_cap,
                                     reqs,
                                     nullptr,
                                     prev_loc,
                                     nullptr,
                                     p, nullptr);
    if (logger_->debugCheck(RSZ, "rebuffer", 4)) {
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
    BufferedNetSeq buffered_options;
    for (LibertyCell *buffer_cell : buffer_cells_) {
      Required best_req = -INF;
      BufferedNet *best_option = nullptr;
      for (BufferedNet *z : Z1) {
        Required req = z->bufferRequired(buffer_cell, this);
        if (fuzzyGreater(req, best_req)) {
          best_req = req;
          best_option = z;
        }
      }
      Requireds requireds = best_option->bufferRequireds(buffer_cell, this);
      Required required = min(requireds[RiseFall::riseIndex()],
                              requireds[RiseFall::fallIndex()]);
      float buffer_cap = bufferInputCapacitance(buffer_cell);
      // Don't add this buffer option if it has worse input cap and req than
      // another existing buffer option.
      bool prune = false;
      for (BufferedNet *buffer_option : buffered_options) {
        if (buffer_option->cap() <= buffer_cap
            && buffer_option->required() >= required) {
          prune = true;
          break;
        }
      }
      if (!prune) {
        BufferedNet *z = makeBufferedNet(BufferedNetType::buffer,
                                         buffer_cap,
                                         requireds,
                                         nullptr,
                                         // Locate buffer at opposite end of wire.
                                         prev_loc,
                                         buffer_cell,
                                         best_option, nullptr);
        if (logger_->debugCheck(RSZ, "rebuffer", 3)) {
          printf("%*sbuffer %s cap %s req %s ->\n",
                 level, "",
                 tree->name(prev, sdc_network_),
                 units_->capacitanceUnit()->asString(best_option->cap()),
                 delayAsString(best_req, this));
          z->print(level, this);
        }
        buffered_options.push_back(z);
      }
    }
    for (BufferedNet *z : buffered_options)
      Z1.push_back(z);
  }
  return Z1;
}

// Return inserted buffer count.
void
Resizer::rebufferTopDown(BufferedNet *choice,
                         Net *net,
                         int level)
{
  switch(choice->type()) {
  case BufferedNetType::buffer: {
    Instance *parent = db_network_->topInstance();
    string buffer_name = makeUniqueInstName("rebuffer");
    Net *net2 = makeUniqueNet();
    LibertyCell *buffer_cell = choice->bufferCell();
    Instance *buffer = db_network_->makeInstance(buffer_cell,
                                                 buffer_name.c_str(),
                                                 parent);
    journalMakeBuffer(buffer);
    inserted_buffer_count_++;
    design_area_ += area(db_network_->cell(buffer_cell));
    level_drvr_vertices_valid_ = false;
    LibertyPort *input, *output;
    buffer_cell->bufferPorts(input, output);
    debugPrint(logger_, RSZ, "rebuffer", 3, "%*sinsert {} -> {} ({}) -> {}",
               level, "",
               sdc_network_->pathName(net),
               buffer_name.c_str(),
               buffer_cell->name(),
               sdc_network_->pathName(net2));
    sta_->connectPin(buffer, input, net);
    sta_->connectPin(buffer, output, net2);
    setLocation(buffer, choice->location());
    rebufferTopDown(choice->ref(), net2, level + 1);
    parasiticsInvalid(net);
    break;
  }
  case BufferedNetType::wire:
    debugPrint(logger_, RSZ, "rebuffer", 3, "%*swire", level, "");
    rebufferTopDown(choice->ref(), net, level + 1);
    break;
  case BufferedNetType::junction: {
    debugPrint(logger_, RSZ, "rebuffer", 3, "%*sjunction", level, "");
    rebufferTopDown(choice->ref(), net, level + 1);
    rebufferTopDown(choice->ref2(), net, level + 1);
    break;
  }
  case BufferedNetType::load: {
    Pin *load_pin = choice->loadPin();
    Net *load_net = network_->net(load_pin);
    if (load_net != net) {
      Instance *load_inst = db_network_->instance(load_pin);
      Port *load_port = db_network_->port(load_pin);
      debugPrint(logger_, RSZ, "rebuffer", 3, "%*sconnect load %s to %s",
                 level, "",
                 sdc_network_->pathName(load_pin),
                 sdc_network_->pathName(net));
      sta_->disconnectPin(load_pin);
      sta_->connectPin(load_inst, load_port, net);
      parasiticsInvalid(load_net);
    }
    break;
  }
  }
}

////////////////////////////////////////////////////////////////

BufferedNet::BufferedNet(BufferedNetType type,
                         float cap,
                         Requireds requireds,
                         Pin *load_pin,
                         Point location,
                         LibertyCell *buffer_cell,
                         BufferedNet *ref,
                         BufferedNet *ref2) :
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

BufferedNet::~BufferedNet()
{
}

void
BufferedNet::printTree(Resizer *resizer)
{
  printTree(0, resizer);
}

void
BufferedNet::printTree(int level,
                       Resizer *resizer)
{
  print(level, resizer);
  switch (type_) {
  case BufferedNetType::load:
    break;
  case BufferedNetType::buffer:
  case BufferedNetType::wire:
    ref_->printTree(level + 1, resizer);
    break;
  case BufferedNetType::junction:
    ref_->printTree(level + 1, resizer);
    ref2_->printTree(level + 1, resizer);
    break;
  }
}

void
BufferedNet::print(int level,
                   Resizer *resizer)
{
  Network *sdc_network = resizer->sdcNetwork();
  Units *units = resizer->units();
  switch (type_) {
  case BufferedNetType::load:
    // %*s format indents level spaces.
    printf("%*sload %s (%d, %d) cap %s req %s\n",
           level, "",
           sdc_network->pathName(load_pin_),
           location_.x(), location_.y(),
           units->capacitanceUnit()->asString(cap_),
           delayAsString(required(), resizer));
    break;
  case BufferedNetType::wire:
    printf("%*swire (%d, %d) cap %s req %s\n",
           level, "",
           location_.x(), location_.y(),
           units->capacitanceUnit()->asString(cap_),
           delayAsString(required(), resizer));
    break;
  case BufferedNetType::buffer:
    printf("%*sbuffer (%d, %d) %s cap %s req %s\n",
           level, "",
           location_.x(), location_.y(),
           buffer_cell_->name(),
           units->capacitanceUnit()->asString(cap_),
           delayAsString(required(), resizer));
    break;
  case BufferedNetType::junction:
    printf("%*sjunction (%d, %d) cap %s req %s\n",
           level, "",
           location_.x(), location_.y(),
           units->capacitanceUnit()->asString(cap_),
           delayAsString(required(), resizer));
    break;
  }
}

Required
BufferedNet::required()
{
  return min(requireds_[RiseFall::riseIndex()],
             requireds_[RiseFall::fallIndex()]);
}

Requireds
BufferedNet::bufferRequireds(LibertyCell *buffer_cell,
                             Resizer *resizer) const
{
  Requireds requireds = {requireds_[RiseFall::riseIndex()]
                         - resizer->bufferDelay(buffer_cell, RiseFall::rise(), cap_),
                         requireds_[RiseFall::fallIndex()]
                         - resizer->bufferDelay(buffer_cell, RiseFall::fall(), cap_)};
  return requireds;
}

Required
BufferedNet::bufferRequired(LibertyCell *buffer_cell,
                            Resizer *resizer) const
{
  Requireds requireds = bufferRequireds(buffer_cell, resizer);
  return min(requireds[RiseFall::riseIndex()],
             requireds[RiseFall::fallIndex()]);
}

int
BufferedNet::bufferCount() const
{
  switch (type_) {
  case BufferedNetType::buffer:
    return ref_->bufferCount() + 1;
  case BufferedNetType::wire:
    return ref_->bufferCount();
  case BufferedNetType::junction:
    return ref_->bufferCount() + ref2_->bufferCount();
  case BufferedNetType::load:
    return 0;
  }
  return 0;
}

BufferedNet *
Resizer::makeBufferedNet(BufferedNetType type,
                         float cap,
                         Requireds requireds,
                         Pin *load_pin,
                         Point location,
                         LibertyCell *buffer_cell,
                         BufferedNet *ref,
                         BufferedNet *ref2)
{
  BufferedNet *option = new BufferedNet(type, cap, requireds,
                                        load_pin, location,
                                        buffer_cell,
                                        ref, ref2);

  rebuffer_options_.push_back(option);
  return option;
}

} // namespace
