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
#include <map>
#include <memory>
#include <vector>

#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "ScanChain.hh"

namespace dft {

// Contains all the scan cells of the design and maintains them separated based
// on the hash domain.
class ScanCellsBucket
{
 public:
  explicit ScanCellsBucket(utl::Logger* logger);
  // Not copyable or movable
  ScanCellsBucket(const ScanCellsBucket&) = delete;
  ScanCellsBucket& operator=(const ScanCellsBucket&) = delete;

  // Gets the next scan cell of the given hash domain
  std::unique_ptr<ScanCell> pop(size_t hash_domain);

  // Init the scan cell bucket with with the given config and scan cells
  void init(const ScanArchitectConfig& config,
            std::vector<std::unique_ptr<ScanCell>>& scan_cells);

  // Returns the number of bits we need to include in each hash domain
  std::unordered_map<size_t, uint64_t> getTotalBitsPerHashDomain() const;

  // The number of cells in the given hash domain
  uint64_t numberOfCells(size_t hash_domain) const;

 private:
  std::unordered_map<size_t, std::vector<std::unique_ptr<ScanCell>>> buckets_;
  utl::Logger* logger_;
};

// The Scan Architect. We can implement different algorithms to architect
class ScanArchitect
{
 public:
  // The limits of a hash domain.
  struct HashDomainLimits
  {
    uint64_t
        chain_count;      // How many chains we are creating in this hash domain
    uint64_t max_length;  // What is the max length of this hash domain
  };

  ScanArchitect(const ScanArchitectConfig& config,
                std::unique_ptr<ScanCellsBucket> scan_cells_bucket);
  // Not copyable or movable
  ScanArchitect(const ScanArchitect&) = delete;
  ScanArchitect& operator=(const ScanArchitect&) = delete;
  virtual ~ScanArchitect() = default;

  // Init the Scan Architect
  virtual void init();

  // Performs the Scan Architect. Implement the bin packing solver here.
  virtual void architect() = 0;

  // Returns a vector of the generated scan chains. This will invalidate the
  // internal state as this performs a move of the std::unique_ptr<ScanChain>
  std::vector<std::unique_ptr<ScanChain>> getScanChains();

  // Calculates the max_length per hash domain based on the given global
  // max_length
  static std::map<size_t, HashDomainLimits> inferChainCountFromMaxLength(
      const std::unordered_map<size_t, uint64_t>& hash_domains_total_bit,
      uint64_t max_length);

  // Returns an ScanArchitect object based on the configuration.
  //
  // TODO: Allow the users to select the scan architect algorithm that they want
  // from the config. We want to support multiple Scan Architect algorithms
  // since some of them could generate better scan chains for some designs
  static std::unique_ptr<ScanArchitect> ConstructScanScanArchitect(
      const ScanArchitectConfig& config,
      std::unique_ptr<ScanCellsBucket> scan_cells_bucket);

 protected:
  void createScanChains();
  void inferChainCount();

  const ScanArchitectConfig& config_;
  std::unique_ptr<ScanCellsBucket> scan_cells_bucket_;
  std::vector<std::unique_ptr<ScanChain>> scan_chains_;
  std::unordered_map<size_t, std::vector<std::unique_ptr<ScanChain>>>
      hash_domain_scan_chains_;
  // We use a map instead of an unordered_map to keep consistency between runs
  // regarding what scan chain is assigned to each hash domain
  std::map<size_t, HashDomainLimits> hash_domain_to_limits_;
};

}  // namespace dft
