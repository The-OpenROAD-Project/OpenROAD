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

%module antennachecker

%{

#include "antennachecker/AntennaChecker.hh"
#include "openroad/OpenRoad.hh"

antenna_checker::AntennaChecker *
getAntennaChecker()
{
  return ord::OpenRoad::openRoad()->getAntennaChecker();
}

%}

%inline %{

void
antennachecker_set_verbose(bool verbose)
{
  getAntennaChecker()->set_verbose(verbose);
}

void
antennachecker_set_net_name(char * netname)
{
  std::string net_name = netname;
  getAntennaChecker()->set_net_name(net_name);
}

void
antennachecker_set_route_level(int rt_lv)
{
  getAntennaChecker()->set_route_level(rt_lv);
}

void
check_antennas()
{
  getAntennaChecker()->check_antennas();
}

void
get_met_avail_length()
{
  getAntennaChecker()->check_par_max_length();
}

void
load_antenna_rules()
{
  getAntennaChecker()->load_antenna_rules();
}

int
check_net_violation(char* netname)
{ 
  odb::dbNet* net = getAntennaChecker()->get_net( std::string(netname));
  std::vector<std::pair<int, std::vector<odb::dbITerm *>>> vios;
  vios = getAntennaChecker()->get_net_antenna_violations(net);
  if (vios.size() !=0)
    return 1;
  else
    return 0;       
}

%} // inline
