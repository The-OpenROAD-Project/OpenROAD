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

#include "ScanArchitect.hh"

#include "ClockDomain.hh"
#include "ScanArchitectHeuristic.hh"

namespace dft {

namespace {

bool CompareScanCells(const std::unique_ptr<ScanCell>& lhs,
                      const std::unique_ptr<ScanCell>& rhs)
{
  // If they have the same number of bits, then we compare the names of the
  // cells so they are ordered by name
  if (lhs->getBits() == rhs->getBits()) {
    const ClockDomain& lhs_clock_domain = lhs->getClockDomain();
    const ClockDomain& rhs_clock_domain = rhs->getClockDomain();

    if (lhs_clock_domain.getClockName() == rhs_clock_domain.getClockName()) {
      return lhs_clock_domain.getClockEdge() < rhs_clock_domain.getClockEdge();
    }

    return lhs_clock_domain.getClockName() < rhs_clock_domain.getClockName();
  }
  // Bigger elements last
  return lhs->getBits() < rhs->getBits();
}

void SortScanCells(std::vector<std::unique_ptr<ScanCell>>& scan_cells)
{
  std::sort(scan_cells.begin(), scan_cells.end(), CompareScanCells);
}

}  // namespace

ScanCellsBucket::ScanCellsBucket(utl::Logger* logger) : logger_(logger)
{
}

void ScanCellsBucket::init(const ScanArchitectConfig& config,
                           std::vector<std::unique_ptr<ScanCell>>& scan_cells)
{
  auto hash_fn = GetClockDomainHashFn(config, logger_);
  for (std::unique_ptr<ScanCell>& scan_cell : scan_cells) {
    buckets_[hash_fn(scan_cell->getClockDomain())].push_back(
        std::move(scan_cell));
  }

  // Sort the buckets
  for (auto& [hash_domain, scan_cells] : buckets_) {
    SortScanCells(scan_cells);
  }
}

std::unordered_map<size_t, uint64_t>
ScanCellsBucket::getTotalBitsPerHashDomain() const
{
  std::unordered_map<size_t, uint64_t> total_bits;
  for (const auto& [hash_domain, scan_cells] : buckets_) {
    for (const std::unique_ptr<ScanCell>& scan_cell : scan_cells) {
      total_bits[hash_domain] += scan_cell->getBits();
    }
  }
  return total_bits;
}

std::unique_ptr<ScanCell> ScanCellsBucket::pop(size_t hash_domain)
{
  auto& bucket = buckets_.find(hash_domain)->second;
  std::unique_ptr<ScanCell> scan_cell = std::move(bucket.back());
  bucket.pop_back();
  return scan_cell;
}

uint64_t ScanCellsBucket::numberOfCells(size_t hash_domain) const
{
  return buckets_.find(hash_domain)->second.size();
}

std::unique_ptr<ScanArchitect> ScanArchitect::ConstructScanScanArchitect(
    const ScanArchitectConfig& config,
    std::unique_ptr<ScanCellsBucket> scan_cells_bucket,
    utl::Logger* logger_)
{
  return std::make_unique<ScanArchitectHeuristic>(
      config, std::move(scan_cells_bucket), logger_);
}

ScanArchitect::ScanArchitect(const ScanArchitectConfig& config,
                             std::unique_ptr<ScanCellsBucket> scan_cells_bucket)
    : config_(config), scan_cells_bucket_(std::move(scan_cells_bucket))
{
}

void ScanArchitect::init()
{
  createScanChains();
}

void ScanArchitect::inferChainCount()
{
  std::unordered_map<size_t, uint64_t> hash_domains_total_bits
      = scan_cells_bucket_->getTotalBitsPerHashDomain();

  if (auto max_length = config_.getMaxLength(); max_length.has_value()) {
    // The user is saying that we should respect this max_length
    hash_domain_to_limits_ = inferChainCountFromMaxLength(
        hash_domains_total_bits, max_length.value(), config_.getMaxChains());
  } else {
    // The user did not specify any max_length, let's use a default of 200
    hash_domain_to_limits_ = inferChainCountFromMaxLength(
        hash_domains_total_bits, 200, config_.getMaxChains());
  }
}

std::map<size_t, ScanArchitect::HashDomainLimits>
ScanArchitect::inferChainCountFromMaxLength(
    const std::unordered_map<size_t, uint64_t>& hash_domains_total_bit,
    uint64_t max_length,
    const std::optional<uint64_t>& max_chains)
{
  std::map<size_t, HashDomainLimits> hash_domain_to_limits;
  for (const auto& [hash_domain, bits] : hash_domains_total_bit) {
    uint64_t domain_chain_count = 0;
    if (bits % max_length != 0) {
      // For unbalance case, we need +1 chains to hold the aditionals cells
      domain_chain_count = bits / max_length + 1;
    } else {
      domain_chain_count = bits / max_length;
    }

    if (max_chains.has_value()) {
      domain_chain_count = std::min(domain_chain_count, max_chains.value());
    }

    uint64_t domain_max_length = 0;
    if (bits % domain_chain_count == 0) {
      domain_max_length = bits / domain_chain_count;
    } else {
      domain_max_length = bits / domain_chain_count + 1;
    }

    HashDomainLimits hash_domain_limits;
    hash_domain_limits.chain_count = domain_chain_count;
    hash_domain_limits.max_length = domain_max_length;

    hash_domain_to_limits.insert({hash_domain, hash_domain_limits});
  }
  return hash_domain_to_limits;
}

void ScanArchitect::createScanChains()
{
  inferChainCount();
  uint64_t chain_number = 0;
  for (const auto& [hash_domain, limits] : hash_domain_to_limits_) {
    for (uint64_t i = 0; i < limits.chain_count; ++i) {
      std::string chain_name = fmt::format("chain_{}", chain_number);
      hash_domain_scan_chains_[hash_domain].push_back(
          std::make_unique<ScanChain>(chain_name));
      ++chain_number;
    }
  }
}

std::vector<std::unique_ptr<ScanChain>> ScanArchitect::getScanChains()
{
  std::vector<std::unique_ptr<ScanChain>> scan_chains_flat;
  for (auto& [hash_domain, scan_chains] : hash_domain_scan_chains_) {
    std::move(std::begin(scan_chains),
              std::end(scan_chains),
              std::back_inserter(scan_chains_flat));
  }

  std::sort(scan_chains_flat.begin(),
            scan_chains_flat.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs->getName() < rhs->getName();
            });

  return scan_chains_flat;
}
}  // namespace dft
