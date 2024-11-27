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

#include "BufferedNet.hh"
#include "RepairSetup.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PortDirection.hh"
#include "sta/Units.hh"

namespace rsz {

using std::make_shared;

using utl::RSZ;

using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::fuzzyLessEqual;
using sta::INF;
using sta::NetConnectedPinIterator;
using sta::PinSeq;
using sta::Port;

// Return inserted buffer count.
int RepairSetup::rebuffer(const Pin* drvr_pin)
{
  int inserted_buffer_count = 0;
  Net* net;

  Instance* parent
      = db_network_->getOwningInstanceParent(const_cast<Pin*>(drvr_pin));
  odb::dbNet* db_net = nullptr;
  odb::dbModNet* db_modnet = nullptr;

  if (network_->isTopLevelPort(drvr_pin)) {
    net = network_->net(network_->term(drvr_pin));
    LibertyCell* buffer_cell = resizer_->buffer_lowest_drive_;
    // Should use sdc external driver here.
    LibertyPort* input;
    buffer_cell->bufferPorts(input, drvr_port_);
  } else {
    db_network_->net(drvr_pin, db_net, db_modnet);

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
      if (debug) {
        logger_->setDebugLevel(RSZ, "rebuffer", 3);
      }
      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 2,
                 "driver {}",
                 sdc_network_->pathName(drvr_pin));
      sta_->findRequireds();
      const BufferedNetSeq& Z = rebufferBottomUp(bnet, 1);
      Required best_slack_penalized = -INF;
      BufferedNetPtr best_option = nullptr;
      int best_index = 0;
      int i = 1;
      for (const BufferedNetPtr& p : Z) {
        // Find slack for drvr_pin into option.
        const PathRef& req_path = p->requiredPath();
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

        //
        // get the modnet driver
        //
        odb::dbITerm* drvr_op_iterm = nullptr;
        odb::dbBTerm* drvr_op_bterm = nullptr;
        odb::dbModITerm* drvr_op_moditerm = nullptr;
        odb::dbModBTerm* drvr_op_modbterm = nullptr;
        db_network_->staToDb(drvr_pin,
                             drvr_op_iterm,
                             drvr_op_bterm,
                             drvr_op_moditerm,
                             drvr_op_modbterm);

        if (db_net && db_modnet) {
          // as we move the modnet and dbnet around we will get a clash
          //(the dbNet name now exposed is the same as the modnet name)
          // so we uniquify the modnet name
          std::string new_name = resizer_->makeUniqueNetName();
          db_modnet->rename(new_name.c_str());
        }

        inserted_buffer_count = rebufferTopDown(best_option,
                                                db_network_->dbToSta(db_net),
                                                1,
                                                parent,
                                                drvr_op_iterm,
                                                db_modnet);
        if (inserted_buffer_count > 0) {
          rebuffer_net_count_++;
          debugPrint(logger_,
                     RSZ,
                     "rebuffer",
                     2,
                     "rebuffer {} inserted {}",
                     network_->pathName(drvr_pin),
                     inserted_buffer_count);
        }
      }
      if (debug) {
        logger_->setDebugLevel(RSZ, "rebuffer", 0);
      }
    } else {
      logger_->warn(RSZ,
                    75,
                    "makeBufferedNet failed for driver {}",
                    network_->pathName(drvr_pin));
    }
  }
  return inserted_buffer_count;
}

Slack RepairSetup::slackPenalized(const BufferedNetPtr& bnet)
{
  return slackPenalized(bnet, -1);
}

Slack RepairSetup::slackPenalized(const BufferedNetPtr& bnet,
                                  // Only used for debug print.
                                  int index)
{
  const PathRef& req_path = bnet->requiredPath();
  if (!req_path.isNull()) {
    Delay drvr_delay = resizer_->gateDelay(drvr_port_,
                                           req_path.transition(sta_),
                                           bnet->cap(),
                                           req_path.dcalcAnalysisPt(sta_));
    Slack slack = bnet->required(sta_) - drvr_delay;
    int buffer_count = bnet->bufferCount();
    double buffer_penalty = buffer_count * rebuffer_buffer_penalty_;
    double slack_penalized
        = slack * (1.0 - (slack > 0 ? buffer_penalty : -buffer_penalty));
    if (index >= 0) {
      debugPrint(
          logger_,
          RSZ,
          "rebuffer",
          2,
          "option {:3d}: {:2d} buffers req {} - {} = {} * {:3.2f} = {} cap {}",
          index,
          bnet->bufferCount(),
          delayAsString(bnet->required(sta_), this, 3),
          delayAsString(drvr_delay, this, 3),
          delayAsString(slack, this, 3),
          buffer_penalty,
          delayAsString(slack_penalized, this, 3),
          units_->capacitanceUnit()->asString(bnet->cap()));
    }
    return slack_penalized;
  }
  return INF;
}

// For testing.
void RepairSetup::rebufferNet(const Pin* drvr_pin)
{
  init();
  resizer_->incrementalParasiticsBegin();
  inserted_buffer_count_ = rebuffer(drvr_pin);
  // Leave the parasitics up to date.
  resizer_->updateParasitics();
  resizer_->incrementalParasiticsEnd();
  logger_->report("Inserted {} buffers.", inserted_buffer_count_);
}

bool RepairSetup::hasTopLevelOutputPort(Net* net)
{
  NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();
    if (network_->isTopLevelPort(pin) && network_->direction(pin)->isOutput()) {
      delete pin_iter;
      return true;
    }
  }
  delete pin_iter;
  return false;
}

BufferedNetSeq RepairSetup::rebufferBottomUp(const BufferedNetPtr& bnet,
                                             const int level)
{
  switch (bnet->type()) {
    case BufferedNetType::wire: {
      BufferedNetSeq Z = rebufferBottomUp(bnet->ref(), level + 1);
      return addWireAndBuffer(Z, bnet, level);
    }
    case BufferedNetType::junction: {
      const BufferedNetSeq& Z1 = rebufferBottomUp(bnet->ref(), level + 1);
      const BufferedNetSeq& Z2 = rebufferBottomUp(bnet->ref2(), level + 1);
      BufferedNetSeq Z;

      size_t size = Z1.size() * Z2.size();
      // This assumption is used only in the final loop
      // but we can quit as early as we know there's nothing to do here.
      if (size == 0) {
        return Z;
      }
      Z.reserve(size);

      std::unordered_map<BufferedNet*, Slack> slacks;

      // Combine the options from both branches.=
      for (const BufferedNetPtr& p : Z1) {
        for (const BufferedNetPtr& q : Z2) {
          const BufferedNetPtr& min_req
              = fuzzyLess(p->required(sta_), q->required(sta_)) ? p : q;
          BufferedNetPtr junc = make_shared<BufferedNet>(
              BufferedNetType::junction, bnet->location(), p, q, resizer_);
          junc->setCapacitance(p->cap() + q->cap());
          junc->setRequiredPath(min_req->requiredPath());
          junc->setRequiredDelay(min_req->requiredDelay());
          slacks[junc.get()] = slackPenalized(junc);
          Z.push_back(std::move(junc));
        }
      }
      // Prune the options if there exists another option with
      // larger required and smaller capacitance.
      // This is fanout*log(fanout) if options are
      // presorted to hit better options sooner.
      sort(Z.begin(),
           Z.end(),
           [&slacks](const BufferedNetPtr& option1,
                     const BufferedNetPtr& option2) {
             const Slack slack1 = slacks[option1.get()];
             const Slack slack2 = slacks[option2.get()];

             if (slack1 != slack2) {
               return slack1 > slack2;
             }

             return option1->cap() < option2->cap();
           });
      float Lsmall = Z[0]->cap();
      size_t si = 1;
      // Remove options by shifting down with index si.
      // Because the options are sorted we don't have to look
      // beyond the first option. We also know that slack
      // is nonincreasing, so we can remove everything that has
      // higher capacitance than the lowest found so far.
      for (size_t pi = si; pi < size; pi++) {
        const BufferedNetPtr& p = Z[pi];
        float Lp = p->cap();
        // If Lp is the same or worse than Lsmall, remove solution p.
        if (fuzzyLess(Lp, Lsmall)) {
          // Otherwise copy the survivor down.
          Z[si++] = p;
          Lsmall = Lp;
        }
      }
      Z.resize(si);
      return Z;
    }
    case BufferedNetType::load: {
      const Pin* load_pin = bnet->loadPin();
      Vertex* vertex = graph_->pinLoadVertex(load_pin);
      PathRef req_path = sta_->vertexWorstSlackPath(vertex, max_);
      const DcalcAnalysisPt* dcalc_ap = req_path.isNull()
                                            ? resizer_->tgt_slew_dcalc_ap_
                                            : req_path.dcalcAnalysisPt(sta_);
      bnet->setCapacitance(resizer_->pinCapacitance(load_pin, dcalc_ap));
      bnet->setRequiredPath(req_path);
      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 4,
                 "{:{}s}{}",
                 "",
                 level,
                 bnet->to_string(resizer_));
      BufferedNetSeq Z;
      Z.push_back(bnet);
      return Z;
    }
    case BufferedNetType::buffer:
      logger_->critical(RSZ, 71, "unhandled BufferedNet type");
  }
  return BufferedNetSeq();
}

BufferedNetSeq RepairSetup::addWireAndBuffer(const BufferedNetSeq& Z,
                                             const BufferedNetPtr& bnet_wire,
                                             int level)
{
  BufferedNetSeq Z1;
  Z1.reserve(Z.size());
  Point wire_end = bnet_wire->location();
  for (const BufferedNetPtr& p : Z) {
    Point p_loc = p->location();
    int wire_length_dbu
        = abs(wire_end.x() - p_loc.x()) + abs(wire_end.y() - p_loc.y());
    double wire_length = resizer_->dbuToMeters(wire_length_dbu);
    const PathRef& req_path = p->requiredPath();
    const Corner* corner = req_path.isNull()
                               ? sta_->cmdCorner()
                               : req_path.dcalcAnalysisPt(sta_)->corner();
    int wire_layer = bnet_wire->layer();
    double layer_res, layer_cap;
    bnet_wire->wireRC(corner, resizer_, layer_res, layer_cap);
    double wire_res = wire_length * layer_res;
    double wire_cap = wire_length * layer_cap;
    double wire_delay = wire_res * wire_cap;
    BufferedNetPtr z = make_shared<BufferedNet>(
        BufferedNetType::wire, wire_end, wire_layer, p, corner, resizer_);
    // account for wire load
    z->setCapacitance(p->cap() + wire_cap);
    z->setRequiredPath(req_path);
    // account for wire delay
    z->setRequiredDelay(p->requiredDelay() + wire_delay);
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               4,
               "{:{}s}wire wl {} {}",
               "",
               level,
               wire_length_dbu,
               z->to_string(resizer_));
    Z1.push_back(z);
  }
  if (!Z1.empty()) {
    BufferedNetSeq buffered_options;
    for (LibertyCell* buffer_cell : resizer_->buffer_cells_) {
      Required best_req = -INF;
      BufferedNetPtr best_option = nullptr;
      for (const BufferedNetPtr& z : Z1) {
        PathRef req_path = z->requiredPath();
        // Do not buffer unconstrained paths.
        if (!req_path.isNull()) {
          const DcalcAnalysisPt* dcalc_ap = req_path.dcalcAnalysisPt(sta_);
          Delay buffer_delay = resizer_->bufferDelay(
              buffer_cell, req_path.transition(sta_), z->cap(), dcalc_ap);
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
          const DcalcAnalysisPt* dcalc_ap = req_path.dcalcAnalysisPt(sta_);
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
        for (const BufferedNetPtr& buffer_option : buffered_options) {
          if (fuzzyLessEqual(buffer_option->cap(), buffer_cap)
              && fuzzyGreaterEqual(buffer_option->required(sta_), required)) {
            prune = true;
            break;
          }
        }
        if (!prune) {
          BufferedNetPtr z = make_shared<BufferedNet>(
              BufferedNetType::buffer,
              // Locate buffer at opposite end of wire.
              wire_end,
              buffer_cell,
              best_option,
              corner_,
              resizer_);
          z->setCapacitance(buffer_cap);
          z->setRequiredPath(req_path);
          z->setRequiredDelay(best_option->requiredDelay() + buffer_delay);
          debugPrint(logger_,
                     RSZ,
                     "rebuffer",
                     3,
                     "{:{}s}buffer cap {} req {} -> {}",
                     "",
                     level,
                     units_->capacitanceUnit()->asString(best_option->cap()),
                     delayAsString(best_req, this),
                     z->to_string(resizer_));
          buffered_options.push_back(z);
        }
      }
    }
    for (const BufferedNetPtr& z : buffered_options) {
      Z1.push_back(z);
    }
  }
  return Z1;
}

float RepairSetup::bufferInputCapacitance(LibertyCell* buffer_cell,
                                          const DcalcAnalysisPt* dcalc_ap)
{
  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  int lib_ap = dcalc_ap->libertyIndex();
  LibertyPort* corner_input = input->cornerPort(lib_ap);
  return corner_input->capacitance();
}

int RepairSetup::rebufferTopDown(const BufferedNetPtr& choice,
                                 Net* net,  // output of buffer.
                                 int level,
                                 Instance* parent_in,
                                 odb::dbITerm* mod_net_drvr,
                                 odb::dbModNet* mod_net_in)
{
  // HFix, pass in the parent
  Instance* parent = parent_in;
  switch (choice->type()) {
    case BufferedNetType::buffer: {
      std::string buffer_name = resizer_->makeUniqueInstName("rebuffer");

      // HFix: make net in hierarchy
      std::string net_name = resizer_->makeUniqueNetName();
      Net* net2 = db_network_->makeNet(net_name.c_str(), parent);

      LibertyCell* buffer_cell = choice->bufferCell();
      Instance* buffer = resizer_->makeBuffer(
          buffer_cell, buffer_name.c_str(), parent, choice->location());

      resizer_->level_drvr_vertices_valid_ = false;
      LibertyPort *input, *output;
      buffer_cell->bufferPorts(input, output);
      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 3,
                 "{:{}s}insert {} -> {} ({}) -> {}",
                 "",
                 level,
                 sdc_network_->pathName(net),
                 buffer_name.c_str(),
                 buffer_cell->name(),
                 sdc_network_->pathName(net2));

      odb::dbNet* db_ip_net = nullptr;
      odb::dbModNet* db_ip_modnet = nullptr;
      db_network_->staToDb(net, db_ip_net, db_ip_modnet);

      //      sta_->connectPin(buffer, input, net);  //rebuffer
      sta_->connectPin(
          buffer, input, db_network_->dbToSta(db_ip_net));  // rebuffer
      sta_->connectPin(buffer, output, net2);

      Pin* buffer_ip_pin = nullptr;
      Pin* buffer_op_pin = nullptr;

      resizer_->getBufferPins(buffer, buffer_ip_pin, buffer_op_pin);
      odb::dbITerm* buffer_op_iterm = nullptr;
      odb::dbBTerm* buffer_op_bterm = nullptr;
      odb::dbModITerm* buffer_op_moditerm = nullptr;
      odb::dbModBTerm* buffer_op_modbterm = nullptr;
      db_network_->staToDb(buffer_op_pin,
                           buffer_op_iterm,
                           buffer_op_bterm,
                           buffer_op_moditerm,
                           buffer_op_modbterm);

      // disconnect modnet from original driver
      // connect the output to the modnet.
      // inch the modnet to the end of the buffer chain created in this scope

      // Hierarchy handling
      if (mod_net_drvr && mod_net_in) {
        // save original dbnet
        dbNet* orig_db_net = mod_net_drvr->getNet();
        // disconnect everything
        mod_net_drvr->disconnect();
        // restore dbnet
        mod_net_drvr->connect(orig_db_net);
        // add the modnet to the new output
        buffer_op_iterm->connect(mod_net_in);
      }

      int buffer_count = rebufferTopDown(
          choice->ref(), net2, level + 1, parent, buffer_op_iterm, mod_net_in);

      // ip_net
      odb::dbNet* db_net = nullptr;
      odb::dbModNet* db_modnet = nullptr;
      db_network_->staToDb(net, db_net, db_modnet);
      resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));

      db_network_->staToDb(net2, db_net, db_modnet);
      resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));
      return buffer_count + 1;
    }

    case BufferedNetType::wire:
      debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}wire", "", level);
      return rebufferTopDown(
          choice->ref(), net, level + 1, parent, mod_net_drvr, mod_net_in);

    case BufferedNetType::junction: {
      debugPrint(logger_, RSZ, "rebuffer", 3, "{:{}s}junction", "", level);
      return rebufferTopDown(choice->ref(),
                             net,
                             level + 1,
                             parent,
                             mod_net_drvr,
                             mod_net_in)
             + rebufferTopDown(choice->ref2(),
                               net,
                               level + 1,
                               parent,
                               mod_net_drvr,
                               mod_net_in);
    }

    case BufferedNetType::load: {
      odb::dbNet* db_net = nullptr;
      odb::dbModNet* db_modnet = nullptr;
      db_network_->staToDb(net, db_net, db_modnet);

      const Pin* load_pin = choice->loadPin();

      // only access at dbnet level
      Net* load_net = network_->net(load_pin);

      dbNet* db_load_net;
      odb::dbModNet* db_mod_load_net;
      db_network_->staToDb(load_net, db_load_net, db_mod_load_net);
      (void) db_load_net;

      if (load_net != net) {
        odb::dbITerm* load_iterm = nullptr;
        odb::dbBTerm* load_bterm = nullptr;
        odb::dbModITerm* load_moditerm = nullptr;
        odb::dbModBTerm* load_modbterm = nullptr;
        db_network_->staToDb(
            load_pin, load_iterm, load_bterm, load_moditerm, load_modbterm);

        debugPrint(logger_,
                   RSZ,
                   "rebuffer",
                   3,
                   "{:{}s}connect load {} to {}",
                   "",
                   level,
                   sdc_network_->pathName(load_pin),
                   sdc_network_->pathName(load_net));

        // disconnect removes everything.
        sta_->disconnectPin(const_cast<Pin*>(load_pin));
        // prepare for hierarchy
        load_iterm->connect(db_net);
        // preserve the mod net.
        if (db_mod_load_net) {
          load_iterm->connect(db_mod_load_net);
        }

        // sta_->connectPin(load_inst, load_port, net);
        resizer_->parasiticsInvalid(db_network_->dbToSta(db_net));
        // resizer_->parasiticsInvalid(load_net);
      }
      return 0;
    }
  }
  return 0;
}

}  // namespace rsz
