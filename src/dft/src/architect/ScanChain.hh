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
#include <vector>

#include "ScanCell.hh"
#include "utl/Logger.h"

namespace dft {

// A scan chain. This class contains all the cells that are going to be stitched
// together into a scan chain. A scan chain can mix different clock domains
// (Equivalent clocks) and different edges too. We are going to connect together
// first the falling edge cells and then the rising edge to prevent
// synchronization issues while shifting data in.
//
// TODO:
//  - Find the scan in and scan out of the chain
//  - Find the scan enable of the chain
class ScanChain
{
 public:
  explicit ScanChain(const std::string& name);
  // Not copyable or movable
  ScanChain(const ScanChain&) = delete;
  ScanChain& operator=(const ScanChain&) = delete;

  // Adds a scan cell to the chain. Depending on the edge of the cell, we will
  // move the cell to rising or falling vectors.
  void add(std::unique_ptr<ScanCell> scan_cell);

  // Returns true if the scan chain is empty
  bool empty() const;

  // The number of bits inside the scan chain. O(1) since this is a value that
  // we update every time we add a scan cell
  uint64_t getBits() const;

  // Sorts the scan cells of this chain.  This function has 3 arguments: falling
  // cells, rising cells and the output vector with the sorted cells
  void sortScanCells(
      const std::function<void(std::vector<std::unique_ptr<ScanCell>>&,
                               std::vector<std::unique_ptr<ScanCell>>&,
                               std::vector<std::unique_ptr<ScanCell>>&)>&
          sort_fn);

  // Returns a reference to a vector containing all the scan cells of the chain
  const std::vector<std::unique_ptr<ScanCell>>& getScanCells() const;

  // Reports this scan chain contents. If verbose is true, the cells of the
  // chains are also going to be printed
  void report(utl::Logger* logger, bool verbose) const;

  // Returns the name of the scan chain
  const std::string& getName() const;

 private:
  std::string name_;
  std::vector<std::unique_ptr<ScanCell>> rising_edge_scan_cells_;
  std::vector<std::unique_ptr<ScanCell>> falling_edge_scan_cells_;
  // The final container of the scan cells once sorted by Scan Architect
  std::vector<std::unique_ptr<ScanCell>> scan_cells_;

  // The total bits in this scan chain. Scan cells can contain more than one
  // bit, that's why this is different from the number of cells.
  uint64_t bits_;
};

}  // namespace dft
