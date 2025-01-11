///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, Efabless Corporation
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

#include "ScanStitchConfig.hh"

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
