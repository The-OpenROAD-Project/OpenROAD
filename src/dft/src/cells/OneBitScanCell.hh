// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "ScanCell.hh"
#include "ScanPin.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Liberty.hh"

namespace dft {

// A simple single cell with just one bit. Usually one scan FF
class OneBitScanCell : public ScanCell
{
 public:
  OneBitScanCell(const std::string& name,
                 std::unique_ptr<ClockDomain> clock_domain,
                 odb::dbInst* inst,
                 sta::TestCell* test_cell,
                 sta::dbNetwork* db_network,
                 utl::Logger* logger);
  // Not copyable or movable
  OneBitScanCell(const OneBitScanCell&) = delete;
  OneBitScanCell& operator=(const OneBitScanCell&) = delete;

  uint64_t getBits() const override;
  void connectScanEnable(const ScanDriver& driver) const override;
  void connectScanIn(const ScanDriver& driver) const override;
  void connectScanOut(const ScanLoad& load) const override;
  ScanLoad getScanEnable() const override;
  ScanLoad getScanIn() const override;
  ScanDriver getScanOut() const override;

  odb::Point getOrigin() const override;
  bool isPlaced() const override;

 private:
  odb::dbITerm* findITerm(sta::LibertyPort* liberty_port) const;

  odb::dbInst* inst_;
  sta::TestCell* test_cell_;
  sta::dbNetwork* db_network_;
};

}  // namespace dft
