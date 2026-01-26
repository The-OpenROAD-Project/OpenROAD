// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <string_view>

#include "utl/Logger.h"

namespace dft {

class ScanStitchConfig
{
 public:
  void setEnableNamePattern(std::string_view enable_name_pattern);
  std::string_view getEnableNamePattern() const;

  void setInNamePattern(std::string_view in_name_pattern);
  std::string_view getInNamePattern() const;

  void setOutNamePattern(std::string_view out_name_pattern);
  std::string_view getOutNamePattern() const;

  // Prints using logger->report the config used by Scan Stitch
  void report(utl::Logger* logger) const;

 private:
  std::string enable_name_pattern_ = "scan_enable_{}";
  std::string in_name_pattern_ = "scan_in_{}";
  std::string out_name_pattern_ = "scan_out_{}";
};

}  // namespace dft
