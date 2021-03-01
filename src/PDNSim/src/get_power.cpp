/*
BSD 3-Clause License

Copyright (c) 2020, The Regents of the University of Minnesota

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "get_power.h"
#include <iostream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"

namespace psm {
using sta::Corner;
using sta::dbNetwork;
using sta::Instance;
using sta::LeafInstanceIterator;
using sta::LibertyCell;
using sta::PowerResult;
using std::pair;
using std::string;
using std::vector;

//! Function for power per instance calculation
vector<pair<string, double>> PowerInst::executePowerPerInst(sta::dbSta* sta, utl::Logger*  logger)
{
  // STA object create
  m_sta = sta;
  m_logger = logger;
  // environment settings
  string cornerName = "wst";
  // string cornerNameFF="bst";

  //  StringSet cornerNameSet;
  //  cornerNameSet.insert(cornerName.c_str());
  //
  //  // define_corners
  //  _sta->makeCorners(&cornerNameSet);
  //  Corner* corner = _sta->findCorner(cornerName.c_str());
  Corner* corner = m_sta->cmdCorner();

  vector<pair<string, double>> power_report;
  dbNetwork*            network   = m_sta->getDbNetwork();
  LeafInstanceIterator* inst_iter = network->leafInstanceIterator();
  PowerResult           total_calc;
  total_calc.clear();
  while (inst_iter->hasNext()) {
    Instance* inst = inst_iter->next();
    LibertyCell* cell = network->libertyCell(inst);
    if (cell) {
      PowerResult inst_power;
      m_sta->power(inst, corner, inst_power);
      total_calc.incr(inst_power);
      power_report.push_back(
          make_pair(string(network->name(inst)), inst_power.total()));
          debugPrint(m_logger, utl::PSM, "get power", 2, "Power of instance {} is {}",string(network->name(inst)), inst_power.total());
    }
  }
  delete inst_iter;

  debugPrint(m_logger, utl::PSM, "get power", 1, "Total power: {}", total_calc.total());
  return power_report;
}
}  // namespace psm
