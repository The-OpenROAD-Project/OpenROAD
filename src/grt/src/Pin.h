/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
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

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "odb/db.h"

namespace grt {

class Net;

// For iterm pins this is the edge on a macro/pad cell that the pin lies on.
// For bterm pins this is the edge on block that the pin lies on.
// It is none for standard cells.
enum class PinEdge
{
  north,
  south,
  east,
  west,
  none
};

class Pin
{
 public:
  Pin(odb::dbITerm* iterm,
      const odb::Point& position,
      const std::vector<odb::dbTechLayer*>& layers,
      const std::map<odb::dbTechLayer*, std::vector<odb::Rect>>&
          boxes_per_layer,
      bool connected_to_pad_or_macro);
  Pin(odb::dbBTerm* bterm,
      const odb::Point& position,
      const std::vector<odb::dbTechLayer*>& layers,
      const std::map<odb::dbTechLayer*, std::vector<odb::Rect>>&
          boxes_per_layer,
      const odb::Point& die_center);

  odb::dbITerm* getITerm() const;
  odb::dbBTerm* getBTerm() const;
  std::string getName() const;
  const odb::Point& getPosition() const { return position_; }
  const std::vector<int>& getLayers() const { return layers_; }
  int getNumLayers() const { return layers_.size(); }
  int getConnectionLayer() const;
  PinEdge getEdge() const { return edge_; }
  const std::map<int, std::vector<odb::Rect>>& getBoxes() const
  {
    return boxes_per_layer_;
  }
  bool isPort() const { return is_port_; }
  bool isConnectedToPadOrMacro() const { return connected_to_pad_or_macro_; }
  const odb::Point& getOnGridPosition() const { return on_grid_position_; }
  void setOnGridPosition(odb::Point on_grid_pos)
  {
    on_grid_position_ = on_grid_pos;
  }
  bool isDriver();

 private:
  void determineEdge(const odb::Rect& bounds,
                     const std::vector<odb::dbTechLayer*>& layers);

  union
  {
    odb::dbITerm* iterm_;
    odb::dbBTerm* bterm_;
  };
  odb::Point position_;
  odb::Point on_grid_position_;
  std::vector<int> layers_;
  int connection_layer_;
  PinEdge edge_;
  std::map<int, std::vector<odb::Rect>> boxes_per_layer_;
  bool is_port_;
  bool connected_to_pad_or_macro_;
};

}  // namespace grt
