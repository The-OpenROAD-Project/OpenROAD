// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

// Unit tests for ScanArchitect behavior with multibit scan cells.
//
// These tests verify that the architect correctly handles cells where
// getBits() > 1 (InternalScanMultibitCell) and cells where getBits() == 1
// but represent one slice of a physical multibit cell (ExternalScanMultibitCell).
// No real Liberty library files are needed — a local mock supplies getBits().

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "ScanChain.hh"
#include "ScanPin.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft::test {
namespace {

// Minimal ScanCell mock where getBits() is configurable.
// Models both InternalScanMultibitCell (bits > 1) and
// ExternalScanMultibitCell slices (bits == 1).
class MultibitScanCellMock : public ScanCell
{
 public:
  MultibitScanCellMock(const std::string& name,
                       std::unique_ptr<ClockDomain> clock_domain,
                       uint64_t bits,
                       utl::Logger* logger)
      : ScanCell(name, std::move(clock_domain), logger), bits_(bits)
  {
  }

  uint64_t getBits() const override { return bits_; }

  void connectScanEnable(const ScanDriver&) const override {}
  void connectScanIn(const ScanDriver&) const override {}
  void connectScanOut(const ScanLoad&) const override {}

  ScanLoad getScanEnable() const override
  {
    return ScanLoad(static_cast<odb::dbITerm*>(nullptr));
  }
  ScanLoad getScanIn() const override
  {
    return ScanLoad(static_cast<odb::dbBTerm*>(nullptr));
  }
  ScanDriver getScanOut() const override
  {
    return ScanDriver(static_cast<odb::dbBTerm*>(nullptr));
  }

  odb::Point getOrigin() const override { return odb::Point(); }
  bool isPlaced() const override { return false; }
  odb::Point getScanInLocation() const override { return odb::Point(); }
  odb::Point getScanOutLocation() const override { return odb::Point(); }

 private:
  uint64_t bits_;
};

// Helper: sum all bits across all chains.
uint64_t totalBits(const std::vector<std::unique_ptr<ScanChain>>& chains)
{
  uint64_t total = 0;
  for (const auto& chain : chains) {
    for (const auto& cell : chain->getScanCells()) {
      total += cell->getBits();
    }
  }
  return total;
}

// --- Tests ---

// 4 cells × 4 bits each = 16 bits total.
// max_length=10 → needs 2 chains.
// Verifies that the architect bins by bit count, not cell count.
TEST(TestScanArchitectMultibit, InternalMultibitCellsBitCount)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);

  std::vector<std::unique_ptr<ScanCell>> scan_cells;
  for (uint64_t i = 0; i < 4; ++i) {
    std::stringstream ss;
    ss << "multibit_cell" << i;
    scan_cells.push_back(std::make_unique<MultibitScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        /*bits=*/4,
        logger));
  }

  auto bucket = std::make_unique<ScanCellsBucket>(logger);
  bucket->init(config, scan_cells);

  auto architect = ScanArchitect::ConstructScanScanArchitect(
      config, std::move(bucket), logger);
  architect->init();
  architect->architect();
  auto chains = architect->getScanChains();

  EXPECT_EQ(chains.size(), 2);
  EXPECT_EQ(totalBits(chains), 16);
}

// 4 slices of a physical 4-bit external scan cell — each slice has getBits()==1.
// Identical to 4 OneBitScanCells from the architect's perspective.
// max_length=10 → all 4 fit in 1 chain.
TEST(TestScanArchitectMultibit, ExternalMultibitSlicesAreOneBitEach)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);

  std::vector<std::unique_ptr<ScanCell>> scan_cells;
  for (uint64_t bit = 0; bit < 4; ++bit) {
    std::stringstream ss;
    ss << "ext_cell[" << bit << "]";
    scan_cells.push_back(std::make_unique<MultibitScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        /*bits=*/1,
        logger));
  }

  auto bucket = std::make_unique<ScanCellsBucket>(logger);
  bucket->init(config, scan_cells);

  auto architect = ScanArchitect::ConstructScanScanArchitect(
      config, std::move(bucket), logger);
  architect->init();
  architect->architect();
  auto chains = architect->getScanChains();

  EXPECT_EQ(chains.size(), 1);
  EXPECT_EQ(totalBits(chains), 4);
}

// 5 one-bit cells + 2 four-bit multibit cells = 5 + 8 = 13 bits total.
// max_length=10 → 2 chains.
// Verifies that 1-bit and multibit cells coexist correctly in the same domain.
TEST(TestScanArchitectMultibit, MixedOneBitAndMultibitCells)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);

  std::vector<std::unique_ptr<ScanCell>> scan_cells;

  for (uint64_t i = 0; i < 5; ++i) {
    std::stringstream ss;
    ss << "onebit_cell" << i;
    scan_cells.push_back(std::make_unique<MultibitScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        /*bits=*/1,
        logger));
  }

  for (uint64_t i = 0; i < 2; ++i) {
    std::stringstream ss;
    ss << "multibit_cell" << i;
    scan_cells.push_back(std::make_unique<MultibitScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        /*bits=*/4,
        logger));
  }

  auto bucket = std::make_unique<ScanCellsBucket>(logger);
  bucket->init(config, scan_cells);

  auto architect = ScanArchitect::ConstructScanScanArchitect(
      config, std::move(bucket), logger);
  architect->init();
  architect->architect();
  auto chains = architect->getScanChains();

  EXPECT_EQ(chains.size(), 2);
  EXPECT_EQ(totalBits(chains), 13);
}

// Two clock domains, each with multibit cells.
// clk1: 3 × 4-bit = 12 bits → 2 chains (max_length=10)
// clk2: 2 × 4-bit = 8 bits  → 1 chain
// Total: 3 chains
TEST(TestScanArchitectMultibit, MultibitCellsTwoClockDomainsNoMix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);

  std::vector<std::unique_ptr<ScanCell>> scan_cells;

  for (uint64_t i = 0; i < 3; ++i) {
    std::stringstream ss;
    ss << "clk1_cell" << i;
    scan_cells.push_back(std::make_unique<MultibitScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        /*bits=*/4,
        logger));
  }

  for (uint64_t i = 0; i < 2; ++i) {
    std::stringstream ss;
    ss << "clk2_cell" << i;
    scan_cells.push_back(std::make_unique<MultibitScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk2", ClockEdge::Rising),
        /*bits=*/4,
        logger));
  }

  auto bucket = std::make_unique<ScanCellsBucket>(logger);
  bucket->init(config, scan_cells);

  auto architect = ScanArchitect::ConstructScanScanArchitect(
      config, std::move(bucket), logger);
  architect->init();
  architect->architect();
  auto chains = architect->getScanChains();

  EXPECT_EQ(chains.size(), 3);
  EXPECT_EQ(totalBits(chains), 12 + 8);
}

}  // namespace
}  // namespace dft::test
