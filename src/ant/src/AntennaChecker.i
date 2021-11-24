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
check_antennas(char* report_filename, bool report_violating_nets)
{
  return getAntennaChecker()->check_antennas(report_filename, report_violating_nets);
}

void
check_max_length(const char *net_name,
                 int layer)
{
  AntennaChecker *checker = getAntennaChecker();
  checker->check_max_length(net_name, layer);
}

void
load_antenna_rules()
{
  getAntennaChecker()->load_antenna_rules();
}

// check if an input net is violated, return 1 if the net is violated
//   - -net_name: set the net name for checking
bool
check_net_violation(char* net_name)
{ 
  odb::dbNet* net = ord::getDb()->getChip()->getBlock()->findNet(net_name);
  if (net) {
    auto vios = getAntennaChecker()->get_net_antenna_violations(net);
    return !vios.empty();
  }
  else
    return false;
}

// Prints the longest wire in the design
void
find_max_wire_length()
{
  getAntennaChecker()->find_max_wire_length();
}

}

%} // inline
