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
#pragma once

#include "utl/Logger.h"

namespace dft {

class ScanStitchConfig
{
 public:
  enum class EnableMode
  {
    Global = 0,
    PerChain
  };

  static const std::string_view EnableModeName(EnableMode enable_mode);

  void setEnableNamePattern(std::string_view& enable_name_pattern);
  const std::string& getEnableNamePattern() const;

  void setInNamePattern(std::string_view& in_name_pattern);
  const std::string& getInNamePattern() const;

  void setOutNamePattern(std::string_view& out_name_pattern);
  const std::string& getOutNamePattern() const;

  void setEnableMode(EnableMode enable_mode);
  EnableMode getEnableMode() const;

  // Prints using logger->report the config used by Scan Stitch
  void report(utl::Logger* logger) const;

 private:
  std::string enable_name_pattern_ = "scan_enable_{}";
  std::string in_name_pattern_ = "scan_in_{}";
  std::string out_name_pattern_ = "scan_out_{}";
  EnableMode enable_mode_ = EnableMode::Global;
};

}  // namespace dft
