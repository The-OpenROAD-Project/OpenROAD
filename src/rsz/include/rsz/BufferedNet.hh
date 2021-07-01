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

#pragma once

#include <array>

#include "utl/Logger.h"
#include "opendb/geom.h"

#include "sta/Transition.hh"
#include "sta/Network.hh"
#include "sta/Delay.hh"
#include "sta/PathRef.hh"

namespace rsz {

using std::array;

using utl::Logger;

using odb::Point;

using sta::LibertyCell;
using sta::Pin;
using sta::Required;
using sta::RiseFall;
using sta::Network;
using sta::Unit;
using sta::Units;
using sta::PathRef;
using sta::Delay;
using sta::StaState;
using sta::DcalcAnalysisPt;

typedef array<Required, RiseFall::index_count> Requireds;

class Resizer;
enum class BufferedNetType { load, junction, wire, buffer };

// The routing tree is represented a binary tree with the sinks being the leaves
// of the tree, the junctions being the Steiner nodes and the root being the
// source of the net.
class BufferedNet
{
public:
  BufferedNet(BufferedNetType type,
              Point location,
              float cap,
              Pin *load_pin,
              PathRef load_req_path,
              Delay required_delay,
              LibertyCell *buffer,
              BufferedNet *ref,
              BufferedNet *ref2);
  BufferedNet(BufferedNetType type,
              Point location,
              float cap,
              Pin *load_pin,
              BufferedNet *ref,
              BufferedNet *ref2);
  ~BufferedNet();
  void report(int level,
              Resizer *resizer);
  void reportTree(Resizer *resizer);
  void reportTree(int level,
                  Resizer *resizer);
  BufferedNetType type() const { return type_; }
  float cap() const { return cap_; }
  Required required(StaState *sta);
  const PathRef &requiredPath() const { return required_path_; }
  Delay requiredDelay() const { return required_delay_; }
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
  // PathRef for worst required path at load.
  PathRef required_path_;
  // Delay from this BufferedNet to the load.
  Delay required_delay_;
  // Type load.
  Pin *load_pin_;
  // Type buffer.
  LibertyCell *buffer_cell_;
  BufferedNet *ref_;
  BufferedNet *ref2_;
};

} // namespace
