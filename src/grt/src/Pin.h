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

#include "opendb/db.h"

namespace grt {

class Net;

enum class PinOrientation
{
  north,
  south,
  east,
  west,
  invalid
};

class Pin
{
 public:
  Pin() = default;
  Pin(odb::dbITerm* iterm,
      const odb::Point& position,
      const std::vector<int>& layers,
      const PinOrientation orientation,
      const std::map<int, std::vector<odb::Rect>>& boxesPerLayer,
      bool connectedToPad);
  Pin(odb::dbBTerm* bterm,
      const odb::Point& position,
      const std::vector<int>& layers,
      const PinOrientation orientation,
      const std::map<int, std::vector<odb::Rect>>& boxesPerLayer,
      bool connectedToPad);

  odb::dbITerm* getITerm() const;
  odb::dbBTerm* getBTerm() const;
  std::string getName() const;
  const odb::Point& getPosition() const { return _position; }
  const std::vector<int>& getLayers() const { return _layers; }
  int getNumLayers() const { return _layers.size(); }
  int getTopLayer() const { return _layers.back(); }
  PinOrientation getOrientation() const { return _orientation; }
  void setOrientation(PinOrientation orientation)
  {
    _orientation = orientation;
  }
  const std::map<int, std::vector<odb::Rect>>& getBoxes() const
  {
    return _boxesPerLayer;
  }
  bool isPort() const { return _isPort; }
  bool isConnectedToPad() const { return _connectedToPad; }
  const odb::Point& getOnGridPosition() const { return _onGridPosition; }
  void setOnGridPosition(odb::Point onGridPos) { _onGridPosition = onGridPos; }
  bool isDriver();

 private:
  union
  {
    odb::dbITerm* _iterm;
    odb::dbBTerm* _bterm;
  };
  odb::Point _position;
  odb::Point _onGridPosition;
  std::vector<int> _layers;
  PinOrientation _orientation;
  std::map<int, std::vector<odb::Rect>> _boxesPerLayer;
  bool _isPort;
  bool _connectedToPad;
};

}  // namespace grt
