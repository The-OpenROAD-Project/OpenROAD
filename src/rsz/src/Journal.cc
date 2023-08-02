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

// swap pin element
JournalElement::JournalElement(Instance* inst, LibertyPort* swap_port1,
                               LibertyPort* swap_port2)
{
  type_ = rsz::JournalElementType::pin_swap;
  swap_inst_ = inst;
  swap_port1_ = swap_port1;
  swap_port2_ = swap_port2;
}

// buffer to inverter element
JournalElement::JournalElement(Instance* inv_buffer1, Instance* inv_buffer2,
                               Instance* inv_inverter1, Instance* inv_inverter2)
{
  type_ = rsz::JournalElementType::inverter_swap;
  inv_buffer1_ = inv_buffer1;
  inv_buffer2_ = inv_buffer2;
  inv_inverter1_ = inv_inverter1;
  inv_inverter2_ = inv_inverter2;
}

// resize element
JournalElement::JournalElement(Instance* inst, LibertyCell* cell)
{
  type_ = rsz::JournalElementType::resize;
  // Original cell used for resize and the instance
  resized_inst_ = inst;
  original_cell_ = cell;
}

// clone element
JournalElement::JournalElement()
{
  type_ = rsz::JournalElementType::clone;
}

Journal::Journal(Logger* logger, Network* network, dbSta* sta)
{
  logger_ = logger;
  network_ = network;
  sta_ = sta;
}

void Journal::journalBegin()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  journal_stack_ = {};
}

void Journal::journalEnd()
{
  debugPrint(logger_, RSZ, "journal", 1, "journal end");
  // We need to do something with cloned instances here
  journal_stack_ = {};
}

void Journal::journalSwapPins(Instance* inst, LibertyPort* port1,
                              LibertyPort* port2)
{
  debugPrint(logger_, RSZ, "journal", 1, "journal swap pins {} ({}->{})",
             network_->pathName(inst), port1->name(), port2->name());
  journal_stack_.emplace(JournalElement(inst, port1, port2));
}

void Journal::journalInstReplaceCellBefore(Instance* inst)
{
  /*
  LibertyCell *lib_cell = network_->libertyCell(inst);
  debugPrint(logger_, RSZ, "journal", 1, "journal replace {} ({})",
             network_->pathName(inst),
             lib_cell->name());
  // Do not clobber an existing checkpoint cell.
  if (!resized_inst_map_.hasKey(inst))
    resized_inst_map_[inst] = lib_cell;
    */
}

void Journal::journalMakeBuffer(Instance* buffer)
{
  /*
  debugPrint(logger_, RSZ, "journal", 1, "journal make_buffer {}",
             network_->pathName(buffer));
  inserted_buffers_.push_back(buffer);
  inserted_buffer_set_.insert(buffer);
   */
}

void Journal::journalUndoGateCloning(JournalElement& item)
{
  // Undo gate cloning
  Instance *original_inst = nullptr; // TODO std::get<0>(element);
  Instance *cloned_inst = nullptr; // TODO std::get<1>(element);
  debugPrint(logger_, RSZ, "journal", 1, "journal unclone {} ({}) -> {} ({})",
             network_->pathName(original_inst),
             network_->libertyCell(original_inst)->name(),
             network_->pathName(cloned_inst),
             network_->libertyCell(cloned_inst)->name());

  const Pin* original_output_pin = nullptr;
  std::vector<const Pin*> clone_pins; // TODO getPins(cloned_inst);
  std::vector<const Pin*> original_pins; // TODO getPins(original_inst);
  for (auto& pin : original_pins) {
    if (network_->direction(pin)->isOutput()) {
      original_output_pin = pin;
      break;
    }
  }
  Net* original_out_net = network_->net(original_output_pin);
  // Net* clone_out_net = nullptr;

  for (auto& pin : clone_pins) {
    // Disconnect all pins from the new net. Also store the output net
    // if (network_->direction(pin)->isOutput()) {
    //  clone_out_net = network_->net(pin);
    //}
    sta_->disconnectPin(const_cast<Pin*>(pin));
    // Connect them to the original nets if they are inputs
    if (network_->direction(pin)->isInput()) {
      Instance* inst = network_->instance(pin);
      auto term_port = network_->port(pin);
      sta_->connectPin(inst, term_port, original_out_net);
    }
  }
  // Final cleanup
  // sta_->deleteNet(clone_out_net);
  sta_->deleteInstance(cloned_inst);
  sta_->graphDelayCalc()->delaysInvalid();
  // TODO --cloned_gate_count;

}

void Journal::journalRestore(int& resize_count, int& inserted_buffer_count,
                             int& cloned_gate_count)
{
  while (!journal_stack_.empty()) {
    auto element = journal_stack_.top();
    journal_stack_.pop();
    switch (element.getType()) {
      case rsz::JournalElementType::resize: break;
      case rsz::JournalElementType::clone:break;
      case rsz::JournalElementType::buffer:break;
      case rsz::JournalElementType::pin_swap:break;
      case rsz::JournalElementType::inverter_swap:break;
      default: break;// TODO error
    }
  }
}

}  // namespace rsz
