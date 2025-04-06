// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ClockDomain.hh"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace dft {

ClockDomain::ClockDomain(const std::string& clock_name, ClockEdge clock_edge)
    : clock_name_(clock_name), clock_edge_(clock_edge)
{
}

std::string_view ClockDomain::getClockName() const
{
  return clock_name_;
}

ClockEdge ClockDomain::getClockEdge() const
{
  return clock_edge_;
}

std::string_view ClockDomain::getClockEdgeName() const
{
  switch (clock_edge_) {
    case ClockEdge::Rising:
      return "rising";
      break;
    case ClockEdge::Falling:
      return "falling";
      break;
  }
  // TODO replace with std::unreachable() once we reach c++23
  return "Unknown clock edge";
}

namespace {
size_t FNV1a(const uint8_t* data, size_t length)
{
  size_t hash = 0xcbf29ce484222325;
  for (size_t i = 0; i < length; i += 1) {
    hash ^= data[i];
    hash *= 0x00000100000001b3;
  }
  return hash;
}
};  // namespace

size_t ClockDomain::getClockDomainId() const
{
  return FNV1a((uint8_t*) clock_name_.data(), clock_name_.length())
         ^ FNV1a((uint8_t*) &clock_edge_, sizeof(ClockEdge));
}

}  // namespace dft
