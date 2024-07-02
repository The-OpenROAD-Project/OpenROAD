// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
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

%{

#include "ant/AntennaChecker.hh"
#include "ord/OpenRoad.hh"

ant::AntennaChecker *
getAntennaChecker()
{
  return ord::OpenRoad::openRoad()->getAntennaChecker();
}

namespace ord {
// Defined in OpenRoad.i
odb::dbDatabase *getDb();
}

%}

%include "../../Exception.i"

%inline %{

namespace ant {

int
check_antennas(const char *net_name, bool verbose)
{
  auto app = ord::OpenRoad::openRoad();
  auto block = app->getDb()->getChip()->getBlock();
  odb::dbNet* net = nullptr;
  if (strlen(net_name) > 0) {
    net = block->findNet(net_name);
    if (!net) {
      auto logger = app->getLogger();
      logger->error(utl::ANT, 12, "Net {} not found.", net_name);
    }
  }
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  return getAntennaChecker()->checkAntennas(net, num_threads, verbose);
}

int
antenna_violation_count()
{
  return getAntennaChecker()->antennaViolationCount();
}

// check a net for antenna violations
bool
check_net_violation(char* net_name)
{ 
  odb::dbNet* net = ord::getDb()->getChip()->getBlock()->findNet(net_name);
  if (net) {
    auto vios = getAntennaChecker()->getAntennaViolations(net, nullptr, 0);
    return !vios.empty();
  }
  else
    return false;
}

void
set_report_file_name(char* file_name)
{
  getAntennaChecker()->setReportFileName(file_name);
}

} // namespace

%} // inline
