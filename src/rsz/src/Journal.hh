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

using sta::dbSta;
using sta::PathExpanded;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::Instance;
using sta::Network;

class BufferedNet;
enum class BufferedNetType;
typedef std::shared_ptr<BufferedNet> BufferedNetPtr;
using BufferedNetSeq = vector<BufferedNetPtr>;


class Undo {
 public:
  // base class for undo elements
  Undo() {};
  virtual int UndoOperation(Logger *logger, Network *network, dbSta *sta) = 0;
};

class UndoBufferToInverter: public Undo {
 public:
  // buffer to inverter element
  UndoBufferToInverter(Instance *inv_buffer1, Instance *inv_buffer2,
                 Instance *inv_inverter1, Instance *inv_inverter2);
  int UndoOperation(Logger *logger, Network *network, dbSta *sta);
 private:
  // four instance pointers for inverter to buffer conversion
  Instance *inv_buffer1_;
  Instance *inv_buffer2_;
  Instance *inv_inverter1_;
  Instance *inv_inverter2_;
};

class UndoBuffer : public Undo {
 public:
  // swap pin element
  UndoBuffer(Instance* inst);
  int UndoOperation(Logger *logger, Network *network, dbSta *sta);
 private:
  Instance *buffer_inst_;
};

class UndoPinSwap : public Undo {
 public:
  // swap pin element
  UndoPinSwap(Instance* inst, LibertyPort* swap_port1,
              LibertyPort* swap_port2);
  int UndoOperation(Logger *logger,  Network *network, dbSta *sta);
 private:
  // Two pin pointers to undo pin swapping.
  Instance *swap_inst_;
  LibertyPort *swap_port1_;
  LibertyPort *swap_port2_;
};

class UndoResize : public Undo {
  public:
  // resize element
  UndoResize(Instance *inst, LibertyCell *cell);
  int UndoOperation(Logger *logger,  Network *network, dbSta *sta);
 private:
  // Original cell used for resize and the instance
  Instance *resized_inst_;
  LibertyCell *original_cell_;
};

class UndoClone
{
  public:
  // clone element
  UndoClone();
  int UndoOperation(Logger *logger, Network *network, dbSta *sta);
 private:
};

class Journal
{
public:
  Journal(Resizer *, Logger *logger);

  void begin();
  void end();
  void restore(int& resize_count, int& inserted_buffer_count, int& cloned_gate_count);
  void swapPins(Instance *inst, LibertyPort *port1, LibertyPort *port2);
  void instReplaceCellBefore(Instance *inst);
  void makeBuffer(Instance *buffer);

 private:
  sta::Network *network();
  dbSta *sta();

 private:
  Resizer *resizer_;
  Logger *logger_;
  std::stack<std::unique_ptr<Undo>> journal_stack_;
};

} // namespace rsz
