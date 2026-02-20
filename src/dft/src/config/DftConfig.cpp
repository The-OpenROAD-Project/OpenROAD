// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "DftConfig.hh"

#include "ScanArchitectConfig.hh"
#include "ScanStitchConfig.hh"
#include "utl/Logger.h"

namespace dft {

ScanArchitectConfig* DftConfig::getMutableScanArchitectConfig()
{
  return &scan_architect_config_;
}

const ScanArchitectConfig& DftConfig::getScanArchitectConfig() const
{
  return scan_architect_config_;
}

ScanStitchConfig* DftConfig::getMutableScanStitchConfig()
{
  return &scan_stitch_config_;
}

const ScanStitchConfig& DftConfig::getScanStitchConfig() const
{
  return scan_stitch_config_;
}

void DftConfig::report(utl::Logger* logger) const
{
  logger->report("***************************");
  logger->report("DFT Config Report:\n");
  scan_architect_config_.report(logger);
  scan_stitch_config_.report(logger);
  logger->report("***************************");
}

}  // namespace dft
