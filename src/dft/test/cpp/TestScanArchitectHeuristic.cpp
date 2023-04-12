#define BOOST_TEST_MODULE TestScanReplace
#include <boost/test/included/unit_test.hpp>
#include <sstream>
#include <unordered_set>

#include "ClockDomain.hh"
#include "ScanArchitect.hh"
#include "ScanArchitectConfig.hh"
#include "ScanCellMock.hh"

namespace {

using namespace dft;
using namespace dft::test;
BOOST_AUTO_TEST_SUITE(test_suite)

// Check if we can architect a simple two chain design with clock no mix
BOOST_AUTO_TEST_CASE(test_one_clock_domain_no_mix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  std::vector<std::shared_ptr<ScanCell>> scan_cells;
  std::vector<std::string> scan_cell_names;

  for (uint64_t i = 0; i < 20; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << i;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_shared<ScanCellMock>(
        ss.str(), std::make_unique<ClockDomain>("clk1", ClockEdge::Rising)));
  }

  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger);
  scan_cells_bucket->init(config, scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(config,
                                                  std::move(scan_cells_bucket));
  scan_architect->init();
  scan_architect->architect();
  std::vector<std::unique_ptr<ScanChain>> scan_chains
      = scan_architect->getScanChains();

  // There should be 2 chains
  BOOST_TEST(scan_chains.size() == 2);

  // All the scan cells should be in the chains
  std::unordered_set<std::string> scan_cells_in_chains_names;
  for (const std::unique_ptr<ScanChain>& scan_chain : scan_chains) {
    BOOST_TEST(!scan_chain->empty());  // All the cells should have cells
    for (const auto& scan_cell : scan_chain->getRisingEdgeScanCells()) {
      scan_cells_in_chains_names.insert(scan_cell->getName());
    }
  }

  for (const std::string& name : scan_cell_names) {
    const bool test = scan_cells_in_chains_names.find(name)
                      != scan_cells_in_chains_names.end();
    BOOST_TEST(test);  // Was the cell included?
  }
}

// Check if we can architect a simple four chain design, with two clocks and no
// clock mixing
BOOST_AUTO_TEST_CASE(test_two_clock_domain_no_mix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  std::vector<std::shared_ptr<ScanCell>> scan_cells;
  std::vector<std::string> scan_cell_names;

  uint64_t name_number = 0;
  for (uint64_t i = 0; i < 20; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_shared<ScanCellMock>(
        ss.str(), std::make_unique<ClockDomain>("clk1", ClockEdge::Rising)));
  }

  for (uint64_t i = 0; i < 15; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_shared<ScanCellMock>(
        ss.str(), std::make_unique<ClockDomain>("clk2", ClockEdge::Rising)));
  }

  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger);
  scan_cells_bucket->init(config, scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(config,
                                                  std::move(scan_cells_bucket));
  scan_architect->init();
  scan_architect->architect();
  std::vector<std::unique_ptr<ScanChain>> scan_chains
      = scan_architect->getScanChains();

  // There should be 4 chains
  BOOST_TEST(scan_chains.size() == 4);

  uint64_t total_bits = 0;
  for (const auto& scan_chain : scan_chains) {
    for (const auto& scan_cell : scan_chain->getRisingEdgeScanCells()) {
      total_bits += scan_cell->getBits();
    }
    BOOST_TEST(scan_chain->getFallingEdgeScanCells().empty());
  }
  BOOST_TEST(total_bits == 20 + 15);
}

// Check if we can architect a simple four chain design, with two clocks and no
// clock mixing
BOOST_AUTO_TEST_CASE(test_two_edges_no_mix)
{
  utl::Logger* logger = new utl::Logger();

  ScanArchitectConfig config;
  config.setClockMixing(ScanArchitectConfig::ClockMixing::NoMix);
  config.setMaxLength(10);
  std::vector<std::shared_ptr<ScanCell>> scan_cells;
  std::vector<std::string> scan_cell_names;

  uint64_t name_number = 0;
  for (uint64_t i = 0; i < 20; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_shared<ScanCellMock>(
        ss.str(), std::make_unique<ClockDomain>("clk1", ClockEdge::Rising)));
  }

  for (uint64_t i = 0; i < 15; ++i) {
    std::stringstream ss;
    ss << "scan_cell" << name_number;
    ++name_number;
    scan_cell_names.push_back(ss.str());
    scan_cells.push_back(std::make_shared<ScanCellMock>(
        ss.str(), std::make_unique<ClockDomain>("clk1", ClockEdge::Falling)));
  }

  std::unique_ptr<ScanCellsBucket> scan_cells_bucket
      = std::make_unique<ScanCellsBucket>(logger);
  scan_cells_bucket->init(config, scan_cells);

  std::unique_ptr<ScanArchitect> scan_architect
      = ScanArchitect::ConstructScanScanArchitect(config,
                                                  std::move(scan_cells_bucket));
  scan_architect->init();
  scan_architect->architect();
  std::vector<std::unique_ptr<ScanChain>> scan_chains
      = scan_architect->getScanChains();

  // There should be 4 chains
  BOOST_TEST(scan_chains.size() == 4);

  uint64_t total_bits_rising = 0;
  uint64_t total_bits_falling = 0;
  for (const auto& scan_chain : scan_chains) {
    for (const auto& scan_cell : scan_chain->getRisingEdgeScanCells()) {
      total_bits_rising += scan_cell->getBits();
    }
    for (const auto& scan_cell : scan_chain->getFallingEdgeScanCells()) {
      total_bits_falling += scan_cell->getBits();
    }
  }
  BOOST_TEST(total_bits_rising == 20);
  BOOST_TEST(total_bits_falling == 15);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace
