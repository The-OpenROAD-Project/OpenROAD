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

#include "Net.h"

namespace grt {

Net::Net(odb::dbNet* net) : _net(net)
{
}

const std::string Net::getName() const
{
  return _net->getName();
}

const char* Net::getConstName() const
{
  return _net->getConstName();
}

odb::dbSigType Net::getSignalType() const
{
  return _net->getSigType().getString();
}

void Net::addPin(Pin& pin)
{
  _pins.push_back(pin);
}

bool Net::isLocal()
{
  odb::Point position = _pins[0].getOnGridPosition();
  for (Pin& pin : _pins) {
    odb::Point pinPos = pin.getOnGridPosition();
    if (pinPos != position) {
      return false;
    }
  }

  return true;
}

void Net::destroyPins()
{
  _pins.clear();
}

}  // namespace grt
