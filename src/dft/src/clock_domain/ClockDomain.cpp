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

#include "ClockDomain.hh"

#include <cstdint>

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
