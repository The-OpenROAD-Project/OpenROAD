// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanChain.hh"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "ScanCell.hh"
#include "ScanPin.hh"
#include "utl/Logger.h"

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

std::optional<ScanDriver> ScanChain::getScanIn() const
{
  return scan_in_;
}
std::optional<ScanDriver> ScanChain::getScanEnable() const
{
  return scan_enable_;
}
std::optional<ScanLoad> ScanChain::getScanOut() const
{
  return scan_out_;
}

void ScanChain::setScanIn(const std::optional<ScanDriver>& signal)
{
  scan_in_ = signal;
}
void ScanChain::setScanEnable(const std::optional<ScanDriver>& signal)
{
  scan_enable_ = signal;
}
void ScanChain::setScanOut(const std::optional<ScanLoad>& signal)
{
  scan_out_ = signal;
}

}  // namespace dft
