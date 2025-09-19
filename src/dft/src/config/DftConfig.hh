// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include "ScanArchitectConfig.hh"
#include "ScanStitchConfig.hh"
#include "utl/Logger.h"

namespace dft {

// Main class that contains all the DFT configuration.
// Pass this object by reference to other functions
class DftConfig
{
 public:
  DftConfig() = default;
  // Not copyable or movable.
  DftConfig(const DftConfig&) = delete;
  DftConfig& operator=(const DftConfig&) = delete;

  ScanArchitectConfig* getMutableScanArchitectConfig();
  const ScanArchitectConfig& getScanArchitectConfig() const;

  ScanStitchConfig* getMutableScanStitchConfig();
  const ScanStitchConfig& getScanStitchConfig() const;

  // Prints the information currently being used by DFT for config
  void report(utl::Logger* logger) const;

 private:
  ScanArchitectConfig scan_architect_config_;
  ScanStitchConfig scan_stitch_config_;
};

}  // namespace dft
