#include "ScanStitch.hh"

#include <deque>
#include <iostream>

namespace {

constexpr std::string_view kScanEnableNamePattern = "scan_enable_{}";
constexpr std::string_view kScanInNamePattern = "scan_in_{}";
constexpr std::string_view kScanOutNamePattern = "scan_out_{}";

}  // namespace

namespace dft {

ScanStitch::ScanStitch(odb::dbDatabase* db) : db_(db)
{
  odb::dbChip* chip = db_->getChip();
  top_block_ = chip->getBlock();
}

void ScanStitch::Stitch(
    const std::vector<std::unique_ptr<ScanChain>>& scan_chains)
{
  // TODO: For now, we only use one scan enable for all the chains. We may
  // support in the future multiple test modes
  if (scan_chains.empty()) {
    return;
  }

  ScanDriver scan_enable = FindOrCreateScanEnable(top_block_);
  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();

  for (const std::unique_ptr<ScanChain>& scan_chain : scan_chains) {
    Stitch(block, *scan_chain, scan_enable);
  }
}

void ScanStitch::Stitch(odb::dbBlock* block,
                        const ScanChain& scan_chain,
                        const ScanDriver& scan_enable)
{
  // Let's create the scan in and scan out of the chain
  // TODO: Suport user defined scan signals

  ScanDriver scan_in_port = FindOrCreateScanIn(block);

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
  first_scan_cell->connectScanIn(scan_in_port);

  if (!scan_cells.empty()) {
    scan_cells.begin()->get()->connectScanIn(first_scan_cell->getScanOut());
  } else {
    // If last_scan_cell == first_scan_cell, then scan in was already connected
    if (last_scan_cell != first_scan_cell) {
      last_scan_cell->connectScanIn(first_scan_cell->getScanOut());
    }
  }

  // Let's connect the last cell
  ScanLoad scan_out_port
      = FindOrCreateScanOut(block, last_scan_cell->getScanOut());
  last_scan_cell->connectScanOut(scan_out_port);
}

ScanDriver ScanStitch::FindOrCreateScanEnable(odb::dbBlock* block)
{
  // TODO: For now we will create a new scan_enable pin at the top level. We
  // need to support defining DFT signals for scan_enable
  return CreateNewPort<ScanDriver>(block, kScanEnableNamePattern);
}

ScanDriver ScanStitch::FindOrCreateScanIn(odb::dbBlock* block)
{
  // TODO: For now we will create a new scan_in pin at the top level. We
  // need to support defining DFT signals for scan_in
  return CreateNewPort<ScanDriver>(block, kScanInNamePattern);
}

ScanLoad ScanStitch::FindOrCreateScanOut(odb::dbBlock* block,
                                         const ScanDriver& cell_scan_out)
{
  // TODO: For now we will create a new scan_out pin at the top level if we need
  // one. We need to support defining DFT signals for scan_out

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

  return CreateNewPort<ScanLoad>(block, kScanOutNamePattern);
}

}  // namespace dft
