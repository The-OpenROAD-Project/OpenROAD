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

#include "get_voltage.h"
#include <iostream>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Liberty.hh"

namespace psm {
std::pair<double, double> SupplyVoltage::getSupplyVoltage(sta::dbSta* sta)
{
  std::pair<double, double> supply_voltage;
  sta::LibertyLibrary*      default_library;
  _sta                                 = sta;
  sta::dbNetwork*             network  = _sta->getDbNetwork();
  sta::Corner*                corner   = _sta->cmdCorner();
  sta::MinMax*                mm       = sta::MinMax::max();
  const sta::DcalcAnalysisPt* dcalc_ap = corner->findDcalcAnalysisPt(mm);
  float                       power_voltage_;
  float                       gnd_voltage_;
  default_library = network->defaultLibertyLibrary();
  bool exists;

  const sta::Pvt* pvt = dcalc_ap->operatingConditions();
  if (pvt == nullptr)
    pvt = default_library->defaultOperatingConditions();
  if (pvt)
    power_voltage_ = pvt->voltage();
  else
    power_voltage_ = 0.0;
  gnd_voltage_ = 0.0;
  // default_library_->supplyVoltage(power_name_, power_voltage_, exists);
  // if (!exists) {
  //  DcalcAnalysisPt *dcalc_ap = path_->dcalcAnalysisPt(this);
  //  const OperatingConditions *op_cond = dcalc_ap->operatingConditions();
  //  if (op_cond == nullptr)
  //    op_cond =
  //    network->defaultLibertyLibrary()->defaultOperatingConditions();
  //  power_voltage_ = op_cond->voltage();
  //}
  // default_library_->supplyVoltage(gnd_name_, gnd_voltage_, exists);
  // if (!exists)
  //  gnd_voltage_ = 0.0;
  supply_voltage.first  = power_voltage_;
  supply_voltage.second = gnd_voltage_;
  return supply_voltage;
}
}  // namespace psm
