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

#include "StaEngine.h"

#include "db_sta/dbSta.hh"
#include "openroad/OpenRoad.hh"
#include "utl/Logger.h"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "db_sta/dbNetwork.hh"
#include "sta/Sdc.hh"

#include <tcl.h>
#include <cassert>
#include <iostream>
#include <sstream>

namespace cts {

void StaEngine::init()
{
  ord::OpenRoad* openRoad = ord::OpenRoad::openRoad();
  _openSta = openRoad->getSta();
  _sdc = _openSta->sdc();
  _network = _openSta->network();
}

void StaEngine::findClockRoots(utl::Logger* _logger)
{
  _logger->report(" Looking for clock sources...");

  std::string clockNames = "";
  for (sta::Clock* clk : _sdc->clks()) {
    for (sta::Pin* pin : clk->leafPins()) {
      clockNames += std::string(_network->name(pin)) + " ";
    }
  }

  _logger->report("    Clock names: {}", clockNames);
  _options->setClockNets(clockNames);
}

float StaEngine::getInputPinCap(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* masterCell = ord::OpenRoad::openRoad()->getDbNetwork()->dbToSta(inst->getMaster());
  sta::LibertyCell* libertyCell = _network->libertyCell(masterCell);
  if (!libertyCell) {
    return 0.0;
  }

  sta::LibertyLibrary* staLib = libertyCell->libertyLibrary();
  sta::LibertyPort *inputPort = libertyCell->findLibertyPort(iterm->getMTerm()->getConstName());
  if (inputPort) {
    return inputPort->capacitance();
  } else {
    return 0.0;
  }
    
}
}  // namespace cts
