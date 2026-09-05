// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <variant>

#include "ScanCell.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {

// Lightweight ScanCell adapter for scan_opt.
//
// Wraps a dbScanInst so it can be passed to OptimizeScanWirelength().
// Only the methods needed by the optimizer are implemented; connect methods
// are stubs because restitching is done directly on the odb nets afterwards.
class OdbScanCellAdapter : public ScanCell
{
 public:
  OdbScanCellAdapter(odb::dbScanInst* scan_inst, utl::Logger* logger);

  uint64_t getBits() const override;

  // Connect stubs — not used during optimisation.
  void connectScanEnable(const ScanDriver&) const override {}
  void connectScanIn(const ScanDriver&) const override {}
  void connectScanOut(const ScanLoad&) const override {}
  ScanLoad getScanEnable() const override;
  ScanLoad getScanIn() const override;
  ScanDriver getScanOut() const override;

  odb::Point getOrigin() const override;
  bool isPlaced() const override;

  odb::Point getScanInLocation() const override;
  odb::Point getScanOutLocation() const override;

  odb::dbITerm* getScanInITerm() const;
  odb::dbITerm* getScanOutITerm() const;

 private:
  odb::dbInst* inst_;
  uint64_t bits_;
  odb::dbITerm* scan_in_iterm_;
  odb::dbITerm* scan_out_iterm_;
};

}  // namespace dft
