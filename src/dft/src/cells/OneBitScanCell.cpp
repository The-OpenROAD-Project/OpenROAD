// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "OneBitScanCell.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "ClockDomain.hh"
#include "ScanCell.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Liberty.hh"

namespace dft {

OneBitScanCell::OneBitScanCell(const std::string& name,
                               std::unique_ptr<ClockDomain> clock_domain,
                               odb::dbInst* inst,
                               sta::LibertyPort* scan_in_port,
                               sta::LibertyPort* scan_enable_port,
                               sta::LibertyPort* scan_out_port,
                               sta::dbNetwork* db_network,
                               utl::Logger* logger)
    : ScanCell(name, std::move(clock_domain), logger),
      inst_(inst),
      scan_in_port_(scan_in_port),
      scan_enable_port_(scan_enable_port),
      scan_out_port_(scan_out_port),
      db_network_(db_network)
{
}

uint64_t OneBitScanCell::getBits() const
{
  return 1;
}

void OneBitScanCell::connectScanEnable(const ScanDriver& driver) const
{
  Connect(ScanLoad(findITerm(scan_enable_port_)),
          driver,
          /*preserve=*/false);
}

void OneBitScanCell::connectScanIn(const ScanDriver& driver) const
{
  Connect(ScanLoad(findITerm(scan_in_port_)),
          driver,
          /*preserve=*/false);
}

void OneBitScanCell::connectScanOut(const ScanLoad& load) const
{
  // The scan out usually will be connected to functional data paths already, we
  // need to preserve the connections
  Connect(load,
          ScanDriver(findITerm(scan_out_port_)),
          /*preserve=*/true);
}

ScanLoad OneBitScanCell::getScanEnable() const
{
  return ScanLoad(findITerm(scan_enable_port_));
}

ScanDriver OneBitScanCell::getScanOut() const
{
  return ScanDriver(findITerm(scan_out_port_));
}

ScanLoad OneBitScanCell::getScanIn() const
{
  return ScanLoad(findITerm(scan_in_port_));
}

odb::dbITerm* OneBitScanCell::findITerm(sta::LibertyPort* liberty_port) const
{
  if (liberty_port == nullptr) {
    logger_->error(
        utl::DFT, 52, "Null Liberty port for scan cell '{}'", inst_->getName());
  }

  // Prefer name-based lookup on the instance. This is more robust than relying
  // on LibertyPort::extPort() -> dbMTerm mapping, and avoids crashes when that
  // mapping is missing or stale after cell replacement.
  const char* port_name = liberty_port->name();
  if (port_name != nullptr) {
    if (odb::dbITerm* iterm = inst_->findITerm(port_name)) {
      return iterm;
    }
  }

  odb::dbMTerm* mterm = db_network_->staToDb(liberty_port);
  if (mterm != nullptr) {
    if (odb::dbITerm* iterm = inst_->getITerm(mterm)) {
      return iterm;
    }
  }

  logger_->error(utl::DFT,
                 53,
                 "Failed to resolve scan ITerm '{}' on instance '{}'",
                 port_name ? port_name : "<null>",
                 inst_->getName());
  return nullptr;
}

odb::Point OneBitScanCell::getOrigin() const
{
  // Use the instance placement location (DEF "PLACED" coordinates). Using
  // dbInst::getOrigin() makes the coordinate depend on orientation (e.g. MX/MY)
  // which breaks scan-chain cost comparisons and can mislead the optimizer.
  return inst_->getLocation();
}

bool OneBitScanCell::isPlaced() const
{
  return inst_->isPlaced();
}

}  // namespace dft
