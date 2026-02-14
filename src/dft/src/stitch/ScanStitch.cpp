// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanStitch.hh"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ScanCell.hh"
#include "ScanChain.hh"
#include "ScanPin.hh"
#include "ScanStitchConfig.hh"
#include "boost/algorithm/string.hpp"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace {
constexpr std::string_view kScanEnable = "scan-enable";
constexpr std::string_view kScanIn = "scan-in";
constexpr std::string_view kScanOut = "scan-out";
constexpr size_t kEnableNumber = 0;
}  // namespace

namespace dft {

ScanStitch::ScanStitch(odb::dbDatabase* db,
                       utl::Logger* logger,
                       const ScanStitchConfig& config)
    : config_(config), db_(db), logger_(logger)
{
  odb::dbChip* chip = db_->getChip();
  top_block_ = chip->getBlock();
}

void ScanStitch::Stitch(
    const std::vector<std::unique_ptr<ScanChain>>& scan_chains)
{
  if (scan_chains.empty()) {
    return;
  }

  size_t ordinal = 0;
  for (const std::unique_ptr<ScanChain>& scan_chain : scan_chains) {
    Stitch(top_block_, *scan_chain, ordinal);
    ordinal += 1;
  }
}

void ScanStitch::Stitch(odb::dbBlock* block,
                        ScanChain& scan_chain,
                        size_t ordinal)
{
  auto scan_enable_name
      = fmt::format(FMT_RUNTIME(config_.getEnableNamePattern()), kEnableNumber);
  auto scan_enable_driver = FindOrCreateScanEnable(block, scan_enable_name);

  auto scan_in_name
      = fmt::format(FMT_RUNTIME(config_.getInNamePattern()), ordinal);
  ScanDriver scan_in_driver = FindOrCreateScanIn(block, scan_in_name);

  scan_chain.setScanIn(scan_in_driver);
  scan_chain.setScanEnable(scan_enable_driver);

  // We need fast pop for front and back
  std::deque<std::reference_wrapper<const std::unique_ptr<ScanCell>>>
      scan_cells;

  const std::vector<std::unique_ptr<ScanCell>>& original_scan_cells
      = scan_chain.getScanCells();

  std::ranges::copy(original_scan_cells, std::back_inserter(scan_cells));

  // All the cells in the scan chain are controlled by the same scan enable
  for (const std::unique_ptr<ScanCell>& scan_cell : scan_cells) {
    scan_cell->connectScanEnable(scan_enable_driver);
  }

  // Lets get the first and last cell
  const std::unique_ptr<ScanCell>& first_scan_cell = *scan_cells.begin();
  const std::unique_ptr<ScanCell>& last_scan_cell = *(scan_cells.end() - 1);

  if (!scan_cells.empty()) {
    scan_cells.pop_front();
  }

  if (!scan_cells.empty()) {
    scan_cells.pop_back();
  }

  for (auto it = scan_cells.begin(); it != scan_cells.end(); ++it) {
    const std::unique_ptr<ScanCell>& current = *it;
    const std::unique_ptr<ScanCell>& next = *(it + 1);
    // Connects current cell scan out to next cell scan in
    next->connectScanIn(current->getScanOut());
  }

  // Let's connect the first cell
  first_scan_cell->connectScanEnable(scan_enable_driver);
  first_scan_cell->connectScanIn(scan_in_driver);

  if (!scan_cells.empty()) {
    scan_cells.begin()->get()->connectScanIn(first_scan_cell->getScanOut());
  } else {
    // If last_scan_cell == first_scan_cell, then scan in was already connected
    if (last_scan_cell != first_scan_cell) {
      last_scan_cell->connectScanIn(first_scan_cell->getScanOut());
    }
  }

  // Let's connect the last cell
  auto scan_out_name
      = fmt::format(FMT_RUNTIME(config_.getOutNamePattern()), ordinal);
  ScanLoad scan_out_load
      = FindOrCreateScanOut(block, last_scan_cell->getScanOut(), scan_out_name);
  last_scan_cell->connectScanOut(scan_out_load);
  scan_chain.setScanOut(scan_out_load);
}

namespace {
static std::pair<std::string, std::optional<std::string>> SplitTermIdentifier(
    const std::string& input)
{
  size_t tracker = 0;
  size_t slash_position;
  while ((slash_position = input.find('/', tracker)) != std::string::npos) {
    if (slash_position != 0 && input[slash_position - 1] == '\\') {
      tracker = slash_position + 1;
      continue;
    }
    return {input.substr(0, slash_position), input.substr(slash_position + 1)};
  }
  return {input, std::nullopt};
}
}  // namespace

ScanDriver ScanStitch::FindOrCreateDriver(std::string_view kind,
                                          odb::dbBlock* block,
                                          const std::string& with_name)
{
  auto term_info = SplitTermIdentifier(with_name);

  if (term_info.second.has_value()) {  // Instance/ITerm
    auto inst = block->findInst(term_info.first.c_str());
    if (inst == nullptr) {
      logger_->error(utl::DFT,
                     34,
                     "Instance {} not found for {} port",
                     term_info.first,
                     kind);
    }
    auto iterm = inst->findITerm(term_info.second.value().c_str());
    if (iterm == nullptr) {
      logger_->error(utl::DFT,
                     35,
                     "ITerm {}/{} not found for {} port",
                     term_info.first,
                     term_info.second.value(),
                     kind);
    }
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      logger_->error(utl::DFT,
                     36,
                     "ITerm {}/{} for {} port is a {}",
                     term_info.first,
                     term_info.second.value(),
                     kind,
                     iterm->getIoType().getString());
    }
    return ScanDriver(iterm);
  }

  // BTerm
  auto bterm = block->findBTerm(with_name.data());
  if (bterm != nullptr) {
    // We don't actually care if it's an output, that works here too.
    return ScanDriver(bterm);
  }
  return CreateNewPort<ScanDriver>(block, with_name);
}

ScanDriver ScanStitch::FindOrCreateScanEnable(odb::dbBlock* block,
                                              const std::string& with_name)
{
  return FindOrCreateDriver(kScanEnable, block, with_name);
}

ScanDriver ScanStitch::FindOrCreateScanIn(odb::dbBlock* block,
                                          const std::string& with_name)
{
  return FindOrCreateDriver(kScanIn, block, with_name);
}

ScanLoad ScanStitch::FindOrCreateScanOut(odb::dbBlock* block,
                                         const ScanDriver& cell_scan_out,
                                         const std::string& with_name)
{
  auto term_info = SplitTermIdentifier(with_name);

  if (term_info.second.has_value()) {  // Instance/ITerm
    auto inst = block->findInst(term_info.first.c_str());
    if (inst == nullptr) {
      logger_->error(utl::DFT,
                     37,
                     "Instance {} not found for {} port",
                     term_info.first,
                     kScanOut);
    }
    auto iterm = inst->findITerm(term_info.second.value().c_str());
    if (iterm == nullptr) {
      logger_->error(utl::DFT,
                     38,
                     "ITerm {}/{} not found for {} port",
                     term_info.first,
                     term_info.second.value(),
                     kScanOut);
    }
    if (iterm->getIoType() != odb::dbIoType::INPUT) {
      logger_->error(utl::DFT,
                     39,
                     "ITerm {}/{} for {} port is a {}",
                     term_info.first,
                     term_info.second.value(),
                     kScanOut,
                     iterm->getIoType().getString());
    }
    return ScanLoad(iterm);
  }
  auto bterm = block->findBTerm(with_name.data());
  if (bterm != nullptr) {
    if (bterm->getIoType() != odb::dbIoType::OUTPUT) {
      logger_->error(utl::DFT,
                     40,
                     "Top-level pin '{}' specified as {} is not an output port",
                     term_info.first,
                     kScanEnable);
    }
    return ScanLoad(bterm);
  }

  // TODO: Trace forward the scan out net so we can see if it is connected to a
  // top port or to functional logic
  odb::dbNet* scan_out_net = cell_scan_out.getNet();
  if (scan_out_net && top_block_ == scan_out_net->getBlock()) {
    // if the scan_out_net exists, and has an BTerm that is an OUTPUT, then we
    // can reuse that BTerm to act as scan_out, only if the block of the bterm
    // is the top block, otherwise we will punch a new port
    for (odb::dbBTerm* bterm : scan_out_net->getBTerms()) {
      if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
        return ScanLoad(bterm);
      }
    }
  }

  return CreateNewPort<ScanLoad>(block, with_name);
}

}  // namespace dft
