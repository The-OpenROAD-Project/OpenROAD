/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, Precision Innovations Inc.
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

#include "Journal.hh"

#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PathVertex.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using std::abs;
using std::map;
using std::max;
using std::min;
using std::pair;
using std::string;
using std::vector;

using utl::RSZ;

using sta::Clock;
using sta::Corners;
using sta::Edge;
using sta::fuzzyEqual;
using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::fuzzyLess;
using sta::fuzzyLessEqual;
using sta::INF;
using sta::InputDrive;
using sta::PathExpanded;
using sta::Unit;
using sta::VertexOutEdgeIterator;


//============================================================================
// Buffer insertion
UndoBuffer::UndoBuffer(Instance* inst)
{
  // Original cell used for resize and the instance
  buffer_inst_ = inst;
}

int UndoBuffer::UndoOperation(Logger *logger, Network *network, dbSta *sta)
{
  // Resize the instance back to the original cell
  // resized_inst_->setMaster(original_cell_);
  return 0;
}
//============================================================================
// swap pin element
UndoPinSwap::UndoPinSwap(Instance* inst, LibertyPort* swap_port1, LibertyPort* swap_port2)
{
  swap_inst_ = inst;
  swap_port1_ = swap_port1;
  swap_port2_ = swap_port2;
}

int UndoPinSwap::UndoOperation(Logger *logger, Network *network, dbSta *sta)
{
  // Swap the pins back
  // swap_inst_->swapPins(swap_port1_, swap_port2_);
  return 0;
}
//============================================================================
// buffer to inverter element
UndoBufferToInverter::UndoBufferToInverter(Instance* inv_buffer1, Instance* inv_buffer2, Instance* inv_inverter1,
                                           Instance* inv_inverter2)
{
  inv_buffer1_ = inv_buffer1;
  inv_buffer2_ = inv_buffer2;
  inv_inverter1_ = inv_inverter1;
  inv_inverter2_ = inv_inverter2;
}

int UndoBufferToInverter::UndoOperation(utl::Logger* logger, sta::Network* network, sta::dbSta* sta)
{
 return 0;
}
//============================================================================
// resize element
UndoResize::UndoResize(Instance* inst, LibertyCell* cell)
{
  // Original cell used for resize and the instance
  resized_inst_ = inst;
  original_cell_ = cell;
}

int UndoResize::UndoOperation(Logger *logger,  Network *network, dbSta *sta)
{
    // Resize the instance back to the original cell
    // resized_inst_->setMaster(original_cell_);
    return 0;
}
//============================================================================
// clone element
UndoClone::UndoClone()
{
}

int UndoClone::UndoOperation(Logger *logger, Network *network, dbSta *sta)
{
    // Undo gate cloning
    Instance *original_inst = nullptr; // TODO std::get<0>(element);
    Instance *cloned_inst = nullptr; // TODO std::get<1>(element);
    debugPrint(logger, RSZ, "journal", 1, "journal unclone {} ({}) -> {} ({})", network->pathName(original_inst),
               network->libertyCell(original_inst)->name(), network->pathName(cloned_inst),
               network->libertyCell(cloned_inst)->name());

    const Pin* original_output_pin = nullptr;
    std::vector<const Pin*> clone_pins; // TODO getPins(cloned_inst);
    std::vector<const Pin*> original_pins; // TODO getPins(original_inst);
    for (auto& pin : original_pins) {
      if (network->direction(pin)->isOutput()) {
        original_output_pin = pin;
        break;
      }
    }
    Net* original_out_net = network->net(original_output_pin);
    // Net* clone_out_net = nullptr;

    for (auto& pin : clone_pins) {
      // Disconnect all pins from the new net. Also store the output net
      // if (network_->direction(pin)->isOutput()) {
      //  clone_out_net = network_->net(pin);
      //}
      sta->disconnectPin(const_cast<Pin*>(pin));
      // Connect them to the original nets if they are inputs
      if (network->direction(pin)->isInput()) {
        Instance* inst = network->instance(pin);
        auto term_port = network->port(pin);
        sta->connectPin(inst, term_port, original_out_net);
      }
    }
    // Final cleanup
    // sta_->deleteNet(clone_out_net);
    sta->deleteInstance(cloned_inst);
    sta->graphDelayCalc()->delaysInvalid();
    // TODO --cloned_gate_count;
    return 0;
}

//============================================================================
Journal::Journal(Resizer *resizer, Logger* logger)
{
    resizer_ = resizer;
    logger_ = logger;
}

void Journal::begin()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  journal_stack_ = {};
}

void Journal::end()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal end");
  // We need to do something with cloned instances here
  journal_stack_ = {};
}

void Journal::swapPins(Instance* inst, LibertyPort* port1,
                              LibertyPort* port2)
{
  debugPrint(logger_, RSZ, "journal", 1, "journal swap pins {} ({}->{})",
             network()->pathName(inst), port1->name(), port2->name());
  std::unique_ptr<Undo> element(new UndoPinSwap(inst, port1, port2));
  journal_stack_.emplace(std::move(element));
}

void Journal::instReplaceCellBefore(Instance* inst)
{
  LibertyCell *lib_cell = network()->libertyCell(inst);
  debugPrint(logger_, RSZ, "journal", 1, "journal replace {} ({})",
             network()->pathName(inst),
             lib_cell->name());
   std::unique_ptr<Undo> element(new UndoResize(inst, lib_cell));
   journal_stack_.emplace(std::move(element));
}

void Journal::makeBuffer(Instance* buffer)
{
  debugPrint(logger_, RSZ, "journal", 1, "journal make_buffer {}",
             network()->pathName(buffer));
  std::unique_ptr<Undo> element(new UndoBuffer(buffer));
  journal_stack_.emplace(std::move(element));
}

void Journal::restore(int& resize_count, int& inserted_buffer_count,
                             int& cloned_gate_count)
{
  while (!journal_stack_.empty()) {
    Undo *element = journal_stack_.top().get();
    element->UndoOperation(logger_, network(), sta());
    journal_stack_.pop();
  }
}

sta::Network * Journal::network()
{
  return resizer_->network_;
}

dbSta *Journal::sta()
{
  return resizer_->sta_;
}


}  // namespace rsz
