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
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathRef.hh"
#include "sta/PathVertex.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;
using sta::Instance;
using utl::RSZ;

//============================================================================
// Buffer insertion
UndoBuffer::UndoBuffer(Instance* inst)
{
  // Original cell used for resize and the instance
  buffer_inst_ = inst;
}

int UndoBuffer::UndoOperation(Resizer *resizer)
{
  // Remove the inserted buffer
  resizer->removeBuffer(buffer_inst_);
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

int UndoPinSwap::UndoOperation(Resizer *resizer)
{
  // Swap the pins back and do not journal (since it is the undo operation)
  resizer->swapPins(swap_inst_, swap_port1_, swap_port2_, false);
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

int UndoBufferToInverter::UndoOperation(Resizer *resizer)
{
  // TODO: Need to implement
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

int UndoResize::UndoOperation(Resizer *resizer)
{
    // Resize the instance back to the original cell
    resizer->replaceCell(resized_inst_, original_cell_, false);
    return 0;
}
//============================================================================
// clone element
UndoClone::UndoClone(Instance *original_inst, Instance *cloned_inst)
{
    original_inst_ = original_inst;
    cloned_inst_ = cloned_inst;
}

int UndoClone::UndoOperation(Resizer *resizer)
{
  resizer->undoGateCloning(original_inst_, cloned_inst_);
  return 1;
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
  // TODO We need to do something with cloned instances here
  journal_stack_ = {};
}

void Journal::swapPins(Instance* inst, LibertyPort* port1,
                              LibertyPort* port2)
{
  debugPrint(logger_, RSZ, "journal", 1, "journal swap pins {} ({}->{})",
             network()->pathName(inst), port1->name(), port2->name());
  std::unique_ptr<Undo> element = std::make_unique<UndoPinSwap>(inst, port1, port2);
  journal_stack_.emplace(std::move(element));
}


Instance *Journal::cloneInstance(LibertyCell *cell, const char *name,  Instance *original_inst,
                                 Instance *parent,  const Point& loc)
 {
   Instance *clone_inst = resizer_->makeInstance(cell, name, parent, loc);
   debugPrint(logger_, RSZ, "journal", 1, "journal clone {} ({})",
              network()->pathName(original_inst), cell->name());
   std::unique_ptr<Undo> element(new UndoClone(original_inst, clone_inst));
   journal_stack_.emplace(std::move(element));
   return clone_inst;
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

void Journal::restore()
{
  while (!journal_stack_.empty()) {
    Undo *element = journal_stack_.top().get();
    element->UndoOperation(resizer_);
    journal_stack_.pop();
  }
}

void Journal::reportStatistics()
{
  // TODO:
  if (inserted_buffer_count_ > 0 && split_load_buffer_count_ == 0) {
    logger_->info(RSZ, 59, "Inserted {} buffers.", inserted_buffer_count_);
  }
  else if (inserted_buffer_count_ > 0 && split_load_buffer_count_ > 0) {
    logger_->info(RSZ, 87, "Inserted {} buffers, {} to split loads.",
                  inserted_buffer_count_, split_load_buffer_count_);
  }
  logger_->metric("design__instance__count__setup_buffer", inserted_buffer_count_);
  if (resize_count_ > 0) {
    logger_->info(RSZ, 91, "Resized {} instances.", resize_count_);
  }
  if (swap_pin_count_ > 0) {
    logger_->info(RSZ, 96, "Swapped pins on {} instances.", swap_pin_count_);
  }
  if (cloned_gate_count_ > 0) {
    logger_->info(RSZ, 97, "Cloned {} instances.", cloned_gate_count_);
  }
  }

sta::Network * Journal::network()
{
  return resizer_->network_;
}

}  // namespace rsz
