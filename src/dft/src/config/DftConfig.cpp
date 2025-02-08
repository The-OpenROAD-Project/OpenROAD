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

#include "DftConfig.hh"

#include <iostream>

namespace dft {

namespace {
// Internal scan is the scan mode where each scan chain is connected to a top
// level scan in , scan enable and scan out.
constexpr std::string_view kDefaultTestModeName = "internal_scan";
}  // namespace

void DftConfig::report(utl::Logger* logger) const
{
  logger->report("***************************");
  logger->report("DFT Config Report:\n");

  for (const auto& [test_mode, test_mode_config] : test_modes_config_) {
    logger->report("\nTest mode: {}", test_mode);
    test_mode_config.report(logger);
  }

  logger->report("***************************");
}

TestModeConfig* DftConfig::createTestMode(const std::string& name,
                                          utl::Logger* logger)
{
  if (test_modes_config_.size() >= 1) {
    logger->error(utl::DFT, 43, "Multiple test modes not supported");
    return nullptr;
  }

  auto pair = test_modes_config_.try_emplace(name, name);
  return &pair.first->second;
}

TestModeConfig* DftConfig::getOrDefaultMutableTestModeConfig(
    const std::string& name,
    utl::Logger* logger)
{
  auto found = test_modes_config_.find(name);
  if (found == test_modes_config_.end()) {
    if (name == kDefaultTestModeName) {
      return createTestMode(name, logger);
    }
    return nullptr;
  }
  return &found->second;
}

const std::unordered_map<std::string, TestModeConfig>&
DftConfig::getTestModesConfig() const
{
  return test_modes_config_;
}

void DftConfig::validate(utl::Logger* logger) const
{
  for (const auto& [test_mode_name, test_mode_config] : test_modes_config_) {
    test_mode_config.validate(logger);
  }
}

}  // namespace dft
