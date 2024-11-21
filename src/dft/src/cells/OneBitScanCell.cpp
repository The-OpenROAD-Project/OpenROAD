///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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

#include "OneBitScanCell.hh"

#include "ClockDomain.hh"
#include "db_sta/dbSta.hh"

namespace dft {

OneBitScanCell::OneBitScanCell(const std::string& name,
                               std::unique_ptr<ClockDomain> clock_domain,
                               odb::dbInst* inst,
                               sta::TestCell* test_cell,
                               sta::dbNetwork* db_network,
                               utl::Logger* logger)
    : ScanCell(name, std::move(clock_domain), logger),
      inst_(inst),
      test_cell_(test_cell),
      db_network_(db_network)
{
}

uint64_t OneBitScanCell::getBits() const
{
  return 1;
}

void OneBitScanCell::connectScanEnable(const ScanDriver& driver) const
{
  Connect(ScanLoad(findITerm(getLibertyScanEnable(test_cell_))),
          driver,
          /*preserve=*/false);
}

void OneBitScanCell::connectScanIn(const ScanDriver& driver) const
{
  Connect(ScanLoad(findITerm(getLibertyScanIn(test_cell_))),
          driver,
          /*preserve=*/false);
}

void OneBitScanCell::connectScanOut(const ScanLoad& load) const
{
  // The scan out usually will be connected to functional data paths already, we
  // need to preserve the connections
  Connect(load,
          ScanDriver(findITerm(getLibertyScanOut(test_cell_))),
          /*preserve=*/true);
}

ScanDriver OneBitScanCell::getScanOut() const
{
  return ScanDriver(findITerm(getLibertyScanOut(test_cell_)));
}

odb::dbITerm* OneBitScanCell::findITerm(sta::LibertyPort* liberty_port) const
{
  odb::dbMTerm* mterm = db_network_->staToDb(liberty_port);
  return inst_->getITerm(mterm);
}

odb::Point OneBitScanCell::getOrigin() const
{
  return inst_->getOrigin();
}

bool OneBitScanCell::isPlaced() const
{
  return inst_->isPlaced();
}

}  // namespace dft
