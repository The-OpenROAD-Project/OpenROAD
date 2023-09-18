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

#include "RepairSetup.hh"
#include "BufferedNet.hh"
#include "rsz/Resizer.hh"

#include "db_sta/dbNetwork.hh"

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
using std::make_shared;

using utl::RSZ;

using sta::Unit;
using sta::Units;
using sta::Port;
using sta::PinSeq;
using sta::PinSet;
using sta::NetConnectedPinIterator;
using sta::PathAnalysisPt;
using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::fuzzyLessEqual;
using sta::fuzzyEqual;
using sta::fuzzyInf;
using sta::INF;

// Return inserted buffer count.
int
RepairSetup::rebuffer(const Pin *drvr_pin)
{
  int inserted_buffer_count = 0;
  Net *net;
  if (network_->isTopLevelPort(drvr_pin)) {
    net = network_->net(network_->term(drvr_pin));
    LibertyCell *buffer_cell = resizer_->buffer_lowest_drive_;
    // Should use sdc external driver here.
    LibertyPort *input;
    buffer_cell->bufferPorts(input, drvr_port_);
  }
  else {
    net = network_->net(drvr_pin);
    drvr_port_ = network_->libertyPort(drvr_pin);
  }
  if (drvr_port_
      && net
      // Verilog connects by net name, so there is no way to distinguish the
      // net from the port.
      && !hasTopLevelOutputPort(net)) {
    corner_ = sta_->cmdCorner();
    BufferedNetPtr bnet = resizer_->makeBufferedNet(drvr_pin, corner_);
    if (bnet) {
      bool debug = (drvr_pin == resizer_->debug_pin_);
      if (debug)
        logger_->setDebugLevel(RSZ, "rebuffer", 3);
      debugPrint(logger_, RSZ, "rebuffer", 2, "driver {}",
                 sdc_network_->pathName(drvr_pin));
      sta_->findRequireds();
      BufferedNetSeq Z = rebufferBottomUp(bnet, 1);
      Required best_slack_penalized = -INF;
      BufferedNetPtr best_option = nullptr;
      int best_index = 0;
      int i = 1;
      for (BufferedNetPtr p : Z) {
        // Find slack for drvr_pin into option.
        const PathRef &req_path = p->requiredPath();
        if (!req_path.isNull()) {
          Slack slack_penalized = slackPenalized(p, i);
          if (best_option == nullptr
              || fuzzyGreater(slack_penalized, best_slack_penalized)) {
            best_slack_penalized = slack_penalized;
            best_option = p;
            best_index = i;
          }
          i++;
        }
      }
      if (best_option) {
        debugPrint(logger_, RSZ, "rebuffer", 2, "best option {}", best_index);
        inserted_buffer_count = rebufferTopDown(best_option, net, 1);
        if (inserted_buffer_count > 0) {
          rebuffer_net_count_++;
          debugPrint(logger_, RSZ, "rebuffer", 2, "rebuffer {} inserted {}",
                     network_->pathName(drvr_pin),
                     inserted_buffer_count);
        }
      }
      if (debug)
        logger_->setDebugLevel(RSZ, "rebuffer", 0);
    }
    else 
      logger_->warn(RSZ, 75, "makeBufferedNet failed for driver {}",
                    network_->pathName(drvr_pin));
  }
  return inserted_buffer_count;
}

Slack
RepairSetup::slackPenalized(BufferedNetPtr bnet)
{
  return slackPenalized(bnet, -1);
}

Slack
RepairSetup::slackPenalized(BufferedNetPtr bnet,
                            // Only used for debug print.
                            int index)
{
  const PathRef &req_path = bnet->requiredPath();
  if (!req_path.isNull()) {
    Delay drvr_delay = resizer_->gateDelay(drvr_port_, req_path.transition(sta_),
                                           bnet->cap(), req_path.dcalcAnalysisPt(sta_));
    Slack slack = bnet->required(sta_) - drvr_delay;
    int buffer_count = bnet->bufferCount();
    double buffer_penalty = buffer_count * rebuffer_buffer_penalty_;
    double slack_penalized = slack * (1.0 - (slack > 0
                                             ? buffer_penalty
                                             : -buffer_penalty));
    if (index >= 0)
      debugPrint(logger_, RSZ, "rebuffer", 2,
                 "option {:3d}: {:2d} buffers req {} - {} = {} * {:3.2f} = {} cap {}",
                 index,
                 bnet->bufferCount(),
                 delayAsString(bnet->required(sta_), this, 3),
                 delayAsString(drvr_delay, this, 3),
                 delayAsString(slack, this, 3),
                 buffer_penalty,
                 delayAsString(slack_penalized, this, 3),
                 units_->capacitanceUnit()->asString(bnet->cap()));
    return slack_penalized;
  }
  else
    return INF;
}

// For testing.
void
RepairSetup::rebufferNet(const Pin *drvr_pin)
{
  init();
  resizer_->incrementalParasiticsBegin();
  inserted_buffer_count_ = rebuffer(drvr_pin);
  // Leave the parasitics up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();
  logger_->report("Inserted {} buffers.", inserted_buffer_count_);
}

bool
RepairSetup::hasTopLevelOutputPort(Net *net)
{
  NetConnectedPinIterator *pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin *pin = pin_iter->next();
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
RepairSetup::rebufferBottomUp(BufferedNetPtr bnet,
                              int level)
{
  switch (bnet->type()) {
  case BufferedNetType::wire: {
    BufferedNetSeq Z = rebufferBottomUp(bnet->ref(), level + 1);
    return addWireAndBuffer(Z, bnet, level);
  }
  case BufferedNetType::junction: {
    BufferedNetSeq Z1 = rebufferBottomUp(bnet->ref(), level + 1);
    BufferedNetSeq Z2 = rebufferBottomUp(bnet->ref2(), level + 1);
    BufferedNetSeq Z;
    // Combine the options from both branches.
    for (BufferedNetPtr p : Z1) {
      for (BufferedNetPtr q : Z2) {
        BufferedNetPtr min_req = fuzzyLess(p->required(sta_),
                                           q->required(sta_)) ? p : q;
        BufferedNetPtr junc = make_shared<BufferedNet>(BufferedNetType::junction,
                                                       bnet->location(),
                                                       p, q, resizer_);
        junc->setCapacitance(p->cap() + q->cap());
        junc->setRequiredPath(min_req->requiredPath());
        junc->setRequiredDelay(min_req->requiredDelay());
        Z.push_back(junc);
      }
    }
    // Prune the options if there exists another option with
    // larger required and smaller capacitance.
    // This is fanout^2.
    // Presort options to hit better options sooner.
    sort(Z.begin(), Z.end(),
         [=](BufferedNetPtr option1,
             BufferedNetPtr option2)
         { Slack slack1 = slackPenalized(option1);
           Slack slack2 = slackPenalized(option2);
           return fuzzyGreater(slack1, slack2)
             || (fuzzyEqual(slack1, slack2)
                 && fuzzyLess(option1->cap(), option2->cap()));
         });
    int si = 0;
    for (size_t pi = 0; pi < Z.size(); pi++) {
      BufferedNetPtr p = Z[pi];
      float Lp = p->cap();
      // Remove options by shifting down with index si.
      si = pi + 1;
      // Because the options are sorted we don't have to look
      // beyond the first option.
      for (size_t qi = pi + 1; qi < Z.size(); qi++) {
        BufferedNetPtr q = Z[qi];
        float Lq = q->cap();
        // We know Tq <= Tp from the sort so we don't need to check req.
        // If q is the same or worse than p, remove solution q.
        if (fuzzyLess(Lq, Lp))
          // Copy survivor down.
          Z[si++] = q;
      }
      Z.resize(si);
    }
    return Z;
  }
  case BufferedNetType::load: {
    const Pin *load_pin = bnet->loadPin();
    Vertex *vertex = graph_->pinLoadVertex(load_pin);
    PathRef req_path = sta_->vertexWorstSlackPath(vertex, max_);
    const DcalcAnalysisPt *dcalc_ap = req_path.isNull()
      ? resizer_->tgt_slew_dcalc_ap_
      : req_path.dcalcAnalysisPt(sta_);
    bnet->setCapacitance(resizer_->pinCapacitance(load_pin, dcalc_ap));
    bnet->setRequiredPath(req_path);
    debugPrint(logger_, RSZ, "rebuffer", 4, "{:{}s}{}",
               "", level, bnet->to_string(resizer_));
    BufferedNetSeq Z;
    Z.push_back(bnet);
    return Z;
  }
  case BufferedNetType::buffer:
    logger_->critical(RSZ, 71, "unhandled BufferedNet type");
  }
  return BufferedNetSeq();
}

BufferedNetSeq
RepairSetup::addWireAndBuffer(BufferedNetSeq Z,
                              BufferedNetPtr bnet_wire,
                              int level)
{
  BufferedNetSeq Z1;
  Point wire_end = bnet_wire->location();
  for (BufferedNetPtr p : Z) {
    Point p_loc = p->location();
    int wire_length_dbu = abs(wire_end.x() - p_loc.x())
      + abs(wire_end.y() - p_loc.y());
    double wire_length = resizer_->dbuToMeters(wire_length_dbu);
    const PathRef &req_path = p->requiredPath();
    const Corner *corner = req_path.isNull()
      ? sta_->cmdCorner()
      : req_path.dcalcAnalysisPt(sta_)->corner();
    int wire_layer = bnet_wire->layer();
    double layer_res, layer_cap;
    bnet_wire->wireRC(corner, resizer_, layer_res, layer_cap);
    double wire_res = wire_length * layer_res;
    double wire_cap = wire_length * layer_cap;
    double wire_delay = wire_res * wire_cap;
    BufferedNetPtr z = make_shared<BufferedNet>(BufferedNetType::wire,
                                                wire_end, wire_layer, p,
                                                corner, resizer_);
    // account for wire load
    z->setCapacitance(p->cap() + wire_cap);
    z->setRequiredPath(req_path);
    // account for wire delay
    z->setRequiredDelay(p->requiredDelay() + wire_delay);
    debugPrint(logger_, RSZ, "rebuffer", 4, "{:{}s}wire wl {} {}",
               "", level,
               wire_length_dbu,
               z->to_string(resizer_));
    Z1.push_back(z);
  }
  if (!Z1.empty()) {
    BufferedNetSeq buffered_options;
    for (LibertyCell *buffer_cell : resizer_->buffer_cells_) {
      Required best_req = -INF;
      BufferedNetPtr best_option = nullptr;
      for (BufferedNetPtr z : Z1) {
        PathRef req_path = z->requiredPath();
        // Do not buffer unconstrained paths.
        if (!req_path.isNull()) {
          const DcalcAnalysisPt *dcalc_ap = req_path.dcalcAnalysisPt(sta_);
          Delay buffer_delay = resizer_->bufferDelay(buffer_cell,
                                                     req_path.transition(sta_),
                                                     z->cap(), dcalc_ap);
          Required req = z->required(sta_) - buffer_delay;
          if (fuzzyGreater(req, best_req)) {
            best_req = req;
            best_option = z;
          }
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
          buffer_delay = resizer_->bufferDelay(buffer_cell,
                                               req_path.transition(sta_),
                                               best_option->cap(),
                                               dcalc_ap);
          required = req_path.required(sta_) - buffer_delay;
        }
        // Don't add this buffer option if it has worse input cap and req than
        // another existing buffer option.
        bool prune = false;
        for (BufferedNetPtr buffer_option : buffered_options) {
          if (fuzzyLessEqual(buffer_option->cap(), buffer_cap)
              && fuzzyGreaterEqual(buffer_option->required(sta_), required)) {
            prune = true;
            break;
          }
        }
        if (!prune) {
          BufferedNetPtr z = make_shared<BufferedNet>(BufferedNetType::buffer,
                                                      // Locate buffer at opposite end of wire.
                                                      wire_end,
                                                      buffer_cell,
                                                      best_option,
                                                      corner_, resizer_);
          z->setCapacitance(buffer_cap);
          z->setRequiredPath(req_path);
          z->setRequiredDelay(best_option->requiredDelay() + buffer_delay);
          debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}buffer cap {} req {} -> {}",
                     "", level,
                     units_->capacitanceUnit()->asString(best_option->cap()),
                     delayAsString(best_req, this),
                     z->to_string(resizer_));
          buffered_options.push_back(z);
        }
      }
    }
    for (BufferedNetPtr z : buffered_options)
      Z1.push_back(z);
  }
  return Z1;
}

float
RepairSetup::bufferInputCapacitance(LibertyCell *buffer_cell,
                                    const DcalcAnalysisPt *dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  int lib_ap = dcalc_ap->libertyIndex();
  LibertyPort *corner_input = input->cornerPort(lib_ap);
  return corner_input->capacitance();
}

int
RepairSetup::rebufferTopDown(BufferedNetPtr choice,
                             Net *net,
                             int level)
{
  switch(choice->type()) {
  case BufferedNetType::buffer: {
    Instance *parent = db_network_->topInstance();
    string buffer_name = resizer_->makeUniqueInstName("rebuffer");
    Net *net2 = resizer_->makeUniqueNet();
    LibertyCell *buffer_cell = choice->bufferCell();
    Instance *buffer = resizer_->makeBuffer(buffer_cell,
                                            buffer_name.c_str(),
                                            parent,
                                            choice->location());
    resizer_->level_drvr_vertices_valid_ = false;
    LibertyPort *input, *output;
    buffer_cell->bufferPorts(input, output);
    debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}insert {} -> {} ({}) -> {}",
               "", level,
               sdc_network_->pathName(net),
               buffer_name.c_str(),
               buffer_cell->name(),
               sdc_network_->pathName(net2));
    sta_->connectPin(buffer, input, net);
    sta_->connectPin(buffer, output, net2);
    int buffer_count = rebufferTopDown(choice->ref(), net2, level + 1);
    resizer_->parasiticsInvalid(net);
    resizer_->parasiticsInvalid(net2);
    return buffer_count + 1;
  }
  case BufferedNetType::wire:
    debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}wire", "", level);
    return rebufferTopDown(choice->ref(), net, level + 1);
  case BufferedNetType::junction: {
    debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}junction", "", level);
    return rebufferTopDown(choice->ref(), net, level + 1)
      + rebufferTopDown(choice->ref2(), net, level + 1);
  }
  case BufferedNetType::load: {
    const Pin *load_pin = choice->loadPin();
    Net *load_net = network_->net(load_pin);
    if (load_net != net) {
      Instance *load_inst = db_network_->instance(load_pin);
      Port *load_port = db_network_->port(load_pin);
      debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}connect load {} to {}",
               "", level,
               sdc_network_->pathName(load_pin),
               sdc_network_->pathName(net));
      sta_->disconnectPin(const_cast<Pin*>(load_pin));
      sta_->connectPin(load_inst, load_port, net);
      resizer_->parasiticsInvalid(load_net);
    }
    return 0;
  }
  }
  return 0;
}

} // namespace
