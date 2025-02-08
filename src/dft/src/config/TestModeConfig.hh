///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2025, Google LLC
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
#pragma once

#include "ScanArchitectConfig.hh"
#include "ScanStitchConfig.hh"
#include "Macros.hh"

namespace dft {

// Holds the config of this particular test mode.
class TestModeConfig
{
 DISABLE_COPY_AND_MOVE(TestModeConfig);
 public:
  TestModeConfig(const std::string& name) : name_(name) {}

  ScanArchitectConfig* getMutableScanArchitectConfig();
  const ScanArchitectConfig& getScanArchitectConfig() const;

  ScanStitchConfig* getMutableScanStitchConfig();
  const ScanStitchConfig& getScanStitchConfig() const;

  void report(utl::Logger* logger) const;

  const std::string& getName() const;

  // Checks that the config is in a valid state. Reports issues in the logger
  void validate(utl::Logger* logger) const;

 private:
  std::string name_;
  ScanArchitectConfig scan_architect_config_;
  ScanStitchConfig scan_stitch_config_;
};

}  // namespace dft
