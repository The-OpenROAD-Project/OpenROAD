#include "ScanStitch.hh"

#include <boost/algorithm/string.hpp>
#include <deque>
#include <iostream>

namespace dft {

ScanStitch::ScanStitch(odb::dbDatabase* db,
                       bool per_chain_enable,
                       std::string scan_enable_name_pattern,
                       std::string scan_in_name_pattern,
                       std::string scan_out_name_pattern)
    : db_(db),
      per_chain_enable_(per_chain_enable),
      scan_enable_name_pattern_(scan_enable_name_pattern),
      scan_in_name_pattern_(scan_in_name_pattern),
      scan_out_name_pattern_(scan_out_name_pattern)
{
  odb::dbChip* chip = db_->getChip();
  top_block_ = chip->getBlock();
}

void ScanStitch::Stitch(
    const std::vector<std::unique_ptr<ScanChain>>& scan_chains,
    utl::Logger* logger)
{
  if (scan_chains.empty()) {
    return;
  }

  size_t enable_ordinal = 0;
  size_t ordinal = 0;
  for (const std::unique_ptr<ScanChain>& scan_chain : scan_chains) {
    Stitch(top_block_, *scan_chain, logger, ordinal, enable_ordinal);
    ordinal += 1;
    if (per_chain_enable_) {
      enable_ordinal += 1;
    }
  }
}

void ScanStitch::Stitch(odb::dbBlock* block,
                        const ScanChain& scan_chain,
                        utl::Logger* logger,
                        size_t ordinal,
                        size_t enable_ordinal)
{
  auto scan_enable_name
      = fmt::format(FMT_RUNTIME(scan_enable_name_pattern_), enable_ordinal);
  auto scan_enable = FindOrCreateScanEnable(block, scan_enable_name, logger);

  // Let's create the scan in and scan out of the chain
  auto scan_in_name = fmt::format(FMT_RUNTIME(scan_in_name_pattern_), ordinal);
  ScanDriver scan_in_driver = FindOrCreateScanIn(block, scan_in_name, logger);

  // We need fast pop for front and back
  std::deque<std::reference_wrapper<const std::unique_ptr<ScanCell>>>
      scan_cells;

  const std::vector<std::unique_ptr<ScanCell>>& original_scan_cells
      = scan_chain.getScanCells();

  std::copy(original_scan_cells.cbegin(),
            original_scan_cells.cend(),
            std::back_inserter(scan_cells));

  // All the cells in the scan chain are controlled by the same scan enable
  for (const std::unique_ptr<ScanCell>& scan_cell : scan_cells) {
    scan_cell->connectScanEnable(scan_enable);
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
  first_scan_cell->connectScanEnable(scan_enable);
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
      = fmt::format(FMT_RUNTIME(scan_out_name_pattern_), ordinal);
  ScanLoad scan_out_load = FindOrCreateScanOut(
      block, last_scan_cell->getScanOut(), scan_out_name, logger);
  last_scan_cell->connectScanOut(scan_out_load);
}

static std::pair<std::string, std::optional<std::string>> split_term_identifier(
    const std::string& input,
    const char* kind,
    utl::Logger* logger)
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

ScanDriver ScanStitch::FindOrCreateDriver(const char* kind,
                                          odb::dbBlock* block,
                                          const std::string& with_name,
                                          utl::Logger* logger)
{
  auto term_info = split_term_identifier(with_name, kind, logger);

  if (term_info.second.has_value()) {  // Instance/ITerm
    auto inst = block->findInst(term_info.first.c_str());
    if (inst == nullptr) {
      logger->error(utl::DFT,
                    34,
                    "Instance {} not found for {} port",
                    term_info.first,
                    kind);
    }
    auto iterm = inst->findITerm(term_info.second.value().c_str());
    if (iterm == nullptr) {
      logger->error(utl::DFT,
                    35,
                    "ITerm {}/{} not found for {} port",
                    term_info.first,
                    term_info.second.value(),
                    kind);
    }
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      logger->error(utl::DFT,
                    36,
                    "ITerm {}/{} for {} port is a {}",
                    term_info.first,
                    term_info.second.value(),
                    kind,
                    iterm->getIoType().getString());
    }
    return ScanDriver(iterm);
  } else {  // BTerm
    auto bterm = block->findBTerm(with_name.data());
    if (bterm != nullptr) {
      // We don't actually care if it's an output, that works here too.
      return ScanDriver(bterm);
    }
    return CreateNewPort<ScanDriver>(block, with_name, logger);
  }
}

ScanDriver ScanStitch::FindOrCreateScanEnable(odb::dbBlock* block,
                                              const std::string& with_name,
                                              utl::Logger* logger)
{
  return FindOrCreateDriver("scan-enable", block, with_name, logger);
}

ScanDriver ScanStitch::FindOrCreateScanIn(odb::dbBlock* block,
                                          const std::string& with_name,
                                          utl::Logger* logger)
{
  return FindOrCreateDriver("scan-in", block, with_name, logger);
}

ScanLoad ScanStitch::FindOrCreateScanOut(odb::dbBlock* block,
                                         const ScanDriver& cell_scan_out,
                                         const std::string& with_name,
                                         utl::Logger* logger)
{
  const char* kind = "scan-out";
  auto term_info = split_term_identifier(with_name, kind, logger);

  if (term_info.second.has_value()) {  // Instance/ITerm
    auto inst = block->findInst(term_info.first.c_str());
    if (inst == nullptr) {
      logger->error(utl::DFT,
                    37,
                    "Instance {} not found for {} port",
                    term_info.first,
                    kind);
    }
    auto iterm = inst->findITerm(term_info.second.value().c_str());
    if (iterm == nullptr) {
      logger->error(utl::DFT,
                    38,
                    "ITerm {}/{} not found for {} port",
                    term_info.first,
                    term_info.second.value(),
                    kind);
    }
    if (iterm->getIoType() != odb::dbIoType::INPUT) {
      logger->error(utl::DFT,
                    39,
                    "ITerm {}/{} for {} port is a {}",
                    term_info.first,
                    term_info.second.value(),
                    kind,
                    iterm->getIoType().getString());
    }
    return ScanLoad(iterm);
  }
  auto bterm = block->findBTerm(with_name.data());
  if (bterm != nullptr) {
    if (bterm->getIoType() != odb::dbIoType::OUTPUT) {
      logger->error(utl::DFT,
                    40,
                    "Top-level pin '{}' specified as {} is not an output port",
                    term_info.first,
                    kind);
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

  return CreateNewPort<ScanLoad>(block, with_name, logger);
}

}  // namespace dft
