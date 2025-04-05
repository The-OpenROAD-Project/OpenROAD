// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ScanStitchConfig.hh"

#include <string_view>

#include "Formatting.hh"

namespace dft {

void ScanStitchConfig::setEnableNamePattern(
    std::string_view enable_name_pattern)
{
  enable_name_pattern_ = enable_name_pattern;
}
std::string_view ScanStitchConfig::getEnableNamePattern() const
{
  return enable_name_pattern_;
};

void ScanStitchConfig::setInNamePattern(std::string_view in_name_pattern)
{
  in_name_pattern_ = in_name_pattern;
};
std::string_view ScanStitchConfig::getInNamePattern() const
{
  return in_name_pattern_;
};

void ScanStitchConfig::setOutNamePattern(std::string_view out_name_pattern)
{
  out_name_pattern_ = out_name_pattern;
};
std::string_view ScanStitchConfig::getOutNamePattern() const
{
  return out_name_pattern_;
};

void ScanStitchConfig::report(utl::Logger* logger) const
{
  logger->report("Scan Stitch Config:");
  logger->report("- Scan Enable Name Pattern: '{}'", enable_name_pattern_);
  logger->report("- Scan In Name Pattern: '{}'", in_name_pattern_);
  logger->report("- Scan Out Name Pattern: '{}'", out_name_pattern_);
}

}  // namespace dft
