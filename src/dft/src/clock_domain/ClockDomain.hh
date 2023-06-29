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

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ScanArchitectConfig.hh"

namespace dft {

enum class ClockEdge
{
  Rising,  // positive edge triggered
  Falling  // negative edge triggered
};

// The clock domain of the scan cells based on the clock name and the edge.
class ClockDomain
{
 public:
  ClockDomain(const std::string& clock_name, ClockEdge clock_edge);
  // Allow move, copy is implicitly deleted
  ClockDomain(ClockDomain&& other) = default;
  ClockDomain& operator=(ClockDomain&& other) = default;

  std::string_view getClockName() const;
  ClockEdge getClockEdge() const;
  std::string_view getClockEdgeName() const;

  // Returns a unique id that can be use to identify a particular clock domain
  size_t getClockDomainId() const;

 private:
  std::string clock_name_;
  ClockEdge clock_edge_;
};

// Depending on the ScanArchitectConfig's clock mixing setting, there are
// different ways to calculate the hash of the clock domain.
//
// For No Mix clock, we will generate a different hash value for all the clock
// domains.
//
// If we want to mix all the clocks, then the hash will be the same for all the
// clock doamins.
//
// We refer to the generated hash from a ClockDomain as Hash Domain.
//
std::function<size_t(const ClockDomain&)> GetClockDomainHashFn(
    const ScanArchitectConfig& config,
    utl::Logger* logger);

}  // namespace dft
