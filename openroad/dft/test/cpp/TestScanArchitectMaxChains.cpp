// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

// Unit tests for the max_chains configuration in ScanArchitect.
//
// max_chains caps the number of chains per clock domain. When it forces fewer
// chains than max_length would produce, the effective chain length grows beyond
// max_length to accommodate all bits. These tests verify that behavior directly
// against the code in ScanArchitect::inferChainCountFromMaxLength().

#include <algorithm>
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

// Minimal ScanCell mock, defined locally so this test file has no external
// dependencies beyond the test framework.
class CellMock : public ScanCell
{
 public:
  CellMock(const std::string& name,
           std::unique_ptr<ClockDomain> clock_domain,
           utl::Logger* logger,
           uint64_t bits = 1)
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

// Helper: build N 1-bit cells in the given clock domain.
void addCells(std::vector<std::unique_ptr<ScanCell>>& cells,
              const std::string& prefix,
              uint64_t count,
              const std::string& clk,
              ClockEdge edge,
              utl::Logger* logger)
{
  for (uint64_t i = 0; i < count; ++i) {
    std::stringstream ss;
    ss << prefix << i;
    cells.push_back(std::make_unique<CellMock>(
        ss.str(), std::make_unique<ClockDomain>(clk, edge), logger));
  }
}

// Helper: sum bits across all chains.
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

// Helper: true if no chain is empty.
bool noEmptyChains(const std::vector<std::unique_ptr<ScanChain>>& chains)
{
  return std::ranges::none_of(chains,
                              [](const auto& chain) { return chain->empty(); });
}

// Helper: run the architect and return chains.
std::vector<std::unique_ptr<ScanChain>> runArchitect(
    const ScanArchitectConfig& config,
    std::vector<std::unique_ptr<ScanCell>>& cells,
    utl::Logger* logger)
{
  auto bucket = std::make_unique<ScanCellsBucket>(logger);
  bucket->init(config, cells);
  auto architect = ScanArchitect::ConstructScanScanArchitect(
      config, std::move(bucket), logger);
  architect->init();
  architect->architect();
  return architect->getScanChains();
}

// --- Tests ---

// max_chains=1: all 20 bits forced into a single chain even though
// max_length=10 would normally produce 2 chains.
TEST(TestScanArchitectMaxChains, MaxChainsOneOverridesMaxLength)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  config.setMaxChains(1);

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "cell", 20, "clk1", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  // max_chains=1 caps to 1 chain despite max_length=10 wanting 2
  EXPECT_EQ(chains.size(), 1);
  EXPECT_EQ(totalBits(chains), 20);
}

// max_chains larger than what max_length produces has no effect —
// max_length=10 with 20 bits → 2 chains; max_chains=5 doesn't add chains.
TEST(TestScanArchitectMaxChains, MaxChainsLargerThanNeededHasNoEffect)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  config.setMaxChains(5);

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "cell", 20, "clk1", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  // max_length=10 produces 2 chains; max_chains=5 is larger so no effect
  EXPECT_EQ(chains.size(), 2);
  EXPECT_EQ(totalBits(chains), 20);
}

// max_chains applied independently per clock domain.
// clk1: 20 bits, max_chains=2 → 2 chains
// clk2: 20 bits, max_chains=2 → 2 chains
// Total: 4 chains (2 per domain)
TEST(TestScanArchitectMaxChains, MaxChainsAppliedPerDomainIndependently)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(5);  // would produce 4 chains per domain without cap
  config.setMaxChains(2);

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "clk1_cell", 20, "clk1", ClockEdge::Rising, &logger);
  addCells(cells, "clk2_cell", 20, "clk2", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  // Each domain capped at 2 chains → 4 total
  EXPECT_EQ(chains.size(), 4);
  EXPECT_EQ(totalBits(chains), 40);
}

// max_chains=1 with two clock domains → 1 chain per domain = 2 total.
TEST(TestScanArchitectMaxChains, MaxChainsOnePerDomainTwoDomains)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(5);
  config.setMaxChains(1);

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "clk1_cell", 20, "clk1", ClockEdge::Rising, &logger);
  addCells(cells, "clk2_cell", 15, "clk2", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  // 1 chain per domain → 2 total; all bits preserved
  EXPECT_EQ(chains.size(), 2);
  EXPECT_EQ(totalBits(chains), 35);
}

// No max_chains set — chain count is driven entirely by max_length.
// 30 bits, max_length=10 → 3 chains.
TEST(TestScanArchitectMaxChains, NoMaxChainsOnlyMaxLength)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  // max_chains intentionally NOT set

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "cell", 30, "clk1", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  EXPECT_EQ(chains.size(), 3);
  EXPECT_EQ(totalBits(chains), 30);
}

// max_chains exactly equals what max_length would produce — no conflict.
TEST(TestScanArchitectMaxChains, MaxChainsExactlyEqualsMaxLengthResult)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  config.setMaxChains(2);  // max_length also produces 2 for 20 bits

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "cell", 20, "clk1", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  EXPECT_EQ(chains.size(), 2);
  EXPECT_EQ(totalBits(chains), 20);
}

// max_chains=6 with 10 bits gives a length of 2, which fills only 5 chains.
// No empty sixth chain is created.
TEST(TestScanArchitectMaxChains, MaxChainsAboveBalancedCountCreatesNoEmptyChain)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(1);
  config.setMaxChains(6);

  std::vector<std::unique_ptr<ScanCell>> cells;
  addCells(cells, "cell", 10, "clk1", ClockEdge::Rising, &logger);

  const auto chains = runArchitect(config, cells, &logger);

  EXPECT_EQ(chains.size(), 5);
  EXPECT_TRUE(noEmptyChains(chains));
  EXPECT_EQ(totalBits(chains), 10);
}

// Multi-bit cells overshoot the max length: with cells of 4, 4, 1 and 1 bits
// and a length of 3, the 4-bit cells fill one chain each and the 1-bit cells
// share a third, so the fourth chain is empty and dropped.
TEST(TestScanArchitectMaxChains, MultiBitOvershootDropsEmptyChain)
{
  utl::Logger logger;

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(3);

  std::vector<std::unique_ptr<ScanCell>> cells;
  for (const uint64_t bits : {4U, 4U, 1U, 1U}) {
    cells.push_back(std::make_unique<CellMock>(
        "cell" + std::to_string(cells.size()),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        &logger,
        bits));
  }

  const auto chains = runArchitect(config, cells, &logger);

  EXPECT_EQ(chains.size(), 3);
  EXPECT_TRUE(noEmptyChains(chains));
  EXPECT_EQ(totalBits(chains), 10);
}

}  // namespace
}  // namespace dft::test
