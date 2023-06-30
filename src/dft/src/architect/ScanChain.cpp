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

#include "ScanChain.hh"

#include "ClockDomain.hh"

namespace dft {

ScanChain::ScanChain(const std::string& name) : name_(name), bits_(0)
{
}

void ScanChain::add(std::unique_ptr<ScanCell> scan_cell)
{
  bits_ += scan_cell->getBits();
  switch (scan_cell->getClockDomain().getClockEdge()) {
    case ClockEdge::Rising:
      rising_edge_scan_cells_.push_back(std::move(scan_cell));
      break;
    case ClockEdge::Falling:
      falling_edge_scan_cells_.push_back(std::move(scan_cell));
      break;
  }
}

bool ScanChain::empty() const
{
  return scan_cells_.empty();
}

uint64_t ScanChain::getBits() const
{
  return bits_;
}

void ScanChain::sortScanCells(
    const std::function<void(std::vector<std::unique_ptr<ScanCell>>&,
                             std::vector<std::unique_ptr<ScanCell>>&,
                             std::vector<std::unique_ptr<ScanCell>>&)>& sort_fn)
{
  sort_fn(falling_edge_scan_cells_, rising_edge_scan_cells_, scan_cells_);
  // At this point, falling_edge_scan_cells_ and rising_edge_scan_cells_ will be
  // with just null, we clear and reduce the memory of the vectors

  falling_edge_scan_cells_.clear();
  rising_edge_scan_cells_.clear();

  falling_edge_scan_cells_.shrink_to_fit();
  rising_edge_scan_cells_.shrink_to_fit();
}

const std::vector<std::unique_ptr<ScanCell>>& ScanChain::getScanCells() const
{
  return scan_cells_;
}

void ScanChain::report(utl::Logger* logger, bool verbose) const
{
  logger->report("Scan chain '{:s}' has {:d} cells ({:d} bits)\n",
                 name_,
                 scan_cells_.size(),
                 bits_);

  if (!verbose) {
    return;
  }

  size_t current_clock_domain_id = 0;
  // First negative triggered cells
  for (const auto& scan_cell : scan_cells_) {
    if (current_clock_domain_id
        != scan_cell->getClockDomain().getClockDomainId()) {
      const ClockDomain& clock_domain = scan_cell->getClockDomain();
      // Change of clock domain, show the clock
      logger->report("  {:s} ({:s}, {:s})",
                     scan_cell->getName(),
                     clock_domain.getClockName(),
                     clock_domain.getClockEdgeName());
      current_clock_domain_id = scan_cell->getClockDomain().getClockDomainId();
    } else {
      logger->report("  {:s}", scan_cell->getName());
    }
  }
  logger->report("");  // line break between scan chains
}

const std::string& ScanChain::getName() const
{
  return name_;
}

}  // namespace dft
