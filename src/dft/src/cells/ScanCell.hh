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
#pragma once

#include <memory>
#include <string>

namespace dft {
class ClockDomain;

// A Scan Cell is a cell that contains a scan enable, scan in and scan out
// It also has the number of bits that can be shifted in this cell and a
// clock domain for architecting.
//
// This is an abstraction since there are several types of scan cells that can
// be use for DFT. The simplest one is just one scan cell but we could support
// in the future black boxes or CTLs (Core Test Language)
class ScanCell
{
 public:
  ScanCell(const std::string& name, std::unique_ptr<ClockDomain> clock_domain);
  virtual ~ScanCell() = default;
  // Not copyable or movable
  ScanCell(const ScanCell&) = delete;
  ScanCell& operator=(const ScanCell&) = delete;

  virtual uint64_t getBits() const = 0;
  virtual void connectScanEnable() const = 0;
  virtual void connectScanIn() const = 0;
  virtual void connectScanOut() const = 0;

  const ClockDomain& getClockDomain() const;
  const std::string& getName() const;

 private:
  std::string name_;
  std::unique_ptr<ClockDomain> clock_domain_;
};

}  // namespace dft
