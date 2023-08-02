/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
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
#include "rsz/Resizer.hh"

#include "utl/Logger.h"
#include "db_sta/dbNetwork.hh"

#include "sta/Units.hh"
#include "sta/Liberty.hh"
#include "sta/TimingArc.hh"
#include "sta/Graph.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Parasitics.hh"
#include "sta/Sdc.hh"
#include "sta/InputDrive.hh"
#include "sta/Corner.hh"
#include "sta/PathVertex.hh"
#include "sta/PathRef.hh"
#include "sta/PathExpanded.hh"
#include "sta/Fuzzy.hh"
#include "sta/PortDirection.hh"

namespace rsz {

using std::abs;
using std::min;
using std::max;
using std::string;
using std::vector;
using std::map;
using std::pair;

using utl::RSZ;

using sta::VertexOutEdgeIterator;
using sta::Edge;
using sta::Clock;
using sta::PathExpanded;
using sta::INF;
using sta::fuzzyEqual;
using sta::fuzzyLess;
using sta::fuzzyLessEqual;
using sta::fuzzyGreater;
using sta::fuzzyGreaterEqual;
using sta::Unit;
using sta::Corners;
using sta::InputDrive;


Journal::Journal()
{

}

void
Journal::journalBegin()
{
  /*
  debugPrint(logger_, RSZ, "journal", 1, "journal begin");
  resized_inst_map_.clear();
  inserted_buffers_.clear();
  inserted_buffer_set_.clear();
  cloned_gates_ = {};
  cloned_inst_set_.clear();
  swapped_pins_.clear();
   */
}

void
Journal::journalEnd()
{
  /*
  debugPrint(logger_, RSZ, "journal", 1, "journal end");
  resized_inst_map_.clear();
  inserted_buffers_.clear();
  inserted_buffer_set_.clear();
  cloned_gates_ = {};
  cloned_inst_set_.clear();
  swapped_pins_.clear();
   */
}

void
Journal::journalSwapPins(Instance *inst, LibertyPort *port1, LibertyPort *port2)
{
  /*
  debugPrint(logger_, RSZ, "journal", 1, "journal swap pins {} ({}->{})",
             network_->pathName(inst),port1->name(), port2->name());
  swapped_pins_[inst] = std::make_tuple(port1, port2);
   */
}

void
Journal::journalInstReplaceCellBefore(Instance *inst)
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

void
Journal::journalMakeBuffer(Instance *buffer)
{
  /*
  debugPrint(logger_, RSZ, "journal", 1, "journal make_buffer {}",
             network_->pathName(buffer));
  inserted_buffers_.push_back(buffer);
  inserted_buffer_set_.insert(buffer);
   */
}

void
Journal::journalUndoGateCloning(int &cloned_gate_count)
{
  /*
  // Undo gate cloning
  while (!cloned_gates_.empty()) {
    auto element = cloned_gates_.top();
    cloned_gates_.pop();
    auto original_inst = std::get<0>(element);
    auto cloned_inst = std::get<1>(element);
    debugPrint(logger_, RSZ, "journal", 1, "journal unclone {} ({}) -> {} ({})",
               network_->pathName(original_inst),
               network_->libertyCell(original_inst)->name(),
               network_->pathName(cloned_inst),
               network_->libertyCell(cloned_inst)->name());

    const Pin* original_output_pin = nullptr;
    std::vector<const Pin*> clone_pins = getPins(cloned_inst);
    std::vector<const Pin*> original_pins = getPins(original_inst);
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
    --cloned_gate_count;
  }
  cloned_inst_set_.clear();
   */
}

void
Journal::journalRestore(int &resize_count,
                        int &inserted_buffer_count,
                        int &cloned_gate_count)
{

  /*
  for (auto [inst, lib_cell] : resized_inst_map_) {
    if (!inserted_buffer_set_.hasKey(inst)) {
      debugPrint(logger_, RSZ, "journal", 1, "journal restore {} ({})",
                 network_->pathName(inst),
                 lib_cell->name());
      // skip if it is a cloned cell
      if (cloned_inst_set_.find(inst) != cloned_inst_set_.end()) {
        debugPrint(logger_, RSZ, "journal", 1, "journal skip cloned {} ({})",
                   network_->pathName(inst),
                   lib_cell->name());
        continue;
      }
      debugPrint(logger_, RSZ, "journal", 1, "journal replace {} ({})",
                 network_->pathName(inst), lib_cell->name());
      replaceCell(inst, lib_cell, false);
      resize_count--;
    }
  }
  inserted_buffer_set_.clear();

  while (!inserted_buffers_.empty()) {
    const Instance *buffer = inserted_buffers_.back();
    debugPrint(logger_, RSZ, "journal", 1, "journal remove buffer {}",
               network_->pathName(buffer));
    removeBuffer(const_cast<Instance*>(buffer));
    inserted_buffers_.pop_back();
    inserted_buffer_count--;
  }

  // Undo pin swaps
  for (const auto& element : swapped_pins_) {
    Instance *inst = element.first;
    LibertyPort *port1 = std::get<0>(element.second);
    LibertyPort *port2 = std::get<1>(element.second);
    debugPrint(logger_, RSZ, "journal", 1, "journal unswap pins {} ({}<-{})",
               network_->pathName(inst),port1->name(), port2->name());
    swapPins(inst, port2, port1, false);
  }
  swapped_pins_.clear();

  journalUndoGateCloning(cloned_gate_count);
   */
}

}  // namespace rsz
