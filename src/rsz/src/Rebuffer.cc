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

#include "rsz/BufferedNet.hh"

#include "sta/Units.hh"
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Graph.hh"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"
#include "sta/DcalcAnalysisPt.hh"

namespace rsz {

using std::min;
using std::max;

using utl::RSZ;

using sta::Unit;
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
      sta_->findRequireds();
      BufferedNetSeq Z = rebufferBottomUp(tree, tree->left(drvr_pt),
                                          drvr_pt, 1);
      Required best_slack = -INF;
      BufferedNet *best_option = nullptr;
      int best_index = 0;
      int i = 1;
      for (BufferedNet *p : Z) {
        // Find slack for drvr_pin into option.
        const PathRef &req_path = p->requiredPath();
        if (!req_path.isNull()) {
          Delay drvr_delay = gateDelay(drvr_port, req_path.transition(sta_),
                                       p->cap(), req_path.dcalcAnalysisPt(sta_));
          Slack slack = p->required(sta_) - drvr_delay;
          debugPrint(logger_, RSZ, "rebuffer", 3,
                     "option {:3d}: {:2d} buffers req {} - {} = {} cap {}",
                     i,
                     p->bufferCount(),
                     delayAsString(p->required(sta_), this, 3),
                     delayAsString(drvr_delay, this, 3),
                     delayAsString(slack, this, 3),
                     units_->capacitanceUnit()->asString(p->cap()));
          if (logger_->debugCheck(RSZ, "rebuffer", 4))
            p->reportTree(this);
          if (fuzzyGreater(slack, best_slack)) {
            best_slack = slack;
            best_option = p;
            best_index = i;
          }
          i++;
        }
      }
      if (best_option) {
        debugPrint(logger_, RSZ, "rebuffer", 3, "best option {}", best_index);
        if (logger_->debugCheck( RSZ, "rebuffer", 4))
          best_option->reportTree(this);
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

BufferedNetSeq
Resizer::rebufferBottomUp(SteinerTree *tree,
                          SteinerPt k,
                          SteinerPt prev,
                          int level)
{
  if (k != SteinerTree::null_pt) {
    Pin *pin = tree->pin(k);
    if (pin && network_->isLoad(pin)) {
      Vertex *vertex = graph_->pinLoadVertex(pin);
      PathRef req_path = sta_->vertexWorstRequiredPath(vertex, max_);
      const DcalcAnalysisPt *dcalc_ap = req_path.isNull()
        ? tgt_slew_dcalc_ap_
        : req_path.dcalcAnalysisPt(sta_);
      BufferedNet *z = makeBufferedNet(BufferedNetType::load,
                                       tree->location(k),
                                       pinCapacitance(pin, dcalc_ap),
                                       pin,
                                       req_path,
                                       0.0,
                                       nullptr,
                                       nullptr, nullptr);
      if (logger_->debugCheck(RSZ, "rebuffer", 4))
        z->report(level, this);
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
          BufferedNet *min_req = fuzzyLess(p->required(sta_), q->required(sta_)) ? p : q;
          BufferedNet *junc = makeBufferedNet(BufferedNetType::junction,
                                              tree->location(k),
                                              p->cap() + q->cap(),
                                              nullptr,
                                              min_req->requiredPath(),
                                              min_req->requiredDelay(),
                                              nullptr,
                                              p, q);
          Z.push_back(junc);
        }
      }
      // Prune the options. This is fanout^2.
      // Presort options to hit better options sooner.
      sort(Z, [=](BufferedNet *option1,
                  BufferedNet *option2)
              { return fuzzyGreater(option1->required(sta_),
                                    option2->required(sta_));
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
Resizer::pinCapacitance(const Pin *pin,
                        const DcalcAnalysisPt *dcalc_ap)
{
  LibertyPort *port = network_->libertyPort(pin);
  if (port) {
    int lib_ap = dcalc_ap->libertyIndex();
    LibertyPort *corner_port = port->cornerPort(lib_ap);
    return corner_port->capacitance();
  }
  else
    return 0.0;
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
  double wire_length = dbuToMeters(wire_length_dbu);
  for (BufferedNet *p : Z) {
    const PathRef &req_path = p->requiredPath();
    const Corner *corner = req_path.isNull()
      ? sta_->cmdCorner()
      : req_path.dcalcAnalysisPt(sta_)->corner();
    double wire_cap = wire_length * wireSignalCapacitance(corner);
    double wire_res = wire_length * wireSignalResistance(corner);
    double wire_delay = wire_res * wire_cap;
    BufferedNet *z = makeBufferedNet(BufferedNetType::wire,
                                     prev_loc,
                                     // account for wire load
                                     p->cap() + wire_cap,
                                     nullptr,
                                     req_path,
                                     // account for wire delay
                                     p->requiredDelay() + wire_delay,
                                     nullptr,
                                     p, nullptr);
    if (logger_->debugCheck(RSZ, "rebuffer", 4)) {
      printf("%*swire %s -> %s wl %d\n",
             level, "",
             tree->name(prev, sdc_network_),
             tree->name(k, sdc_network_),
             wire_length_dbu);
      z->report(level, this);
    }
    Z1.push_back(z);
  }
  if (!Z1.empty()) {
    BufferedNetSeq buffered_options;
    for (LibertyCell *buffer_cell : buffer_cells_) {
      Required best_req = -INF;
      BufferedNet *best_option = nullptr;
      for (BufferedNet *z : Z1) {
        Required req = INF;
        PathRef req_path = z->requiredPath();
        if (!req_path.isNull()) {
          const DcalcAnalysisPt *dcalc_ap = req_path.dcalcAnalysisPt(sta_);
          Delay buffer_delay = bufferDelay(buffer_cell,
                                           req_path.transition(sta_),
                                           z->cap(), dcalc_ap);
          req = z->required(sta_) - buffer_delay;
        }
        if (fuzzyGreater(req, best_req)) {
          best_req = req;
          best_option = z;
        }
      }
      if (best_option) {
        Required required = INF;
        PathRef req_path = best_option->requiredPath();
        float buffer_cap = 0.0;
        Delay buffer_delay = 0.0;
        if (!req_path.isNull()) {
          const DcalcAnalysisPt *dcalc_ap = req_path.dcalcAnalysisPt(sta_);
          buffer_cap = bufferInputCapacitance(buffer_cell, dcalc_ap);
          buffer_delay = bufferDelay(buffer_cell,
                                     req_path.transition(sta_),
                                     best_option->cap(),
                                     dcalc_ap);
          required = req_path.required(sta_) - buffer_delay;
        }
        // Don't add this buffer option if it has worse input cap and req than
        // another existing buffer option.
        bool prune = false;
        for (BufferedNet *buffer_option : buffered_options) {
          if (buffer_option->cap() <= buffer_cap
              && buffer_option->required(sta_) >= required) {
            prune = true;
            break;
          }
        }
        if (!prune) {
          BufferedNet *z = makeBufferedNet(BufferedNetType::buffer,
                                           // Locate buffer at opposite end of wire.
                                           prev_loc,
                                           buffer_cap,
                                           nullptr,
                                           req_path,
                                           best_option->requiredDelay()+buffer_delay,
                                           buffer_cell,
                                           best_option, nullptr);
          if (logger_->debugCheck(RSZ, "rebuffer", 3)) {
            printf("%*sbuffer %s cap %s req %s ->\n",
                   level, "",
                   tree->name(prev, sdc_network_),
                   units_->capacitanceUnit()->asString(best_option->cap()),
                   delayAsString(best_req, this));
            z->report(level, this);
          }
          buffered_options.push_back(z);
        }
      }
    }
    for (BufferedNet *z : buffered_options)
      Z1.push_back(z);
  }
  return Z1;
}

float
Resizer::bufferInputCapacitance(LibertyCell *buffer_cell,
                                const DcalcAnalysisPt *dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  int lib_ap = dcalc_ap->libertyIndex();
  LibertyPort *corner_input = input->cornerPort(lib_ap);
  return corner_input->capacitance();
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
    if (logger_->debugCheck(RSZ, "rebuffer", 3))
      printf("%*sinsert %s -> %s (%s) -> %s",
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
    if (logger_->debugCheck(RSZ, "rebuffer", 3))
      printf("%*swire", level, "");
    rebufferTopDown(choice->ref(), net, level + 1);
    break;
  case BufferedNetType::junction: {
    if (logger_->debugCheck(RSZ, "rebuffer", 3))
      printf("%*sjunction", level, "");
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
      if (logger_->debugCheck(RSZ, "rebuffer", 3))
        printf("%*sconnect load %s to %s",
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

BufferedNet *
Resizer::makeBufferedNet(BufferedNetType type,
                         Point location,
                         float cap,
                         Pin *load_pin,
                         PathRef req_path,
                         Delay req_delay,
                         LibertyCell *buffer_cell,
                         BufferedNet *ref,
                         BufferedNet *ref2)
{
  BufferedNet *option = new BufferedNet(type, location, cap,
                                        load_pin, req_path, req_delay,
                                        buffer_cell, ref, ref2);

  rebuffer_options_.push_back(option);
  return option;
}

// Make BufferedNet from steiner tree.
BufferedNet *
Resizer::makeBufferedNetSteiner(const Pin *drvr_pin)
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
      BufferedNet *bnet = makeBufferedNetWire(tree, drvr_pt, tree->left(drvr_pt), 0);
      delete tree;
      return bnet;
    }
  }
  return nullptr;
}

BufferedNet *
Resizer::makeBufferedNet(SteinerTree *tree,
                         SteinerPt k,
                         SteinerPt prev,
                         int level)
{
  if (k != SteinerTree::null_pt) {
    Pin *pin = tree->pin(k);
    if (pin && network_->isLoad(pin)) {
      return new BufferedNet(BufferedNetType::load,
                             tree->location(k),
                             // should wait unil req path is known to find
                             // dcalc_ap
                             pinCapacitance(pin, tgt_slew_dcalc_ap_),
                             pin,
                             nullptr, nullptr);
    }
    else if (pin == nullptr) {
      // Steiner pt.
      BufferedNet *bnet1 = makeBufferedNetWire(tree, k, tree->left(k), level + 1);
      BufferedNet *bnet2 = makeBufferedNetWire(tree, k, tree->right(k), level + 1);
      if (bnet1 && bnet2)
        return new BufferedNet(BufferedNetType::junction,
                               tree->location(k),
                               bnet1->cap() + bnet2->cap(),
                               nullptr,
                               bnet1, bnet2);
    }
  }
  return nullptr;
}

// Wire from steiner pt prev to k.
BufferedNet *
Resizer::makeBufferedNetWire(SteinerTree *tree,
                             SteinerPt from,
                             SteinerPt to,
                             int level)
{
  BufferedNet *end = makeBufferedNet(tree, to, from, level);
  if (end) {
    Point from_loc = tree->location(from);
    Point to_loc = tree->location(to);
    int wire_length_dbu = abs(from_loc.x() - to_loc.x())
      + abs(from_loc.y() - to_loc.y());
    const Corner *corner = end->ref()->requiredPath().dcalcAnalysisPt(sta_)->corner();
    double wire_length = dbuToMeters(wire_length_dbu);
    double wire_cap = wire_length * wireSignalCapacitance(corner);
    double wire_res = wire_length * wireSignalResistance(corner);

    return new BufferedNet(BufferedNetType::wire,
                           from_loc,
                           // account for wire load
                           end->cap() + wire_cap,
                           nullptr,
                           end, nullptr);

  }
  return nullptr;
}

} // namespace
