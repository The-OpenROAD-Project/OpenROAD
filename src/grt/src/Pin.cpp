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

#include "Pin.h"

#include "grt/GlobalRouter.h"

namespace grt {

Pin::Pin(odb::dbITerm* iterm,
         const odb::Point& position,
         const std::vector<int>& layers,
         const PinOrientation orientation,
         const std::map<int, std::vector<odb::Rect>>& boxesPerLayer,
         bool connectedToPad)
    : _iterm(iterm),
      _position(position),
      _layers(layers),
      _orientation(orientation),
      _boxesPerLayer(boxesPerLayer),
      _isPort(false),
      _connectedToPad(connectedToPad)
{
  std::sort(_layers.begin(), _layers.end());
}

Pin::Pin(odb::dbBTerm* bterm,
         const odb::Point& position,
         const std::vector<int>& layers,
         const PinOrientation orientation,
         const std::map<int, std::vector<odb::Rect>>& boxesPerLayer,
         bool connectedToPad)
    : _bterm(bterm),
      _position(position),
      _layers(layers),
      _orientation(orientation),
      _boxesPerLayer(boxesPerLayer),
      _isPort(true),
      _connectedToPad(connectedToPad)
{
  std::sort(_layers.begin(), _layers.end());
}

odb::dbITerm* Pin::getITerm() const
{
  if (_isPort)
    return nullptr;
  else
    return _iterm;
}

odb::dbBTerm* Pin::getBTerm() const
{
  if (_isPort)
    return _bterm;
  else
    return nullptr;
}

std::string Pin::getName() const
{
  if (_isPort)
    return _bterm->getName();
  else
    return getITermName(_iterm);
}

}  // namespace grt
