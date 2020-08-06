/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "Box.h"
#include "Coordinate.h"
#include "opendb/db.h"

namespace FastRoute {

class Net;

enum Orientation
{
  ORIENT_NORTH,
  ORIENT_SOUTH,
  ORIENT_EAST,
  ORIENT_WEST,
  INVALID
};

class Pin
{
 public:
  enum Type
  {
    SINK,
    SOURCE,
    OTHER
  };

  Pin() = default;
  Pin(odb::dbITerm* iterm,
      const Coordinate& position,
      const std::vector<int>& layers,
      const Orientation orientation,
      const std::map<int, std::vector<Box>>& boxesPerLayer,
      bool connectedToPad,
      Type type);
  Pin(odb::dbBTerm* bterm,
      const Coordinate& position,
      const std::vector<int>& layers,
      const Orientation orientation,
      const std::map<int, std::vector<Box>>& boxesPerLayer,
      bool connectedToPad,
      Type type);

  odb::dbITerm* getITerm() const;
  odb::dbBTerm* getBTerm() const;
  std::string getName() const;
  const Coordinate& getPosition() const { return _position; }
  const std::vector<int>& getLayers() const { return _layers; }
  int getNumLayers() const { return _layers.size(); }
  int getTopLayer() const { return _layers.back(); }
  Orientation getOrientation() const { return _orientation; }
  void setOrientation(Orientation orientation) { _orientation = orientation; }
  const std::map<int, std::vector<Box>>& getBoxes() const
  {
    return _boxesPerLayer;
  }
  bool isPort() const { return _isPort; }
  Type getType() const { return _type; }
  bool isConnectedToPad() const { return _connectedToPad; }
  const Coordinate& getOnGridPosition() const { return _onGridPosition; }
  void setOnGridPosition(Coordinate onGridPos) { _onGridPosition = onGridPos; }

 private:
  union
  {
    odb::dbITerm* _iterm;
    odb::dbBTerm* _bterm;
  };
  Coordinate _position;
  Coordinate _onGridPosition;
  std::vector<int> _layers;
  Orientation _orientation;
  std::map<int, std::vector<Box>> _boxesPerLayer;
  bool _isPort;
  Type _type;
  bool _connectedToPad;
};

}  // namespace FastRoute
