// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "OneBitScanCell.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "ClockDomain.hh"
#include "ScanCell.hh"
#include "ScanPin.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Liberty.hh"

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

ScanLoad OneBitScanCell::getScanEnable() const
{
  return ScanLoad(findITerm(getLibertyScanEnable(test_cell_)));
}

ScanDriver OneBitScanCell::getScanOut() const
{
  return ScanDriver(findITerm(getLibertyScanOut(test_cell_)));
}

ScanLoad OneBitScanCell::getScanIn() const
{
  return ScanLoad(findITerm(getLibertyScanIn(test_cell_)));
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
