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

  odb::dbChip* chip = db_->getChip();
  odb::dbBlock* block = chip->getBlock();
  FindOrCreateScanEnable(block);

  for (const std::unique_ptr<ScanChain>& scan_chain : scan_chains) {
    Stitch(block, *scan_chain);
  }
}

void ScanStitch::Stitch(odb::dbBlock* block, const ScanChain& scan_chain)
{
  // Let's create the scan in and scan out of the chain
  // TODO: Suport user defined scan signals

  ScanDriver scan_in_port = FindOrCreateScanIn(block);

  // We need fast pop for front and back
  std::deque<std::shared_ptr<ScanCell>> scan_cells;

  const std::vector<std::shared_ptr<ScanCell>>& falling_edge
      = scan_chain.getFallingEdgeScanCells();
  const std::vector<std::shared_ptr<ScanCell>>& rising_edge
      = scan_chain.getRisingEdgeScanCells();

  // Falling edge first
  std::copy(
      falling_edge.begin(), falling_edge.end(), std::back_inserter(scan_cells));
  std::copy(
      rising_edge.begin(), rising_edge.end(), std::back_inserter(scan_cells));

  // All the cells in the scan chain are controlled by the same scan enable
  for (const auto& scan_cell : scan_cells) {
    scan_cell->connectScanEnable(*scan_enable_);
  }

  // Lets get the first and last cell
  const std::shared_ptr<ScanCell>& first_scan_cell = *scan_cells.begin();
  const std::shared_ptr<ScanCell>& last_scan_cell = *(scan_cells.end() - 1);

  if (!scan_cells.empty()) {
    scan_cells.pop_front();
  }

  if (!scan_cells.empty()) {
    scan_cells.pop_back();
  }

  for (auto current = scan_cells.begin(); current != scan_cells.end();
       ++current) {
    auto next = current + 1;
    // Connects current cell scan out to next cell scan in
    (*next)->connectScanIn((*current)->getScanOut());
  }

  // Let's connect the first cell
  first_scan_cell->connectScanEnable(*scan_enable_);
  first_scan_cell->connectScanIn(scan_in_port);

  if (!scan_cells.empty()) {
    (*scan_cells.begin())->connectScanIn(first_scan_cell->getScanOut());
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

void ScanStitch::FindOrCreateScanEnable(odb::dbBlock* block)
{
  // TODO: For now we will create a new scan_enable pin at the top level. We
  // need to support defining DFT signals for scan_enable
  for (uint64_t scan_enable_number = 1;; ++scan_enable_number) {
    std::string scan_enable_pin_name
        = fmt::format(kScanEnableNamePattern, scan_enable_number);
    odb::dbBTerm* scan_enable = block->findBTerm(scan_enable_pin_name.c_str());
    if (!scan_enable) {
      odb::dbNet* net = odb::dbNet::create(block, scan_enable_pin_name.c_str());
      net->setSigType(odb::dbSigType::SCAN);
      odb::dbBTerm* scan_enable_port
          = odb::dbBTerm::create(net, scan_enable_pin_name.c_str());
      scan_enable_port->setSigType(odb::dbSigType::SCAN);
      scan_enable_port->setIoType(odb::dbIoType::INPUT);
      scan_enable_.emplace(scan_enable_port);
      return;
    }
  }
}

ScanDriver ScanStitch::FindOrCreateScanIn(odb::dbBlock* block)
{
  // TODO: For now we will create a new scan_in pin at the top level. We
  // need to support defining DFT signals for scan_in

  for (uint64_t scan_in_number = 1;; ++scan_in_number) {
    std::string scan_in_pin_name
        = fmt::format(kScanInNamePattern, scan_in_number);
    odb::dbBTerm* scan_in = block->findBTerm(scan_in_pin_name.c_str());
    if (!scan_in) {
      odb::dbNet* net = odb::dbNet::create(block, scan_in_pin_name.c_str());
      net->setSigType(odb::dbSigType::SCAN);
      scan_in = odb::dbBTerm::create(net, scan_in_pin_name.c_str());
      scan_in->setSigType(odb::dbSigType::SCAN);
      scan_in->setIoType(odb::dbIoType::INPUT);
      return ScanDriver(scan_in);
    }
  }
  return ScanDriver(static_cast<odb::dbBTerm*>(nullptr));
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

  for (uint64_t scan_out_number = 1;; ++scan_out_number) {
    std::string scan_out_pin_name
        = fmt::format(kScanOutNamePattern, scan_out_number);
    odb::dbBTerm* scan_out = block->findBTerm(scan_out_pin_name.c_str());
    if (!scan_out) {
      odb::dbNet* net = odb::dbNet::create(block, scan_out_pin_name.c_str());
      net->setSigType(odb::dbSigType::SCAN);
      scan_out = odb::dbBTerm::create(net, scan_out_pin_name.c_str());
      scan_out->setSigType(odb::dbSigType::SCAN);
      scan_out->setIoType(odb::dbIoType::OUTPUT);
      return ScanLoad(scan_out);
    }
  }
  return ScanLoad(static_cast<odb::dbBTerm*>(nullptr));
}

}  // namespace dft
