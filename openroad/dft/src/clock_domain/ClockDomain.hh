// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

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

}  // namespace dft
