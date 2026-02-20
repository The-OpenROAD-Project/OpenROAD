#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ClockDomain.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCell.hh"
#include "ScanCellMock.hh"
#include "ScanChain.hh"
#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace dft::test {
namespace {

TEST(TestScanArchitectHeuristic, ArchitectWithOneClockDomainNoMix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  std::vector<std::unique_ptr<ScanCell>> scan_cells;
  std::vector<std::string> scan_cell_names;

  for (uint64_t i = 0; i < 20; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << i;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_unique<ScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        logger));
  }

  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger);
  scan_cells_bucket->init(config, scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(
          config, std::move(scan_cells_bucket), logger);
  scan_architect->init();
  scan_architect->architect();
  std::vector<std::unique_ptr<ScanChain>> scan_chains
      = scan_architect->getScanChains();

  // There should be 2 chains
  EXPECT_EQ(scan_chains.size(), 2);

  // All the scan cells should be in the chains
  std::unordered_set<std::string_view> scan_cells_in_chains_names;
  for (const std::unique_ptr<ScanChain>& scan_chain : scan_chains) {
    EXPECT_FALSE(scan_chain->empty());  // All the chains should have cells
    for (const auto& scan_cell : scan_chain->getScanCells()) {
      scan_cells_in_chains_names.insert(scan_cell->getName());
    }
  }

  for (const std::string& name : scan_cell_names) {
    const bool test = scan_cells_in_chains_names.find(name)
                      != scan_cells_in_chains_names.end();
    EXPECT_TRUE(test);  // Was the cell included?
  }
}

TEST(TestScanArchitectHeuristic, ArchitectWithTwoClockDomainNoMix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  std::vector<std::unique_ptr<ScanCell>> scan_cells;
  std::vector<std::string> scan_cell_names;

  uint64_t name_number = 0;
  for (uint64_t i = 0; i < 20; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_unique<ScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        logger));
  }

  for (uint64_t i = 0; i < 15; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_unique<ScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk2", ClockEdge::Rising),
        logger));
  }

  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger);
  scan_cells_bucket->init(config, scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(
          config, std::move(scan_cells_bucket), logger);
  scan_architect->init();
  scan_architect->architect();
  std::vector<std::unique_ptr<ScanChain>> scan_chains
      = scan_architect->getScanChains();

  // There should be 4 chains
  EXPECT_EQ(scan_chains.size(), 4);

  uint64_t total_bits = 0;
  for (const auto& scan_chain : scan_chains) {
    for (const auto& scan_cell : scan_chain->getScanCells()) {
      total_bits += scan_cell->getBits();
    }
  }
  EXPECT_EQ(total_bits, 20 + 15);
}

TEST(TestScanArchitectHeuristic, ArchitectWithTwoEdgesNoMix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  std::vector<std::unique_ptr<ScanCell>> scan_cells;
  std::vector<std::string> scan_cell_names;

  uint64_t name_number = 0;
  for (uint64_t i = 0; i < 20; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_unique<ScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Rising),
        logger));
  }

  for (uint64_t i = 0; i < 15; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_unique<ScanCellMock>(
        ss.str(),
        std::make_unique<ClockDomain>("clk1", ClockEdge::Falling),
        logger));
  }

  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger);
  scan_cells_bucket->init(config, scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(
          config, std::move(scan_cells_bucket), logger);
  scan_architect->init();
  scan_architect->architect();
  std::vector<std::unique_ptr<ScanChain>> scan_chains
      = scan_architect->getScanChains();

  // There should be 4 chains
  EXPECT_EQ(scan_chains.size(), 4);

  uint64_t total_bits_rising = 0;
  uint64_t total_bits_falling = 0;
  for (const auto& scan_chain : scan_chains) {
    for (const auto& scan_cell : scan_chain->getScanCells()) {
      switch (scan_cell->getClockDomain().getClockEdge()) {
        case ClockEdge::Falling:
          total_bits_falling += scan_cell->getBits();
          break;
        case ClockEdge::Rising:
          total_bits_rising += scan_cell->getBits();
          break;
      }
    }
  }
  EXPECT_EQ(total_bits_rising, 20);
  EXPECT_EQ(total_bits_falling, 15);
}

}  // namespace
}  // namespace dft::test
