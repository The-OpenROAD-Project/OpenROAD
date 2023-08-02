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

#pragma once

#include "utl/Logger.h"
#include "db_sta/dbSta.hh"

#include "sta/StaState.hh"
#include "sta/MinMax.hh"
#include "sta/FuncExpr.hh"

namespace sta {
class PathExpanded;
}

namespace rsz {
class Resizer;
using std::vector;
using utl::Logger;
using sta::StaState;
using sta::dbSta;
using sta::dbNetwork;
using sta::Pin;
using sta::Net;
using sta::PathRef;
using sta::MinMax;
using sta::Slack;
using sta::PathExpanded;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::TimingArc;
using sta::DcalcAnalysisPt;
using sta::Vertex;
using sta::Corner;
using sta::Instance;
class BufferedNet;
enum class BufferedNetType;
typedef std::shared_ptr<BufferedNet> BufferedNetPtr;
using BufferedNetSeq = vector<BufferedNetPtr>;

enum class JournalElementType
{
  resize,
  clone,
  buffer,
  pin_swap,
  inverter_swap
};

class JournalElement
{
  JournalElementType type_;
  // Original cell used for resize and the instance
  Instance *resized_inst_;
  LibertyCell *original_cell_;
  // Two pin pointers to undo pin swapping.
  Pin *swap_pin1_;
  Pin *swap_pin2_;
  // four instance pointers for inverter to buffer conversion
  Instance *inv_buffer1_;
  Instance *inv_buffer2_;
  Instance *inv_inverter1_;
  Instance *inv_inverter2_;
};

class Journal
{
public:
  Journal();

  void journalBegin();
  void journalEnd();
  void journalSwapPins(Instance *inst, LibertyPort *port1, LibertyPort *port2);
  void journalInstReplaceCellBefore(Instance *inst);
  void journalMakeBuffer(Instance *buffer);
  void journalUndoGateCloning(int &cloned_gate_count);
  void journalRestore(int& resize_count, int& inserted_buffer_count,
                      int& cloned_gate_count);
};

} // namespace rsz
