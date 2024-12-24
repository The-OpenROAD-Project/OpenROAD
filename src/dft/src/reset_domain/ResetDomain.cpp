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
#include "ResetDomain.hh"

namespace dft {

std::function<size_t(const ResetDomain&)> GetResetDomainHashFn()
{
  return [](const ResetDomain& reset_domain) {
    return std::hash<std::string_view>{}(reset_domain.getResetName())
           ^ std::hash<ResetActiveEdge>{}(reset_domain.getResetActiveEdge());
  };
}

ResetDomain::ResetDomain(const std::string& reset_name,
                         ResetActiveEdge reset_edge)
    : reset_name_(reset_name), reset_edge_(reset_edge)
{
}

std::string_view ResetDomain::getResetName() const
{
  return reset_name_;
}

ResetActiveEdge ResetDomain::getResetActiveEdge() const
{
  return reset_edge_;
}

std::string_view ResetDomain::getResetActiveEdgeName() const
{
  switch (reset_edge_) {
    case ResetActiveEdge::High:
      return "high";
      break;
    case ResetActiveEdge::Low:
      return "low";
      break;
  }
  // TODO replace with std::unreachable() once we reach c++23
  return "Unknown reset active edge";
}

size_t ResetDomain::getResetDomainId() const
{
  return std::hash<std::string_view>{}(reset_name_)
         ^ std::hash<ResetActiveEdge>{}(reset_edge_);
}

}  // namespace dft
