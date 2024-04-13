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
#include <memory>

#include "odb/geom.h"
#include "spdlog/fmt/fmt.h"
#include "sta/Delay.hh"
#include "sta/Network.hh"
#include "sta/PathRef.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using std::array;
using std::string;

using utl::Logger;

using odb::Point;

using sta::Corner;
using sta::DcalcAnalysisPt;
using sta::Delay;
using sta::LibertyCell;
using sta::Network;
using sta::PathRef;
using sta::Pin;
using sta::Required;
using sta::RiseFall;
using sta::StaState;
using sta::Unit;
using sta::Units;

class Resizer;

class BufferedNet;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using Requireds = array<Required, RiseFall::index_count>;

enum class BufferedNetType
{
  load,
  junction,
  wire,
  buffer
};

// The routing tree is represented as a binary tree with the sinks being the
// leaves of the tree, the junctions being the Steiner nodes and the root being
// the source of the net.
class BufferedNet
{
 public:
  // load
  BufferedNet(BufferedNetType type,
              const Point& location,
              const Pin* load_pin,
              const Corner* corner,
              const Resizer* resizer);
  // wire
  BufferedNet(BufferedNetType type,
              const Point& location,
              int layer,
              const BufferedNetPtr& ref,
              const Corner* corner,
              const Resizer* resizer);
  // junc
  BufferedNet(BufferedNetType type,
              const Point& location,
              const BufferedNetPtr& ref,
              const BufferedNetPtr& ref2,
              const Resizer* resizer);
  // buffer
  BufferedNet(BufferedNetType type,
              const Point& location,
              LibertyCell* buffer_cell,
              const BufferedNetPtr& ref,
              const Corner* corner,
              const Resizer* resizer);
  string to_string(const Resizer* resizer) const;
  void reportTree(const Resizer* resizer) const;
  void reportTree(int level, const Resizer* resizer) const;
  BufferedNetType type() const { return type_; }
  // junction steiner point location connecting ref/ref2
  // wire     wire is from loc to location(ref_)
  // buffer   buffer driver pin location
  // load     load pin location
  Point location() const { return location_; }
  float cap() const { return cap_; }
  void setCapacitance(float cap);
  float fanout() const { return fanout_; }
  void setFanout(float fanout);
  float maxLoadSlew() const { return max_load_slew_; }
  void setMaxLoadSlew(float max_slew);
  // load
  const Pin* loadPin() const { return load_pin_; }
  // wire
  int length() const;
  // routing level
  int layer() const { return layer_; }
  void wireRC(const Corner* corner,
              const Resizer* resizer,
              // Return values.
              double& res,
              double& cap);
  // buffer
  LibertyCell* bufferCell() const { return buffer_cell_; }
  // junction  left
  // buffer    wire
  // wire      end of wire
  BufferedNetPtr ref() const { return ref_; }
  // junction  right
  BufferedNetPtr ref2() const { return ref2_; }

  // repairNet
  int maxLoadWireLength() const;

  // Rebuffer
  Required required(const StaState* sta) const;
  const PathRef& requiredPath() const { return required_path_; }
  void setRequiredPath(const PathRef& path_ref);
  Delay requiredDelay() const { return required_delay_; }
  void setRequiredDelay(Delay delay);
  // Downstream buffer count.
  int bufferCount() const;

  static constexpr int null_layer = -1;

 private:
  BufferedNetType type_;
  Point location_;
  // only used by load type
  const Pin* load_pin_;
  // only used by buffer type
  LibertyCell* buffer_cell_;
  // only used by wire type
  int layer_;
  // only used by buffer, wire, and junc types
  BufferedNetPtr ref_;
  // only used by junc type
  BufferedNetPtr ref2_;

  // Capacitance looking downstream from here.
  float cap_;
  float fanout_;
  float max_load_slew_;

  // Rebuffer annotations
  // PathRef for worst required path at load.
  PathRef required_path_;
  // Max delay from here to the loads.
  Delay required_delay_;
};

}  // namespace rsz
