// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ScanCell.hh"
#include "ScanPin.hh"
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

  // For stitchers
  std::optional<ScanDriver> getScanIn() const;
  std::optional<ScanDriver> getScanEnable() const;
  std::optional<ScanLoad> getScanOut() const;

  void setScanIn(const std::optional<ScanDriver>& signal);
  void setScanEnable(const std::optional<ScanDriver>& signal);
  void setScanOut(const std::optional<ScanLoad>& signal);

 private:
  std::string name_;
  std::vector<std::unique_ptr<ScanCell>> rising_edge_scan_cells_;
  std::vector<std::unique_ptr<ScanCell>> falling_edge_scan_cells_;
  // The final container of the scan cells once sorted by Scan Architect
  std::vector<std::unique_ptr<ScanCell>> scan_cells_;

  // The total bits in this scan chain. Scan cells can contain more than one
  // bit, that's why this is different from the number of cells.
  uint64_t bits_;

  // After stitching: store input/output bterms/iterms
  std::optional<ScanDriver> scan_in_;
  std::optional<ScanLoad> scan_out_;
  std::optional<ScanDriver> scan_enable_;
};

}  // namespace dft
